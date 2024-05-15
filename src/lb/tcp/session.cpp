#include <lb/tcp/session.hpp>
#include <iostream>
#include <lb/logging.hpp>
#include <boost/beast.hpp>

#include <atomic>



namespace lb {

namespace tcp {

HttpSession::HttpSession(SocketType client_socket, SocketType server_socket, VisitorPtr visitor)
    : BasicSession()
    , client_stream_(std::move(client_socket))
    , server_stream_(std::move(server_socket))
    , id(generateId())
    , visitor_(std::move(visitor))
{
    DEBUG("HttpSession id:{} created", id);
}

void HttpSession::Run()
{
    if (visitor_) {
        visitor_->OnConnect();
    }
    ClientRead();
}

bool NeedErrorLogging(const HttpSession::ErrorCode& ec)
{
    return ec != boost::asio::error::eof
        && ec != boost::beast::http::error::end_of_stream
        && ec != boost::asio::error::operation_aborted;
}


void HttpSession::ClientRead()
{
    namespace http = boost::beast::http;

    client_buffer_.clear();
    server_buffer_.clear();
    client_request_.clear();
    server_response_.clear();

    http::async_read(
        client_stream_,
        client_buffer_,
        client_request_,
        [self=shared_from_this()](ErrorCode ec, std::size_t length){
            self->HandleClientRead(ec, length);
        }
    );
}

void HttpSession::HandleClientRead(ErrorCode ec, std::size_t length)
{
    if (ec) {
        if (NeedErrorLogging(ec)) {
            SERROR("sid:{} {}", id, ec.message());
        }
        Cancel();
        return;
    }

    if (visitor_) {
        visitor_->OnRequestReceive();
    }
    SendToServer();
}

void HttpSession::SendToServer()
{
    namespace http = boost::beast::http;

    http::async_write(
        server_stream_,
        client_request_,
        [self=shared_from_this()](ErrorCode ec, std::size_t length){
            self->HandleSendToServer(ec, length);
        });
}


void HttpSession::HandleSendToServer(ErrorCode ec, std::size_t length)
{
    if (ec) {
        if (NeedErrorLogging(ec)) {
            SERROR("sid:{} {}", id, ec.message());
        }
        Cancel();
        return;
    }
    DEBUG("sid: {} sent to server", id);
    if (visitor_) {
        visitor_->OnRequestSent();
    }
    ServerRead();
}


void HttpSession::ServerRead()
{
    namespace http = boost::beast::http;
    http::async_read(
        server_stream_,
        server_buffer_,
        server_response_,
        [self=shared_from_this()](ErrorCode ec, std::size_t length){
            self->HandleServerRead(ec, length);
        }
    );
}

void HttpSession::HandleServerRead(ErrorCode ec, std::size_t length)
{
    if (ec) {
        if (NeedErrorLogging(ec)) {
            SERROR("sid:{} {}", id, ec.message());
        }
        Cancel();
        return;
    }
    if (visitor_) {
        visitor_->OnResponseReceive();
    }
    SendToClient();
}

void HttpSession::SendToClient()
{
    namespace http = boost::beast::http;
    http::async_write(
        client_stream_,
        server_response_,
        [self=shared_from_this()](ErrorCode ec, std::size_t length){
            self->HandleSendToClient(ec, length);
        }
    );
}

void HttpSession::HandleSendToClient(ErrorCode ec, std::size_t length) {
    if (ec) {
        if (NeedErrorLogging(ec)) {
            SERROR("sid:{} {}", id, ec.message());
        }
        Cancel();
        return;
    }
    if (visitor_) {
        visitor_->OnResponseSent();
    }
    ClientRead();
}

void CloseSocket(HttpSession::SocketType& socket)
{
    HttpSession::ErrorCode ec;
    if (socket.is_open()) {
        socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        if (!ec) {
            socket.close();
        } else if (ec != boost::asio::error::not_connected && ec !=  boost::asio::error::bad_descriptor) {
            SERROR("{}", ec.message());
            return;
        }
    }
}


void HttpSession::Cancel()
{
    CloseSocket(client_stream_.socket());
    CloseSocket(server_stream_.socket());
    if (visitor_) {
        visitor_->OnDisconnect();
    }
}

HttpSession::~HttpSession()
{
    Cancel();
}

HttpSession::IdType HttpSession::generateId()
{
    static std::atomic<IdType> id = 0;
    HttpSession::IdType result = id.fetch_add(1, std::memory_order_relaxed);
    return result;
}

const HttpSession::IdType& HttpSession::Id() const
{
    return id;
}

} // namespace tcp

} // namespace lb