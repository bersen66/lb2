#include <lb/tcp/session.hpp>
#include <iostream>
#include <lb/logging.hpp>
#include <boost/beast.hpp>

#include <atomic>

namespace lb {

namespace tcp {
Session::Session(boost::asio::ip::tcp::socket client,
                 boost::asio::ip::tcp::socket server)
    : BasicSession()
    , client_socket(std::move(client))
    , server_socket(std::move(server))
    , id(generateId())
{
    cb.prepare(BUFFER_SIZE);
    sb.prepare(BUFFER_SIZE);
    DEBUG("Session id:{} constructed", id);
}

void Session::Run()
{
    ClientRead();
    ServerRead();
}

bool NeedErrorLogging(const boost::system::error_code& ec)
{
    return ec != boost::asio::error::eof
        && ec != boost::beast::http::error::end_of_stream
        && ec != boost::asio::error::operation_aborted;
}

// Client->Server communication callbacks-chain
void Session::ClientRead()
{
    cr.clear();
    boost::beast::http::async_read(
        client_socket,
        cb,
        cr,
        [self=shared_from_this()] (const boost::system::error_code& ec, std::size_t length) {
            self->HandleClientRead(ec, length);
        }
    );
}

void Session::HandleClientRead(boost::system::error_code ec, std::size_t length)
{
    if (ec) {
        if (NeedErrorLogging(ec)) {
            SERROR("sid:{} {}", id, ec.message());
        }
        Cancel();
        return;
    }
    //DEBUG("sid:{} client-msg:{}", id, client_buffer);
    SendToServer();
}

void Session::SendToServer()
{
    boost::beast::http::async_write(
        server_socket,
        cr,
        [self=shared_from_this()](boost::system::error_code ec, std::size_t length) {
           self->HandleSendToServer(ec, length);
        });
}


void Session::HandleSendToServer(boost::system::error_code ec, std::size_t length)
{
    if (ec) {
        if (NeedErrorLogging(ec)) {
            SERROR("sid:{} {}", id, ec.message());
        }
        Cancel();
        return;
    }

    ClientRead();
}

// Server->Client communication callbacks-chain
void Session::ServerRead()
{
    sr.clear();
    boost::beast::http::async_read(
        server_socket,
        sb,
        sr,
        [self=shared_from_this()] (const boost::system::error_code& ec, std::size_t length) {
            self->HandleServerRead(ec, length);
        }
    );
}

void Session::HandleServerRead(boost::system::error_code ec, std::size_t length)
{
    if (ec) {
        if (NeedErrorLogging(ec)) {
            SERROR("sid:{} {}", id, ec.message());
        }
        Cancel();
        return;
    }
    SendToClient();
}

void Session::SendToClient()
{
    boost::beast::http::async_write(client_socket, sr,
    [self=shared_from_this()](boost::system::error_code ec, std::size_t length){
        self->HandleSendToClient(ec, length);
    });
}

void Session::HandleSendToClient(boost::system::error_code ec, std::size_t length) {
    if (ec) {
        if (NeedErrorLogging(ec)) {
            SERROR("sid:{} {}", id, ec.message());
        }
        Cancel();
    }
    ServerRead();
}

void CloseSocket(boost::asio::ip::tcp::socket& socket)
{
    boost::system::error_code ec;
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

 // Cancel all unfinished async operartions on boths sockets
void Session::Cancel()
{
    CloseSocket(client_socket);
    CloseSocket(server_socket);
}

Session::~Session()
{
    Cancel();
}

Session::IdType Session::generateId()
{
    static std::atomic<IdType> id = 0;
    Session::IdType result = id.fetch_add(1, std::memory_order_relaxed);
    return result;
}

const Session::IdType& Session::Id() const
{
    return id;
}

} // namespace tcp

} // namespace lb