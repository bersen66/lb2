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

class HttpSession : public BasicSession,
                    public std::enable_shared_from_this<HttpSession> {
public:
    using IdType = std::size_t;
    static constexpr const std::size_t BUFFER_SIZE = 4096;
public:
    HttpSession(boost::asio::ip::tcp::socket client,
            boost::asio::ip::tcp::socket server);

    HttpSession(const HttpSession&) = delete;
    HttpSession& operator=(const HttpSession&) = delete;
    ~HttpSession() noexcept;

    void Run() override;

    // Cancel all unfinished async operartions on boths sockets
    void Cancel() override;

    const IdType& Id() const;
protected:

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
protected:
    static IdType generateId();
protected:
    boost::asio::ip::tcp::socket client_socket;
    boost::asio::ip::tcp::socket server_socket;
    boost::asio::streambuf cb;
    boost::asio::streambuf sb;
    IdType id;
    boost::beast::http::request<boost::beast::http::string_body> cr;
    boost::beast::http::response<boost::beast::http::string_body> sr;
    boost::mutex mutex;
};


class LeastConnectionsHttpSession : public BasicSession,
                                    public std::enable_shared_from_this<LeastConnectionsHttpSession> {
public:
    using IdType = std::size_t;
    static constexpr const std::size_t BUFFER_SIZE = 4096;
    using SelectorType = std::shared_ptr<LeastConnectionsSelector>;
public:
    LeastConnectionsHttpSession(boost::asio::ip::tcp::socket client,
                                boost::asio::ip::tcp::socket server,
                                SelectorType selector, Backend server_backend);

    LeastConnectionsHttpSession(const LeastConnectionsHttpSession&) = delete;
    LeastConnectionsHttpSession& operator=(const LeastConnectionsHttpSession&) = delete;
    ~LeastConnectionsHttpSession() noexcept;

    void Run() override;

    // Cancel all unfinished async operartions on boths sockets
    void Cancel() override;

    const IdType& Id() const;
protected:

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
protected:
    static IdType generateId();
protected:
    boost::asio::ip::tcp::socket client_socket;
    boost::asio::ip::tcp::socket server_socket;
    boost::asio::streambuf cb;
    boost::asio::streambuf sb;
    IdType id;
    boost::beast::http::request<boost::beast::http::string_body> cr;
    boost::beast::http::response<boost::beast::http::string_body> sr;
    boost::mutex mutex;
    SelectorType lc_selector;
    Backend server_backend;
};



class LeastResponseTimeHttpSession : public BasicSession,
                                    public std::enable_shared_from_this<LeastResponseTimeHttpSession> {
public:
    using IdType = std::size_t;
    static constexpr const std::size_t BUFFER_SIZE = 4096;
    using SelectorType = std::shared_ptr<LeastResponseTimeSelector>;
    using TimeType = boost::posix_time::ptime;
public:
    LeastResponseTimeHttpSession(boost::asio::ip::tcp::socket client,
                                boost::asio::ip::tcp::socket server,
                                SelectorType selector, Backend server_backend);

   LeastResponseTimeHttpSession(const LeastResponseTimeHttpSession&) = delete;
   LeastResponseTimeHttpSession& operator=(const LeastResponseTimeHttpSession&) = delete;
    ~LeastResponseTimeHttpSession() noexcept;

    void Run() override;

    // Cancel all unfinished async operartions on boths sockets
    void Cancel() override;

    const IdType& Id() const;
protected:

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
protected:
    static IdType generateId();
protected:
    boost::asio::ip::tcp::socket client_socket;
    boost::asio::ip::tcp::socket server_socket;
    boost::asio::streambuf cb;
    boost::asio::streambuf sb;
    IdType id;
    boost::beast::http::request<boost::beast::http::string_body> cr;
    boost::beast::http::response<boost::beast::http::string_body> sr;
    boost::mutex mutex;
    SelectorType lrt_selector;
    Backend server_backend;
    TimeType response_begin;
    TimeType response_end;
};
} // namespace tcp



} // namespace lb