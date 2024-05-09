#pragma once

#include <memory>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/thread/mutex.hpp>
#include <lb/tcp/selectors.hpp>

namespace lb {

namespace tcp {

struct BasicSession {
    virtual void Run() = 0;
    virtual void Cancel() = 0;
    virtual ~BasicSession() = default;
};

using SessionPtr = std::shared_ptr<BasicSession>;

struct StateNotifier {
    virtual void OnConnect() {};
    virtual void OnDisconnect() {};
    virtual void OnResponseReceive() {};
    virtual void OnResponseSent() {};
    virtual void OnRequestReceive() {};
    virtual void OnRequestSent() {};
    virtual ~StateNotifier() = default;
};

using VisitorPtr = std::unique_ptr<StateNotifier>;

class HttpSession : public BasicSession,
                    public std::enable_shared_from_this<HttpSession> {
public:
    using IdType        = std::size_t;
    using SocketType    = boost::asio::ip::tcp::socket;
    using EndpointType  = boost::asio::ip::tcp::endpoint;
    using TcpStream     = boost::beast::tcp_stream;
    using BufferType    = boost::beast::flat_buffer;
    using RequestType   = boost::beast::http::request<boost::beast::http::string_body>;
    using ResponseType  = boost::beast::http::response<boost::beast::http::string_body>;
    using ErrorCode     = boost::system::error_code;
public:
    HttpSession(SocketType client_socket, SocketType server_socket, VisitorPtr visitor=nullptr);

    HttpSession(const HttpSession&) = delete;
    HttpSession& operator=(const HttpSession&) = delete;
    ~HttpSession() noexcept;

    void Run() override;

    void Cancel() override;

    const IdType& Id() const;
protected:
    void ClientRead();
    void HandleClientRead(ErrorCode ec, std::size_t length);
    void SendToServer();
    void HandleSendToServer(ErrorCode ec, std::size_t length);
    void ServerRead();
    void HandleServerRead(ErrorCode ec, std::size_t length);
    void SendToClient();
    void HandleSendToClient(ErrorCode ec, std::size_t length);
protected:
    static IdType generateId();
protected:
    TcpStream client_stream_;
    TcpStream server_stream_;
    BufferType client_buffer_;
    BufferType server_buffer_;
    RequestType client_request_;
    ResponseType server_response_;
    IdType id;
    VisitorPtr visitor_;
};

} // namespace tcp



} // namespace lb