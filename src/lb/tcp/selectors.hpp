#pragma once

#include <memory>
#include <ostream>
#include <variant>
#include <unordered_map>

#include <yaml-cpp/yaml.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <pumba/pumba.hpp> // for pumba::ConsistentHashingRouter
#include <boost/heap/pairing_heap.hpp> // for boost::heap::pairing_heap
#include <lb/url.hpp>

namespace lb::tcp {

class Backend {
public:
    using EndpointType = boost::asio::ip::tcp::endpoint;
    using UrlType = Url;
public:
    bool IsUrl() const;
    bool IsIpEndpoint() const;

    Backend(const EndpointType& ep);

    Backend(const UrlType& url);

    Backend(const std::string& ip_address, int port);

    Backend(const std::string& url_string);

    const UrlType& AsUrl() const;
    const EndpointType& AsEndpoint() const;

    bool operator==(const Backend& other) const;

    std::string ToString() const;
private:
    std::variant<EndpointType, UrlType> value;
};


struct BackendHasher {
    std::size_t operator()(const lb::tcp::Backend& backend) const {
        static std::hash<std::string> hash{};
        return hash(backend.ToString());
    }
};

std::ostream& operator<<(std::ostream& out, const Backend& backend);

enum class SelectorType {
    ROUND_ROBIN=0,
    WEIGHTED_ROUND_ROBIN,
    IP_HASH,
    CONSISTENT_HASH,
    LEAST_CONNECTIONS,
    LEAST_RESPONSE_TIME
};

struct ISelector {
    virtual void Configure(const YAML::Node& config) = 0;
    virtual Backend SelectBackend(const boost::asio::ip::tcp::endpoint& client_socket) = 0;
    virtual void ExcludeBackend(const Backend& backend) = 0;
    virtual SelectorType Type() const = 0;
    virtual ~ISelector() = default;
};

using SelectorPtr = std::shared_ptr<ISelector>;

SelectorPtr DetectSelector(const YAML::Node& config);

class RoundRobinSelector final : public ISelector {
public:
    using EndpointType = boost::asio::ip::tcp::endpoint;
    using SocketType = boost::asio::ip::tcp::socket;
public:
    void Configure(const YAML::Node& config) override;

    Backend SelectBackend(const EndpointType& client_endpoint) override;

    void ExcludeBackend(const Backend& backend) override;

    SelectorType Type() const override;

private:
    boost::mutex mutex;
    std::size_t counter=0;
    std::vector<Backend> backends_;

};

class WeightedRoundRobinSelector final : public ISelector {
public:
    struct WeightWrapper {
        Backend backend;
        std::size_t weight;
        std::size_t current = 0;
    };
public:
    void Configure(const YAML::Node& config) override;

    Backend SelectBackend(const boost::asio::ip::tcp::endpoint& client_socket) override;

    void ExcludeBackend(const Backend& backend) override;

    SelectorType Type() const override;
private:
    void AdvanceCounter();
private:
    boost::mutex mutex_;
    std::vector<WeightWrapper> backends_; // guarded by mutex
    std::size_t counter_ = 0;
};

class IpHashSelector final : public ISelector {
public:
    void Configure(const YAML::Node& config) override;

    Backend SelectBackend(const boost::asio::ip::tcp::endpoint& client_socket) override;

    void ExcludeBackend(const Backend& backend) override;

    SelectorType Type() const override;
private:
    boost::mutex mutex_;
    std::vector<Backend> backends_;
};


struct BackendCHTraits {
    using HashType = std::size_t;

    static HashType GetHash(const Backend& backend);

    static std::vector<HashType>
    Replicate(const Backend& backend, std::size_t replicas);
};


class ConsistentHashSelector final : public ISelector {
public:

    ConsistentHashSelector(std::size_t spawn_replicas = 1);

    void Configure(const YAML::Node& config) override;

    Backend SelectBackend(const boost::asio::ip::tcp::endpoint& client_address) override;

    void ExcludeBackend(const Backend& backend) override;

    SelectorType Type() const override;

private:
    boost::mutex mutex_;
    pumba::ConsistentHashingRouter<Backend, BackendCHTraits> ring_; // guarded by mutex
};



class LeastConnectionsSelector final : public ISelector {
public:
    void Configure(const YAML::Node& config) override;
    Backend SelectBackend(const boost::asio::ip::tcp::endpoint& client_address) override;
    void ExcludeBackend(const Backend& backend) override;
    SelectorType Type() const override;
    void IncreaseConnectionCount(const Backend& backend);
    void DecreaseConnectionCount(const Backend& backend);
private:

    struct CounterWrapper {
        lb::tcp::Backend b;
        std::size_t counter = 0;
    };

    struct ConnectionsCompare {
        bool operator()(const CounterWrapper& lhs, const CounterWrapper& rhs) const
        {
            return lhs.counter > rhs.counter;
        }
    };

    using PairingMap = boost::heap::pairing_heap<CounterWrapper, boost::heap::compare<ConnectionsCompare>>;
    using HandleType = PairingMap::handle_type;

private:

    void InsertBackend(Backend&& b);

private:
    boost::recursive_mutex mutex_;
    std::unordered_map<std::string, HandleType> handle_pool_;
    PairingMap backends_;
};


class LeastResponseTimeSelector final : public ISelector {
public:
    void Configure(const YAML::Node& config) override;
    Backend SelectBackend(const boost::asio::ip::tcp::endpoint& client_address) override;
    void ExcludeBackend(const Backend& backend) override;
    SelectorType Type() const override;

    void AddResponseTime(const Backend& backend, long response_time);
private:
    void InsertBackend(Backend&& b);

private:
    struct AverageTimeWrapper {
        lb::tcp::Backend backend;
        double response_time_ewa = 0.0; // exponential weighted average for response time
        double alpha = 0.9;
    };

    struct ResponseTimeEMACompare{
        bool operator()(const AverageTimeWrapper& lhs, const AverageTimeWrapper& rhs) const
        {
            return lhs.response_time_ewa > rhs.response_time_ewa;
        }
    };
private:
    using PairingMap = boost::heap::pairing_heap<AverageTimeWrapper, boost::heap::compare<ResponseTimeEMACompare>>;
    using HandleType = PairingMap::handle_type;
private:
    boost::mutex mutex_;
    std::unordered_map<std::string, HandleType> handle_pool_;
    PairingMap backends_;

};

} // namespace lb::tcp
