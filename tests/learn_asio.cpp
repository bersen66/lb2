#include <gtest/gtest.h>
// Prints asio internal completion handler realations
// #define BOOST_ASIO_ENABLE_HANDLER_TRACKING
#include <boost/asio.hpp>
#include <type_traits>
#include <memory>


#if 1
TEST(LearnAsio, const_buffer)
{
    boost::asio::const_buffer const_buffer;
    ASSERT_EQ(const_buffer.size(), 0);

    std::string text = "Floppa - big russian cat";
    boost::asio::const_buffer buf{text.data(), text.size()};
    ASSERT_EQ(buf.size(), text.size());
    ASSERT_EQ(buf.data(), text.data());
}

TEST(LearnAsio, mutable_buffer)
{
    std::string text = "Hello, world";
    boost::asio::mutable_buffer mbuf(text.data(), text.size());
    ASSERT_EQ(mbuf.size(), text.size());
    ASSERT_EQ(mbuf.data(), text.data());
}

TEST(LearnAsio, streambuf)
{
    boost::asio::streambuf sb;
    ASSERT_EQ(sb.size(), 0);

    std::ostream out(&sb);
    out << "hello, world!";

    std::istream in(&sb);
    std::string text;
    std::getline(in, text);
    ASSERT_EQ(text, "hello, world!");
}

TEST(LearnAsio, buffer_func)
{
    std::string text = "Floppa big russian cat";
    boost::asio::mutable_buffer buf = boost::asio::buffer(text);
    static_assert(std::is_same_v<decltype(buf), boost::asio::mutable_buffer>, "bad buf type");
}

TEST(LearnAsio, timer)
{
    boost::asio::io_context ioc;
    boost::asio::steady_timer timer(ioc);

    timer.async_wait([](const boost::system::error_code& ec) {
        if (ec)
        std::cout << "Waiting finished!";
    });
}
#else
namespace EchoServer {

class EchoSession : public std::enable_shared_from_this<EchoSession> {
public:
    EchoSession(boost::asio::ip::tcp::socket socket)
        : socket(std::move(socket))
    {}

    void DoRead()
    {
        boost::asio::async_read_until(socket,
                                      boost::asio::dynamic_buffer(storage),
                                      "\n",
                                      [self=shared_from_this()] (boost::system::error_code ec, std::size_t length)
                                      {
                                        self->ReadHandler(ec, length);
                                      });
    }

    void ReadHandler(boost::system::error_code ec, std::size_t length)
    {
        if (ec || storage == "\n") {
            std::cout << ec.message() << std::endl;
            return;
        }
        DoWrite();
    }

    void DoWrite()
    {
        boost::asio::async_write(socket,
                                 boost::asio::dynamic_buffer(storage),
                                 [self=shared_from_this()](boost::system::error_code ec, std::size_t length) {
                                    self->WriteHandler(ec, length);
                                 });
    }

    void WriteHandler(boost::system::error_code ec, std::size_t length)
    {
        if (ec) {
            std::cout << ec.message() << std::endl;
            return;
        }
        storage.clear();
        DoRead();
    }

private:
    boost::asio::ip::tcp::socket socket;
    std::string storage;
};


void HandleAccept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket)
{
    if (ec) {
        std::cout << ec.value() << std::endl;
        return;
    }
    auto session = std::make_shared<EchoSession>(std::move(socket));
    session->DoRead();
}

void Serve(boost::asio::ip::tcp::acceptor& acceptor)
{
    acceptor.async_accept([&acceptor](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
        Serve(acceptor);
        HandleAccept(ec, std::move(socket));
    });
}

} // namespace EchoServer

TEST(EchoServer, run)
{
    boost::asio::io_context ioc;
    boost::asio::ip::tcp::acceptor acceptor(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 9999));

    EchoServer::Serve(acceptor);
    ioc.run();
}


namespace ConnectionServer {

class Connection final : public std::enable_shared_from_this<Connection>
{
public:
    Connection(boost::asio::ip::tcp::socket client,
               boost::asio::ip::tcp::socket server)
        : client_socket(std::move(client)),
          server_socket(std::move(server))
    {
        std::cout << "CONSTRUCTED\n";
    }

    void Run()
    {
        ClientRead();
        ServerRead();
    }

    void ClientRead()
    {
        boost::asio::async_read_until(
            client_socket,
            boost::asio::dynamic_buffer(client_buffer),
            "\n",
            [self=shared_from_this()] (const boost::system::error_code& ec, std::size_t length) {
                self->HandleClientRead(ec, length);
            });
    }

    void HandleClientRead(boost::system::error_code ec, std::size_t length)
    {
        if (ec) {
            std::cout << ec.message() << std::endl;
            Cancel();
            return;
        }
        SendToServer();
    }

    void SendToServer()
    {
        boost::asio::async_write(server_socket,
                                 boost::asio::dynamic_buffer(client_buffer),
                                 [self=shared_from_this()](boost::system::error_code ec, std::size_t length) {
                                    self->HandleSendToServer(ec, length);
                                 }
        );
    }

    void HandleSendToServer(boost::system::error_code ec, std::size_t length) {
        if (ec) {
            std::cout << ec.message() << std::endl;
            Cancel();
            return;
        }
        client_buffer.clear();
        ClientRead();
    }

    void ServerRead()
    {
        boost::asio::async_read_until(
            server_socket,
            boost::asio::dynamic_buffer(server_buffer),
            "\n",
            [self=shared_from_this()] (boost::system::error_code ec, std::size_t length) {
                self->HandleServerRead(ec, length);
            });
    }

    void HandleServerRead(boost::system::error_code ec, std::size_t length)
    {
        if (ec) {
            std::cout << ec.message() << std::endl;
            Cancel();
            return;
        }

        std::cout << "SERVER MESSAGE: " << server_buffer << std::endl;
        SendToClient();
    }

    void SendToClient()
    {
        boost::asio::async_write(client_socket,
                                 boost::asio::dynamic_buffer(server_buffer),
                                 [self=shared_from_this()](boost::system::error_code ec, std::size_t length){
                                    self->HandleSendToClient(ec, length);
                                 });
    }

    void HandleSendToClient(boost::system::error_code ec, std::size_t length) {
        if (ec) {
            Cancel();
        }
        server_buffer.clear();
        ServerRead();
    }

    void Cancel() {
        CancelClientOperations();
        CancelServerOperations();
    }


    void CancelClientOperations() {
        client_socket.cancel();
    }

    void CancelServerOperations() {
        server_socket.cancel();
    }
private:
    boost::asio::ip::tcp::socket client_socket;
    boost::asio::ip::tcp::socket server_socket;
    std::string client_buffer;
    std::string server_buffer;
};

void HandleAccept(boost::asio::io_context& ioc,
                  std::shared_ptr<boost::asio::ip::tcp::socket> client_socket,
                  boost::asio::ip::tcp::endpoint server_endpoint)
{
    auto server_socket = std::make_shared<boost::asio::ip::tcp::socket>(ioc);
    server_socket->async_connect(server_endpoint, [server_socket, client_socket](const boost::system::error_code& error){
        if (error) {
            std::cout << __LINE__ << " Error: " << error.message() << std::endl;
            return;
        }
        auto connection = std::make_shared<Connection>(std::move(*client_socket), std::move(*server_socket));
        connection->Run();
    });
}

void Serve(boost::asio::io_context& executor,
           boost::asio::ip::tcp::acceptor& acceptor,
           const boost::asio::ip::tcp::endpoint& endpoint)
{
    acceptor.async_accept(
        [&] (boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (ec) {
                std::cout << ec.message() << std::endl;
                return;
            }
            std::cout << __LINE__  << " Accepted" << std::endl;
            Serve(executor, acceptor, endpoint);
            auto client_socket = std::make_shared<boost::asio::ip::tcp::socket>(std::move(socket));
            HandleAccept(executor, client_socket, endpoint);
        }
    );
}

} // namespace ConnectionServer

TEST(ConnServer, run)
{
    std::cerr << "START\n";
    boost::asio::io_context ioc;
    boost::asio::ip::tcp::acceptor acceptor(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 5656));
    boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), 9999);
    ConnectionServer::Serve(ioc, acceptor, ep);
    ioc.run();
}
#endif