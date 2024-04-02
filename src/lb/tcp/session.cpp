#include <lb/tcp/session.hpp>
#include <iostream>
#include <lb/logging.hpp>

#include <atomic>

namespace lb {

namespace tcp {
Session::Session(boost::asio::ip::tcp::socket client,
                 boost::asio::ip::tcp::socket server)
    : client_socket(std::move(client))
    , server_socket(std::move(server))
    , id(generateId())
{
    DEBUG("Session id:{} constructed", id);
}

void Session::Run()
{
    ClientRead();
    ServerRead();
}


// Client->Server communication callbacks-chain
void Session::ClientRead()
{
    boost::asio::async_read_until(
        client_socket,
        boost::asio::dynamic_buffer(client_buffer),
        "\n",
        [self=shared_from_this()] (const boost::system::error_code& ec, std::size_t length) {
            self->HandleClientRead(ec, length);
        });
}

void Session::HandleClientRead(boost::system::error_code ec, std::size_t length)
{
    if (ec) {
        ERROR("sid:{} {}", id, ec.message());
        Cancel();
        return;
    }
    DEBUG("sid:{} client-msg:{}", id, client_buffer);
    SendToServer();
}

void Session::SendToServer()
{
    boost::asio::async_write(
        server_socket,
        boost::asio::dynamic_buffer(client_buffer),
        [self=shared_from_this()](boost::system::error_code ec, std::size_t length) {
           self->HandleSendToServer(ec, length);
        });
}


void Session::HandleSendToServer(boost::system::error_code ec, std::size_t length)
{
    if (ec) {
        ERROR("sid:{} {}", id, ec.message());
        Cancel();
        return;
    }
    client_buffer.clear();
    ClientRead();
}

// Server->Client communication callbacks-chain
void Session::ServerRead()
{
    boost::asio::async_read_until(
        server_socket,
        boost::asio::dynamic_buffer(server_buffer),
        "\n",
        [self=shared_from_this()] (boost::system::error_code ec, std::size_t length) {
            self->HandleServerRead(ec, length);
        });
}

void Session::HandleServerRead(boost::system::error_code ec, std::size_t length)
{
    if (ec) {
        ERROR("sid:{} {}", id, ec.message());
        Cancel();
        return;
    }
    DEBUG("sid:{} server-msg:{}", id, server_buffer);
    SendToClient();
}

void Session::SendToClient()
{
    boost::asio::async_write(client_socket,
        boost::asio::dynamic_buffer(server_buffer),
        [self=shared_from_this()](boost::system::error_code ec, std::size_t length){
           self->HandleSendToClient(ec, length);
        });
}

void Session::HandleSendToClient(boost::system::error_code ec, std::size_t length) {
    if (ec) {
        ERROR("sid:{} {}", id, ec.message());
        Cancel();
    }
    server_buffer.clear();
    ServerRead();
}

 // Cancel all unfinished async operartions on boths sockets
void Session::Cancel()
{
    client_socket.close();
    server_socket.close();
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