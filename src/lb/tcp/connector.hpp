#pragma once

#include <boost/asio.hpp>
#include <lb/tcp/session.hpp>
#include <memory>

namespace lb::tcp {

class Connector {
public:

public:
    explicit Connector(boost::asio::io_context& ctx);

    Connector(const Connector&) = delete;
    Connector& operator=(const Connector& other) = delete;

    void MakeAndRunSession(boost::asio::ip::tcp::socket client);

private:
    boost::asio::io_context& ioc;
};

} // namespace lb::tcp