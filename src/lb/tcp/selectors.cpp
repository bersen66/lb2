#include <unordered_map>
#include <lb/tcp/selectors.hpp>
#include <lb/logging.hpp>

namespace lb::tcp
{

Backend::Backend(const std::string& ip_address, int port)
    : value(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(ip_address), port))
{
    DEBUG("Backend {}:{} init as ip", ip_address, port);
}

Backend::Backend(const std::string& url_string)
    : value(Url(url_string))
{
    DEBUG("Backend {} init as url");
}

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
        {"consistent_hash", SelectorType::CONSISTENT_HASH}
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
        default: {
            STACKTRACE("Selector {} is not implemented", name);
        }
    }

}

void RoundRobinSelector::Configure(const YAML::Node &balancing_node)
{
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

} // namespace lb::tcp
