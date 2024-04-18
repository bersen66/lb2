#pragma once

#include <memory>
#include <ostream>
#include <variant>
#include <yaml-cpp/yaml.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/thread/mutex.hpp>
#include <lb/url.hpp>

namespace lb::tcp {

class Backend {
public:
    using EndpointType = boost::asio::ip::tcp::endpoint;
    using UrlType = Url;
public:
    bool IsUrl() const;
    bool IsIpEndpoint() const;

    Backend(const std::string& ip_address, int port);

    Backend(const std::string& url_string);

    const UrlType& AsUrl() const;
    const EndpointType& AsEndpoint() const;

    bool operator==(const Backend& other) const;
private:
    std::variant<EndpointType, UrlType> value;
};

std::ostream& operator<<(std::ostream& out, const Backend& backend);

enum class SelectorType {
    ROUND_ROBIN=0,
    CONSISTENT_HASH=1
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

} // namespace lb::tcp