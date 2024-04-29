#include <lb/tcp/session.hpp>
#include <iostream>
#include <lb/logging.hpp>
#include <boost/beast.hpp>

#include <atomic>

namespace lb {

namespace tcp {

// ================================= HttpSession =================================
HttpSession::HttpSession(boost::asio::ip::tcp::socket client,
                 boost::asio::ip::tcp::socket server)
    : BasicSession()
    , client_socket(std::move(client))
    , server_socket(std::move(server))
    , id(generateId())
{
    cb.prepare(BUFFER_SIZE);
    sb.prepare(BUFFER_SIZE);
    DEBUG("HttpSession id:{} constructed", id);
}

void HttpSession::Run()
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
void HttpSession::ClientRead()
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

void HttpSession::HandleClientRead(boost::system::error_code ec, std::size_t length)
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

void HttpSession::SendToServer()
{
    boost::beast::http::async_write(
        server_socket,
        cr,
        [self=shared_from_this()](boost::system::error_code ec, std::size_t length) {
           self->HandleSendToServer(ec, length);
        });
}


void HttpSession::HandleSendToServer(boost::system::error_code ec, std::size_t length)
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
void HttpSession::ServerRead()
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

void HttpSession::HandleServerRead(boost::system::error_code ec, std::size_t length)
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

void HttpSession::SendToClient()
{
    boost::beast::http::async_write(client_socket, sr,
    [self=shared_from_this()](boost::system::error_code ec, std::size_t length){
        self->HandleSendToClient(ec, length);
    });
}

void HttpSession::HandleSendToClient(boost::system::error_code ec, std::size_t length) {
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
void HttpSession::Cancel()
{
    CloseSocket(client_socket);
    CloseSocket(server_socket);
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


// ================================= LeastConnectionsHttpSession =================================
LeastConnectionsHttpSession::LeastConnectionsHttpSession(boost::asio::ip::tcp::socket client,
                                                         boost::asio::ip::tcp::socket server,
                                                         SelectorType selector, Backend server_backend)
    : BasicSession()
    , client_socket(std::move(client))
    , server_socket(std::move(server))
    , id(generateId())
    , lc_selector(std::move(selector))
    , server_backend(std::move(server_backend))
{
    cb.prepare(BUFFER_SIZE);
    sb.prepare(BUFFER_SIZE);
    DEBUG("LeastConnectionsHttpSession id:{} constructed", id);
    // selector->IncreaseConnectionCount(server_backend);
}

void LeastConnectionsHttpSession::Run()
{
    ClientRead();
    ServerRead();
}


// Client->Server communication callbacks-chain
void LeastConnectionsHttpSession::ClientRead()
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

void LeastConnectionsHttpSession::HandleClientRead(boost::system::error_code ec, std::size_t length)
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

void LeastConnectionsHttpSession::SendToServer()
{
    boost::beast::http::async_write(
        server_socket,
        cr,
        [self=shared_from_this()](boost::system::error_code ec, std::size_t length) {
           self->HandleSendToServer(ec, length);
        });
}


void LeastConnectionsHttpSession::HandleSendToServer(boost::system::error_code ec, std::size_t length)
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
void LeastConnectionsHttpSession::ServerRead()
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

void LeastConnectionsHttpSession::HandleServerRead(boost::system::error_code ec, std::size_t length)
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

void LeastConnectionsHttpSession::SendToClient()
{
    boost::beast::http::async_write(client_socket, sr,
    [self=shared_from_this()](boost::system::error_code ec, std::size_t length){
        self->HandleSendToClient(ec, length);
    });
}

void LeastConnectionsHttpSession::HandleSendToClient(boost::system::error_code ec, std::size_t length) {
    if (ec) {
        if (NeedErrorLogging(ec)) {
            SERROR("sid:{} {}", id, ec.message());
        }
        Cancel();
    }
    ServerRead();
}

void LeastConnectionsHttpSession::Cancel()
{
    CloseSocket(client_socket);
    CloseSocket(server_socket);
    lc_selector->DecreaseConnectionCount(server_backend);
}


LeastConnectionsHttpSession::IdType LeastConnectionsHttpSession::generateId()
{
    static std::atomic<IdType> id = 0;
    LeastConnectionsHttpSession::IdType result = id.fetch_add(1, std::memory_order_relaxed);
    return result;
}

const LeastConnectionsHttpSession::IdType& LeastConnectionsHttpSession::Id() const
{
    return id;
}

LeastConnectionsHttpSession::~LeastConnectionsHttpSession()
{
    Cancel();
    DEBUG("LeastConnectionsHttpSession id:{} destructed", id);
}


// ================================= LeastResponseTimeHttpSession =================================
LeastResponseTimeHttpSession::LeastResponseTimeHttpSession(boost::asio::ip::tcp::socket client,
                                                           boost::asio::ip::tcp::socket server,
                                                           SelectorType selector, Backend server_backend)
    : BasicSession()
    , client_socket(std::move(client))
    , server_socket(std::move(server))
    , id(generateId())
    , lrt_selector(std::move(selector))
    , server_backend(std::move(server_backend))
{
    cb.prepare(BUFFER_SIZE);
    sb.prepare(BUFFER_SIZE);
    DEBUG("LeastConnectionsHttpSession id:{} constructed", id);
    // selector->IncreaseConnectionCount(server_backend);
}

void LeastResponseTimeHttpSession::Run()
{
    ClientRead();
    ServerRead();
}


// Client->Server communication callbacks-chain
void LeastResponseTimeHttpSession::ClientRead()
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

void LeastResponseTimeHttpSession::HandleClientRead(boost::system::error_code ec, std::size_t length)
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

void LeastResponseTimeHttpSession::SendToServer()
{
    boost::beast::http::async_write(
        server_socket,
        cr,
        [self=shared_from_this()](boost::system::error_code ec, std::size_t length) {
           self->HandleSendToServer(ec, length);
        });
}


void LeastResponseTimeHttpSession::HandleSendToServer(boost::system::error_code ec, std::size_t length)
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
void LeastResponseTimeHttpSession::ServerRead()
{
    sr.clear();
    response_begin = boost::posix_time::microsec_clock::local_time();
    boost::beast::http::async_read(
        server_socket,
        sb,
        sr,
        [self=shared_from_this()] (const boost::system::error_code& ec, std::size_t length) {
            self->HandleServerRead(ec, length);
        }
    );
}

void LeastResponseTimeHttpSession::HandleServerRead(boost::system::error_code ec, std::size_t length)
{
    if (ec) {
        if (NeedErrorLogging(ec)) {
            SERROR("sid:{} {}", id, ec.message());
        }
        Cancel();
        return;
    }
    response_end = boost::posix_time::microsec_clock::local_time();
    long response_time = (response_end - response_begin).total_microseconds();
    lrt_selector->AddResponseTime(server_backend, response_time);
    SendToClient();
}

void LeastResponseTimeHttpSession::SendToClient()
{
    boost::beast::http::async_write(client_socket, sr,
    [self=shared_from_this()](boost::system::error_code ec, std::size_t length){
        self->HandleSendToClient(ec, length);
    });
}

void LeastResponseTimeHttpSession::HandleSendToClient(boost::system::error_code ec, std::size_t length) {
    if (ec) {
        if (NeedErrorLogging(ec)) {
            SERROR("sid:{} {}", id, ec.message());
        }
        Cancel();
    }
    ServerRead();
}

void LeastResponseTimeHttpSession::Cancel()
{
    CloseSocket(client_socket);
    CloseSocket(server_socket);
}


LeastResponseTimeHttpSession::IdType LeastResponseTimeHttpSession::generateId()
{
    static std::atomic<IdType> id = 0;
    LeastResponseTimeHttpSession::IdType result = id.fetch_add(1, std::memory_order_relaxed);
    return result;
}

const LeastResponseTimeHttpSession::IdType& LeastResponseTimeHttpSession::Id() const
{
    return id;
}

LeastResponseTimeHttpSession::~LeastResponseTimeHttpSession()
{
    Cancel();
    DEBUG("LeastConnectionsHttpSession id:{} destructed", id);
}
} // namespace tcp

} // namespace lb