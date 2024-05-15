#include <unordered_map>
#include <lb/tcp/selectors.hpp>
#include <lb/logging.hpp>
#include <pumba/pumba.hpp>
#include <boost/lexical_cast.hpp>

namespace lb::tcp
{

Backend::Backend(const std::string& ip_address, int port)
    : value(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip_address), port))
{}

Backend::Backend(const std::string& url_string)
    : value(Url(url_string))
{}

bool Backend::IsUrl() const
{
    return std::holds_alternative<UrlType>(value);
}

bool Backend::IsIpEndpoint() const
{
    return std::holds_alternative<EndpointType>(value);
}

const Backend::EndpointType& Backend::AsEndpoint() const
{
    return std::get<EndpointType>(value);
}

const Backend::UrlType& Backend::AsUrl() const
{
    return std::get<UrlType>(value);
}

bool Backend::operator==(const Backend& other) const
{
    if (IsUrl() && other.IsUrl()) {
        return AsUrl() == other.AsUrl();
    }

    if (IsIpEndpoint() && other.IsIpEndpoint()) {
        return AsEndpoint() == other.AsEndpoint();
    }

    return false;
}

std::string Backend::ToString() const
{
    if (IsUrl()) {
        return AsUrl().ToString();
    } else {
        return AsEndpoint().address().to_string() + ":" + std::to_string(AsEndpoint().port());
    }
}

Backend::Backend(const Backend::EndpointType& ep)
    : value(ep)
{}

Backend::Backend(const Backend::UrlType& url)
    : value(url)
{}


std::ostream& operator<<(std::ostream& out, const Backend& backend)
{
    if (backend.IsIpEndpoint()) {
        out << backend.AsEndpoint().address() << ", " << backend.AsEndpoint().port();
    } else {
        out << backend.AsUrl().Hostname();
    }
    return out;
}

SelectorPtr DetectSelector(const YAML::Node& node)
{
    if (!node["load_balancing"].IsDefined()) {
        EXCEPTION("Load balancing node missed in configs");
    }

    const YAML::Node& balancing_node = node["load_balancing"];
    static const std::unordered_map<std::string, SelectorType> selector_switch = {
        {"round_robin", SelectorType::ROUND_ROBIN},
        {"weighted_round_robin", SelectorType::WEIGHTED_ROUND_ROBIN},
        {"consistent_hash", SelectorType::CONSISTENT_HASH},
        {"ip_hash", SelectorType::IP_HASH},
        {"least_connections", SelectorType::LEAST_CONNECTIONS},
        {"least_response_time", SelectorType::LEAST_RESPONSE_TIME}
    };

    if (!balancing_node["algorithm"].IsDefined()) {
        EXCEPTION("Missed algorithm field!");
    }

    auto it = selector_switch.find(balancing_node["algorithm"].as<std::string>());
    if (it == selector_switch.end()) {
        EXCEPTION("Unknown balancing algorithm: {}", balancing_node["algorithm"].as<std::string>());
    }

    const auto& [name, type] = *it;
    switch (type) {
        case SelectorType::ROUND_ROBIN: {
            SelectorPtr rr = std::make_shared<RoundRobinSelector>();
            rr->Configure(balancing_node);
            return rr;
        }
        break;
        case SelectorType::WEIGHTED_ROUND_ROBIN: {
            SelectorPtr wrr = std::make_shared<WeightedRoundRobinSelector>();
            wrr->Configure(balancing_node);
            return wrr;
        }
        break;
        case SelectorType::IP_HASH: {
            SelectorPtr ip = std::make_shared<IpHashSelector>();
            ip->Configure(balancing_node);
            return ip;
        } break;
        case SelectorType::CONSISTENT_HASH: {
            if (!balancing_node["replicas"].as<std::size_t>()) {
                EXCEPTION("Missed replicas field!");
            }
            std::size_t replicas_num = balancing_node["replicas"].as<std::size_t>();
            SelectorPtr ch = std::make_shared<ConsistentHashSelector>(replicas_num);
            ch->Configure(balancing_node);
            return ch;
        } break;
        case SelectorType::LEAST_CONNECTIONS: {
            SelectorPtr lc = std::make_shared<LeastConnectionsSelector>();
            lc->Configure(balancing_node);
            return lc;
        }
        break;
        case SelectorType::LEAST_RESPONSE_TIME: {
            SelectorPtr lrt = std::make_shared<LeastResponseTimeSelector>();
            lrt->Configure(balancing_node);
            return lrt;
        }
        break;
        default: {
            STACKTRACE("Selector {} is not implemented", name);
        }
    }

}


// ============================ RoundRobinSelector ============================
void RoundRobinSelector::Configure(const YAML::Node &balancing_node)
{
    INFO("Configuring RoundRobinSelector");
    if (!balancing_node["endpoints"].IsDefined()) {
        STACKTRACE("Round-robin endpoints node is missed");
    }
    const YAML::Node& ep_node = balancing_node["endpoints"];
    if (!ep_node.IsSequence()) {
        EXCEPTION("endpoints node must be a sequence");
    }
    backends_.reserve(ep_node.size());
    for (const YAML::Node& ep : ep_node) {
        if (ep["url"].IsDefined()) {
            backends_.emplace_back(ep["url"].as<std::string>());
            continue;
        }

        if (!ep["ip"].IsDefined()) {
            STACKTRACE("{} missed {} field", ep, "ip");
        }
        if (!ep["port"].IsDefined()) {
            STACKTRACE("{} missed {} field", ep, "port");
        }

        backends_.emplace_back(ep["ip"].as<std::string>(), ep["port"].as<int>());
    }

    for (const auto& backend : backends_) {
        DEBUG("\t{}", backend);
    }
}

Backend RoundRobinSelector::SelectBackend(const EndpointType &notused)
{
    boost::mutex::scoped_lock lock(mutex);
    counter = (counter + 1) % backends_.size();
    DEBUG("Round robin selected: {}", counter);
    return backends_[counter];
}

void RoundRobinSelector::ExcludeBackend(const Backend& backend)
{
    boost::mutex::scoped_lock lock(mutex);
    backends_.erase(std::remove(backends_.begin(), backends_.end(), backend), backends_.end());
    INFO("Excluded backend: {}", backend);
    if (backends_.empty()) {
        EXCEPTION("All backends are excluded!");
    }
}

SelectorType RoundRobinSelector::Type() const
{
    return SelectorType::ROUND_ROBIN;
}

// ============================ WeightedRoundRobinSelector ============================

std::size_t ReadWeight(const YAML::Node& ep)
{
    std::size_t weight = 0;
    if (ep["weight"].IsDefined()) {
        weight = ep["weight"].as<std::size_t>();
    } else {
        STACKTRACE("{} missed {} field", ep, "weight");
    }
    return weight;
}

void WeightedRoundRobinSelector::Configure(const YAML::Node &balancing_node)
{
    INFO("Configuring WeightedRoundRobinSelector");
    if (!balancing_node["endpoints"].IsDefined()) {
        STACKTRACE("Weighted-round-robin endpoints node is missed");
    }
    const YAML::Node& ep_node = balancing_node["endpoints"];
    if (!ep_node.IsSequence()) {
        EXCEPTION("endpoints node must be a sequence");
    }
    backends_.reserve(ep_node.size());
    for (const YAML::Node& ep : ep_node) {
        std::size_t weight = ReadWeight(ep);
        if (ep["url"].IsDefined()) {
            backends_.push_back(
                WeightWrapper{
                    .backend=Backend(ep["url"].as<std::string>()),
                    .weight=weight,
            });
            continue;
        }

        if (ep["ip"].IsDefined()) {
            if (!ep["port"].IsDefined()) {
                STACKTRACE("{} missed {} field", ep, "port");
            }

            backends_.push_back(
                WeightWrapper{
                    .backend=Backend(ep["ip"].as<std::string>(), ep["port"].as<int>()),
                    .weight=weight,
            });
            continue;
        }

        STACKTRACE("Weighted-round-robin endpoint is not defined: {}", ep);

    }

    static const auto weight_compare =
        [](const WeightWrapper& a, const WeightWrapper& b) -> bool {
            return a.weight > b.weight;
        };

    std::sort(backends_.begin(), backends_.end(), weight_compare);

    for (const auto& backend : backends_) {
        DEBUG("backend: {} weight: {}", backend.backend, backend.weight);
    }
}

Backend WeightedRoundRobinSelector::SelectBackend(const boost::asio::ip::tcp::endpoint& notused)
{
    boost::mutex::scoped_lock lock(mutex_);
    const auto& [backend, weight, current] = backends_[counter_];
    AdvanceCounter();
    DEBUG("Weighted round robin selected: {}", backend);
    return backend;
}

void WeightedRoundRobinSelector::ExcludeBackend(const Backend& backend)
{
    boost::mutex::scoped_lock lock(mutex_);
    for (std::size_t i = 0; i < backends_.size(); i++) {
        if (backends_[i].backend == backend) {
            if (i != counter_) {
                backends_[counter_].current=0;
            }
            backends_.erase(backends_.begin() + i);
            break;
        }
    }
    counter_ = 0;
    if (backends_.empty()) {
        EXCEPTION("All backends are excluded!");
    }
}

SelectorType WeightedRoundRobinSelector::Type() const
{
    return SelectorType::WEIGHTED_ROUND_ROBIN;
}

void WeightedRoundRobinSelector::AdvanceCounter()
{
    auto& entry = backends_[counter_];
    if (entry.current + 1 < entry.weight) {
        entry.current++;
    } else {
        entry.current = 0;
        counter_ = (counter_ + 1) % backends_.size();
    }
}

// ============================ IpHashSelector ============================

void IpHashSelector::Configure(const YAML::Node& balancing_node)
{
    INFO("Configuring IpHashSelector");
    if (!balancing_node["endpoints"].IsDefined()) {
        STACKTRACE("Ip-hash endpoints node is missed");
    }
    const YAML::Node& ep_node = balancing_node["endpoints"];
    if (!ep_node.IsSequence()) {
        EXCEPTION("endpoints node must be a sequence");
    }
    backends_.reserve(ep_node.size());
    for (const YAML::Node& ep : ep_node) {
        if (ep["url"].IsDefined()) {
            backends_.emplace_back(ep["url"].as<std::string>());
            continue;
        }

        if (!ep["ip"].IsDefined()) {
            STACKTRACE("{} missed {} field", ep, "ip");
        }
        if (!ep["port"].IsDefined()) {
            STACKTRACE("{} missed {} field", ep, "port");
        }

        backends_.emplace_back(ep["ip"].as<std::string>(), ep["port"].as<int>());
    }

    for (const auto& backend : backends_) {
        DEBUG("\t{}", backend);
    }
}

Backend IpHashSelector::SelectBackend(const boost::asio::ip::tcp::endpoint& client_address)
{
    boost::mutex::scoped_lock lock(mutex_);
    static auto ComputeHash = [&](const boost::asio::ip::tcp::endpoint& ep) -> std::size_t {
        return std::hash<std::string>{}(ep.address().to_string()) * 37 +
               ep.port() * 37 * 37;
    };

    Backend result = backends_[ComputeHash(client_address) % backends_.size()];
    // DEBUG("Ip hash selected: {}", result);
    // DEBUG("Hash: {}", ComputeHash(client_address));
    return result;
}

void IpHashSelector::ExcludeBackend(const Backend& backend)
{
    boost::mutex::scoped_lock lock(mutex_);
    backends_.erase(std::remove(backends_.begin(), backends_.end(), backend), backends_.end());
    if (backends_.empty()) {
        EXCEPTION("All backends are excluded!");
    }
}

SelectorType IpHashSelector::Type() const
{
    return SelectorType::IP_HASH;
}

// ============================ ConsistentHashSelector ============================

BackendCHTraits::HashType BackendCHTraits::GetHash(const Backend& backend)
{

    static std::hash<std::string> hash{};
    std::size_t res = hash(backend.ToString());
    return res;
}

std::vector<BackendCHTraits::HashType>
BackendCHTraits::Replicate(const Backend& backend, std::size_t replicas)
{
    std::vector<HashType> result;
    result.reserve(replicas);

    for (int i = 0; i < replicas; ++i) {
        result.emplace_back(GetHash(backend.ToString() + "#" + boost::lexical_cast<std::string>(i)));
    }

    return result;
}


ConsistentHashSelector::ConsistentHashSelector(std::size_t spawn_replicas)
    : ring_(spawn_replicas)
{}

void ConsistentHashSelector::Configure(const YAML::Node &balancing_node)
{
    INFO("Configuring ConsistentHashSelector");
    if (!balancing_node["endpoints"].IsDefined()) {
        STACKTRACE("Consistent hash endpoints node is missed");
    }
    const YAML::Node& ep_node = balancing_node["endpoints"];
    if (!ep_node.IsSequence()) {
        EXCEPTION("endpoints node must be a sequence");
    }

    for (const YAML::Node& ep : ep_node) {
        if (ep["url"].IsDefined()) {
            ring_.EmplaceNode(ep["url"].as<std::string>());
            continue;
        }

        if (!ep["ip"].IsDefined()) {
            STACKTRACE("{} missed {} field", ep, "ip");
        }
        if (!ep["port"].IsDefined()) {
            STACKTRACE("{} missed {} field", ep, "port");
        }

        ring_.EmplaceNode(ep["ip"].as<std::string>(), ep["port"].as<int>());
    }


}

Backend ConsistentHashSelector::SelectBackend(const boost::asio::ip::tcp::endpoint& client)
{
    boost::mutex::scoped_lock lock(mutex_);
    Backend result = ring_.SelectNode(client);
    DEBUG("Selected: {}", result);
    return result;
}

void ConsistentHashSelector::ExcludeBackend(const Backend& backend)
{
    boost::mutex::scoped_lock lock(mutex_);
    ring_.EraseNode(backend);
    DEBUG("Excluded: {}", backend);
    if (ring_.Empty()) {
        EXCEPTION("All backends are excluded!");
    }
}

SelectorType ConsistentHashSelector::Type() const
{
    return SelectorType::CONSISTENT_HASH;
}

// ============================ LeastConnectionsSelector ============================

void LeastConnectionsSelector::Configure(const YAML::Node& config) {
    INFO("Configuring LeastConnectionsSelector");
    if (!config["endpoints"].IsDefined()) {
        STACKTRACE("Least connections endpoints node is missed");
    }
    const YAML::Node& ep_node = config["endpoints"];
    if (!ep_node.IsSequence()) {
        EXCEPTION("endpoints node must be a sequence");
    }

    for (const YAML::Node& ep : ep_node) {
        if (ep["url"].IsDefined()) {
            InsertBackend(Backend(ep["url"].as<std::string>()));
            continue;
        }

        if (!ep["ip"].IsDefined()) {
            STACKTRACE("{} missed {} field", ep, "ip");
        }
        if (!ep["port"].IsDefined()) {
            STACKTRACE("{} missed {} field", ep, "port");
        }

        InsertBackend(Backend(ep["ip"].as<std::string>(), ep["port"].as<int>()));
    }

    for (const auto& backend : backends_) {
        DEBUG("\t{}", backend.b);
    }
}


void LeastConnectionsSelector::InsertBackend(Backend&& b)
{
    boost::recursive_mutex::scoped_lock lock(mutex_);
    HandleType handle = backends_.push(CounterWrapper{.b = std::move(b),
                                                      .counter = 0});
    handle_pool_[(*handle).b.ToString()] = std::move(handle);
}


Backend LeastConnectionsSelector::SelectBackend(const boost::asio::ip::tcp::endpoint& notused)
{
    boost::recursive_mutex::scoped_lock lock(mutex_);
    Backend result = backends_.top().b;
    IncreaseConnectionCount(backends_.top().b);
    return result;
}

void LeastConnectionsSelector::ExcludeBackend(const Backend& backend)
{
    boost::recursive_mutex::scoped_lock lock(mutex_);

    const std::string& str = backend.ToString();
    if (auto it = handle_pool_.find(str); it != handle_pool_.end()) {
        HandleType& handle = it->second;

        backends_.erase(handle);
        handle_pool_.erase(str);

        if (backends_.empty()) {
            EXCEPTION("All backends are excluded!");
        }
    }
}

SelectorType LeastConnectionsSelector::Type() const
{
    return SelectorType::LEAST_CONNECTIONS;
}

void LeastConnectionsSelector::IncreaseConnectionCount(const Backend& backend)
{
    boost::recursive_mutex::scoped_lock lock(mutex_);
    const std::string& str = backend.ToString();
    if (auto it = handle_pool_.find(str); it != handle_pool_.end()) {
        HandleType& handle = it->second;
        (*handle).counter++;
        backends_.decrease(handle);
    }
}

void LeastConnectionsSelector::DecreaseConnectionCount(const Backend& backend)
{
    boost::recursive_mutex::scoped_lock lock(mutex_);
    const std::string& str = backend.ToString();
    if (auto it = handle_pool_.find(str); it != handle_pool_.end()) {
        HandleType& handle = it->second;
        if ((*handle).counter > 0) {
            (*handle).counter--;
            backends_.decrease(handle);
        }
    }
}

// ============================ LeastResponseTimeSelector ============================

void LeastResponseTimeSelector::Configure(const YAML::Node& config)
{
    INFO("Configuring LeastResponseTimeSelector");
    if (!config["endpoints"].IsDefined()) {
        STACKTRACE("Least response time endpoints node is missed");
    }
    const YAML::Node& ep_node = config["endpoints"];
    if (!ep_node.IsSequence()) {
        EXCEPTION("endpoints node must be a sequence");
    }

    for (const YAML::Node& ep : ep_node) {
        if (ep["url"].IsDefined()) {
            InsertBackend(Backend(ep["url"].as<std::string>()));
            continue;
        }

        if (!ep["ip"].IsDefined()) {
            STACKTRACE("{} missed {} field", ep, "ip");
        }
        if (!ep["port"].IsDefined()) {
            STACKTRACE("{} missed {} field", ep, "port");
        }

        InsertBackend(Backend(ep["ip"].as<std::string>(), ep["port"].as<int>()));

    }

    for (const auto& backend : backends_) {
        DEBUG("\t{}", backend.backend);
    }
}


Backend LeastResponseTimeSelector::SelectBackend(const boost::asio::ip::tcp::endpoint& notused)
{
    boost::mutex::scoped_lock lock(mutex_);
    return backends_.top().backend;
}

void LeastResponseTimeSelector::ExcludeBackend(const Backend& backend)
{
    boost::mutex::scoped_lock lock(mutex_);
    const std::string& str = backend.ToString();
    if (auto it = handle_pool_.find(str); it != handle_pool_.end()) {
        HandleType& handle = it->second;
        backends_.erase(handle);
        handle_pool_.erase(str);
        if (backends_.empty()) {
            EXCEPTION("All backends are excluded!");
        }
    }
}

SelectorType LeastResponseTimeSelector::Type() const
{
    return SelectorType::LEAST_RESPONSE_TIME;
}

void LeastResponseTimeSelector::InsertBackend(Backend&& b)
{
    boost::mutex::scoped_lock lock(mutex_);
    HandleType handle = backends_.push(AverageTimeWrapper{.backend = std::move(b)});
    handle_pool_[(*handle).backend.ToString()] = std::move(handle);
}

void LeastResponseTimeSelector::AddResponseTime(const Backend& backend, long response_time)
{
    boost::mutex::scoped_lock lock(mutex_);
    const std::string& str = backend.ToString();
    if (auto it = handle_pool_.find(str); it != handle_pool_.end()) {
        HandleType& handle = it->second;
        double old_ema = (*handle).response_time_ema;
        (*handle).response_time_ema = (1 - (*handle).alpha) * old_ema + (*handle).alpha * response_time;
        if (old_ema > (*handle).response_time_ema) {
            backends_.decrease(handle);
        } else {
            backends_.increase(handle);
        }
    }
}

} // namespace lb::tcp
