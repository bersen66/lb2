#pragma once

#include <memory>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/thread/mutex.hpp>

namespace lb {

namespace tcp {

struct BasicSession {
    virtual void Run() = 0;
    virtual void Cancel() = 0;
    virtual ~BasicSession() = default;
};

using SessionPtr = std::shared_ptr<BasicSession>;

class Session final : public BasicSession,
                      public std::enable_shared_from_this<Session> {
public:
    using IdType = std::size_t;
    static constexpr inline std::size_t BUFFER_SIZE = 4096;
public:
    Session(boost::asio::ip::tcp::socket client,
            boost::asio::ip::tcp::socket server);

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    ~Session() noexcept;

    void Run() override;

    // Cancel all unfinished async operartions on boths sockets
    void Cancel() override;

    const IdType& Id() const;
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
    static IdType generateId();
private:
    boost::asio::ip::tcp::socket client_socket;
    boost::asio::ip::tcp::socket server_socket;
    boost::asio::streambuf cb;
    boost::asio::streambuf sb;
    IdType id;
    boost::beast::http::request<boost::beast::http::string_body> cr;
    boost::beast::http::response<boost::beast::http::string_body> sr;
    boost::mutex mutex;
};

} // namespace tcp



} // namespace lb