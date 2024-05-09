#include <lb/logging.hpp>
#include <lb/tcp/connector.hpp>
#include <lb/tcp/session.hpp>

using SocketType = lb::tcp::HttpSession::SocketType;

namespace lb::tcp {

Connector::Connector(boost::asio::io_context& ctx, SelectorPtr selector)
    : ioc(ctx)
    , resolver(boost::asio::make_strand(ctx))
    , selector(selector)
{}


class LeastConnectionsCallbacks : public StateNotifier
{
public:
    LeastConnectionsCallbacks(Backend backend, SelectorPtr selector)
        : StateNotifier()
        , backend(std::move(backend))
        , selector(std::dynamic_pointer_cast<LeastConnectionsSelector>(selector))
    {}

    void OnConnect() override
    {
        selector->IncreaseConnectionCount(backend);
    }

    void OnDisconnect() override
    {
        selector->DecreaseConnectionCount(backend);
    }

    using StateNotifier::StateNotifier;
private:
    Backend backend;
    std::shared_ptr<LeastConnectionsSelector> selector;
};


class LeastResponseTimeCallbacks : public StateNotifier
{
public:
    using TimeType = decltype(std::chrono::high_resolution_clock::now());
public:
    LeastResponseTimeCallbacks(Backend backend, SelectorPtr selector)
        : StateNotifier()
        , backend(std::move(backend))
        , selector(std::dynamic_pointer_cast<LeastResponseTimeSelector>(selector))
    {}

    void OnRequestSent() override
    {
        response_begin = std::chrono::high_resolution_clock::now();
    }

    void OnResponseReceive() override
    {
        response_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<long, std::nano> duration = response_end - response_begin;
        selector->AddResponseTime(backend, duration.count());
    }

    using StateNotifier::StateNotifier;
private:
    Backend backend;
    std::shared_ptr<LeastResponseTimeSelector> selector;
    TimeType response_begin;
    TimeType response_end;
};

SessionPtr MakeSession(SelectorPtr& selector,
                       SocketType client_socket,
                       SocketType server_socket,
                       Backend backend)
{
    switch (selector->Type()) {
    case SelectorType::LEAST_CONNECTIONS:
    {
        return std::make_shared<HttpSession>(std::move(client_socket), std::move(server_socket),
                                             std::make_unique<LeastConnectionsCallbacks>(std::move(backend), selector));
    } break;

    case SelectorType::LEAST_RESPONSE_TIME:
    {
        return std::make_shared<HttpSession>(std::move(client_socket), std::move(server_socket),
                                             std::make_unique<LeastResponseTimeCallbacks>(std::move(backend), selector));
    } break;
    default:
        return std::make_shared<HttpSession>(std::move(client_socket), std::move(server_socket));
    }
}


void Connector::MakeAndRunSession(SocketType client_socket)
{
    DEBUG("In connector");
    Backend backend = selector->SelectBackend(client_socket.remote_endpoint());

    if (backend.IsIpEndpoint()) {
        DEBUG("Is ip endpoint");
        auto server_socket = std::make_shared<SocketType>(client_socket.get_executor());

        server_socket->async_connect(
            backend.AsEndpoint(),
            [this, server_socket, client_socket=std::move(client_socket), backend=std::move(backend)]
            (const boost::system::error_code& error) mutable
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
        auto server_socket = std::make_shared<SocketType>(client_socket.get_executor());

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
