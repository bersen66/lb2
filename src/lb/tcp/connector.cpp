#include <lb/logging.hpp>
#include <lb/tcp/connector.hpp>
#include <lb/tcp/session.hpp>

namespace lb::tcp {

Connector::Connector(boost::asio::io_context& ctx, SelectorPtr selector)
    : ioc(ctx)
    , resolver(boost::asio::make_strand(ctx))
    , selector(selector)
{}

SessionPtr MakeSession(SelectorPtr& selector, boost::asio::ip::tcp::socket client_socket, boost::asio::ip::tcp::socket server_socket, Backend backend)
{

    switch(selector->Type()) {
        case SelectorType::LEAST_CONNECTIONS: {
            std::shared_ptr<LeastConnectionsSelector>  lc_selector = std::dynamic_pointer_cast<LeastConnectionsSelector>(selector);
            return std::make_shared<LeastConnectionsHttpSession>(std::move(client_socket), std::move(server_socket), lc_selector, backend);
        } break;
        case SelectorType::LEAST_RESPONSE_TIME: {
            std::shared_ptr<LeastResponseTimeSelector> lrt_selector = std::dynamic_pointer_cast<LeastResponseTimeSelector>(selector);
            return std::make_shared<LeastResponseTimeHttpSession>(std::move(client_socket), std::move(server_socket), lrt_selector, backend);
        } break;
        default: {
            return std::make_shared<HttpSession>(std::move(client_socket), std::move(server_socket));
        }
    }
}


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
                        selector->ExcludeBackend(std::move(backend));
                        MakeAndRunSession(std::move(client_socket));
                    }
                    return;
                }
                SessionPtr session = MakeSession(selector, std::move(client_socket), std::move(*server_socket), std::move(backend));
                session->Run();
            });
    } else if (backend.IsUrl()) {
        DEBUG("Is url");
        auto server_socket = std::make_shared<boost::asio::ip::tcp::socket>(client_socket.get_executor());

        const auto& url = backend.AsUrl();
        DEBUG("URL: hostname: {}, port: {}", url.Hostname(), url.Port());
        ResolverQuery resolve_query{url.Hostname(), url.Port()};
        resolver.async_resolve(
            resolve_query,
            [this, server_socket, client_socket=std::move(client_socket), backend=std::move(backend)]
            (const boost::system::error_code& error, ResolverResults resolver_results) mutable
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
                    [this, server_socket, client_socket=std::move(client_socket), backend=std::move(backend)]
                    (const boost::system::error_code& error, const boost::asio::ip::tcp::endpoint& endpoint) mutable
                    {
                        if (error) {
                            ERROR("{}", error.message());
                            return;
                        }
                        SessionPtr session = MakeSession(selector, std::move(client_socket), std::move(*server_socket), std::move(backend));
                        session->Run();
                    });
            });
    }
}

} // namespace lb::tcp
