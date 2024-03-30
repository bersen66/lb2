#pragma once

#include <boost/asio.hpp>

namespace lb {

namespace tcp {

// Accept incoming tcp-connections
class Acceptor{
public:
    using PortType = unsigned;

    struct Configuration {
        PortType port;
        bool useIpV6 = false;
    };

public:
    Acceptor(boost::asio::io_context& io_ctx, PortType port, bool useIpV6=false);

    Acceptor(boost::asio::io_context& io_ctx, const Configuration& config);

    void Run();

private:

    // Acception callbacks
    void DoAccept();

private:
    boost::asio::io_context& io_context;
    boost::asio::ip::tcp::acceptor acceptor;
};

} // namespace tcp

} // namespace lb