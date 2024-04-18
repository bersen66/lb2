#pragma once

#include <boost/asio.hpp>
#include <lb/tcp/selectors.hpp>

#include <memory>

namespace lb::tcp {

class Connector {
public:
    using ResolverResults = boost::asio::ip::tcp::resolver::results_type;
    using ResolverQuery = boost::asio::ip::tcp::resolver::query;
public:
    explicit Connector(boost::asio::io_context& ctx, SelectorPtr selector);

    Connector(const Connector&) = delete;
    Connector& operator=(const Connector& other) = delete;
    Connector(Connector&&) = delete;
    Connector& operator=(Connector&&) = delete;

    void MakeAndRunSession(boost::asio::ip::tcp::socket client);
private:
    void HandleAsyncResolve(const boost::system::error_code& ec, ResolverResults results, boost::asio::ip::tcp::socket client_socket);
private:
    boost::asio::io_context& ioc;
    boost::asio::ip::tcp::resolver resolver;
    ::lb::tcp::SelectorPtr selector;
};

} // namespace lb::tcp