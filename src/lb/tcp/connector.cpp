#include <lb/logging.hpp>
#include <lb/tcp/connector.hpp>
#include <lb/tcp/session.hpp>
#include "connector.hpp"

namespace lb::tcp {

Connector::Connector(boost::asio::io_context& ctx)
    : ioc(ctx)
{}

void Connector::MakeAndRunSession(boost::asio::ip::tcp::socket client_socket)
{
    // TODO: selection of backend
    boost::asio::ip::tcp::endpoint server_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 9999);

    auto server_socket = std::make_shared<boost::asio::ip::tcp::socket>(ioc);

    server_socket->async_connect(server_endpoint,
        [this, server_socket, client_socket=std::move(client_socket)] (const boost::system::error_code& error) mutable {
        if (error) {
            ERROR("{}", error.message());
            return;
        }
        auto connection = std::make_shared<Session>(std::move(client_socket), std::move(*server_socket));
        connection->Run();
        });
}

} // namespace lb::tcp
