#pragma once

#include <memory>
#include <boost/asio.hpp>

namespace lb {

namespace tcp {

class Session final : public std::enable_shared_from_this<Session> {
public:
    using IdType = std::size_t;
public:
    Session(boost::asio::ip::tcp::socket client,
            boost::asio::ip::tcp::socket server);

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    ~Session() noexcept;

    void Run();

    // Cancel all unfinished async operartions on boths sockets
    void Cancel();

    static IdType generateId();
private:

    // Client->Server communication callbacks-chain
    void ClientRead();
    void HandleClientRead(boost::system::error_code ec, std::size_t length);
    void SendToServer();
    void HandleSendToServer(boost::system::error_code ec, std::size_t length);

    // Server->Client communication callbacks-chain
    void ServerRead();
    void HandleServerRead(boost::system::error_code ec, std::size_t length);
    void SendToClient();
    void HandleSendToClient(boost::system::error_code ec, std::size_t length);

private:
    boost::asio::ip::tcp::socket client_socket;
    boost::asio::ip::tcp::socket server_socket;
    std::string client_buffer;
    std::string server_buffer;
    IdType id;
};

} // namespace tcp


} // namespace lb