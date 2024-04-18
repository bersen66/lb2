#include <lb/logging.hpp>
#include <lb/tcp/connector.hpp>
#include <lb/tcp/session.hpp>
#include "connector.hpp"

namespace lb::tcp {

Connector::Connector(boost::asio::io_context& ctx, SelectorPtr selector)
    : ioc(ctx)
    , resolver(boost::asio::make_strand(ctx))
    , selector(selector)
{}

void Connector::MakeAndRunSession(boost::asio::ip::tcp::socket client_socket)
{
    // TODO: selection of backend
    DEBUG("In connector");
    Backend backend = selector->SelectBackend(client_socket.remote_endpoint());

    if (backend.IsIpEndpoint()) {
        DEBUG("Is ip endpoint");
        auto server_socket = std::make_shared<boost::asio::ip::tcp::socket>(client_socket.get_executor());

        server_socket->async_connect(
            backend.AsEndpoint(),
            [this, server_socket, client_socket=std::move(client_socket), backend=std::move(backend)] (const boost::system::error_code& error) mutable
            {
                if (error) {
                    ERROR("{}", error.message());
                    if (error == boost::asio::error::connection_refused) {
                        selector->ExcludeBackend(backend);
                        MakeAndRunSession(std::move(client_socket));
                    }
                    return;
                }
                auto connection = std::make_shared<Session>(std::move(client_socket), std::move(*server_socket));
                connection->Run();
            });
    } else if (backend.IsUrl()) {
        DEBUG("Is url");
        auto server_socket = std::make_shared<boost::asio::ip::tcp::socket>(client_socket.get_executor());

        const auto& url = backend.AsUrl();
        DEBUG("URL: hostname: {}, port: {}", url.Hostname(), url.Port());
        ResolverQuery resolve_query{url.Hostname(), url.Port()};
        resolver.async_resolve(
            resolve_query,
            [this, server_socket, client_socket=std::move(client_socket), backend=std::move(backend)](const boost::system::error_code& error, ResolverResults resolver_results) mutable
            {
                if (error) {
                    ERROR("Resolve error: {}", error.message());
                    if (error == boost::asio::error::connection_refused) {
                        selector->ExcludeBackend(backend);
                        MakeAndRunSession(std::move(client_socket));
                    }
                    return;
                }
                DEBUG("Resolved successfully!");
                boost::asio::async_connect(
                    *server_socket,
                    resolver_results,
                    [this, server_socket, client_socket=std::move(client_socket)] (const boost::system::error_code& error, const boost::asio::ip::tcp::endpoint& endpoint) mutable
                    {
                        if (error) {
                            ERROR("{}", error.message());
                            return;
                        }
                        auto connection = std::make_shared<Session>(std::move(client_socket), std::move(*server_socket));
                        connection->Run();
                    });
            });
    }
}

} // namespace lb::tcp
