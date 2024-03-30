#include <lb/tcp/acceptor.hpp>
#include <spdlog/spdlog.h>

namespace asio = boost::asio;
namespace sys = boost::system;

namespace lb {

namespace tcp {

namespace {

asio::ip::tcp::endpoint AcceptorEndpoint(Acceptor::PortType port, bool useIpV6)
{
    if (useIpV6) {
        return asio::ip::tcp::endpoint(asio::ip::tcp::v6(), port);
    }
    return asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port);
}

} // anonimous namespace


Acceptor::Acceptor(asio::io_context& io_ctx, PortType port, bool useIpV6)
    : io_context(io_ctx)
    , acceptor(io_ctx, AcceptorEndpoint(port, useIpV6))
    , logger(spdlog::get("multi-sink"))
{}

Acceptor::Acceptor(asio::io_context& io_ctx, const Acceptor::Configuration& config)
    : io_context(io_ctx)
    , acceptor(io_ctx, AcceptorEndpoint(config.port, config.useIpV6))
    , logger(spdlog::get("multi-sink"))
{}

void Acceptor::Run()
{
    DoAccept();
}

void Acceptor::DoAccept()
{
    acceptor.async_accept(
        [&](const sys::error_code& ec, asio::ip::tcp::socket client_socket){
            if (ec) {
                SPDLOG_LOGGER_ERROR(logger, "Acceptor error: {}", ec.message());
                return;
            }
            DoAccept();
            SPDLOG_LOGGER_INFO(logger, "Accepted {}:{}",
                                       client_socket.local_endpoint().address().to_string(),
                                       client_socket.local_endpoint().port());
        });
}

} // namespace tcp

} // namespace lb