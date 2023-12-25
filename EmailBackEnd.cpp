
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/json.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include "Logger.h"
#include "boost/mysql.hpp"
#include "DBManager.hpp"
#include "boost/filesystem.hpp"
#include "boost/filesystem/fstream.hpp"
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
DBManager* m_dbmanger;

namespace my_program_state
{
    std::size_t
        request_count()
    {
        static std::size_t count = 0;
        return ++count;
    }
    
    std::time_t
        now()
    {
        return std::time(0);
    }
  
}

class http_connection : public std::enable_shared_from_this<http_connection>
{
public:
    http_connection(tcp::socket socket)
        : socket_(std::move(socket))
    {
    }

   
    // Initiate the asynchronous operations associated with the connection.
    void
        start()
    {
        
        read_request();
        check_deadline();
    }

private:
    // The socket for the currently connected client.
    tcp::socket socket_;

    // The buffer for performing reads.
    beast::flat_buffer buffer_{ 8192 };

    // The request message.
    http::request<http::dynamic_body> request_;

    // The response message.
    http::response<http::dynamic_body> response_;

    // The timer for putting a deadline on connection processing.
    net::basic_waitable_timer<std::chrono::steady_clock> deadline_{
        socket_.get_executor(), std::chrono::seconds(60) };
   
    // Asynchronously receive a complete request message.
    void
        read_request()
    {
        auto self = shared_from_this();

        http::async_read(
            socket_,
            buffer_,
            request_,
            [self](beast::error_code ec,
                std::size_t bytes_transferred)
            {
                boost::ignore_unused(bytes_transferred);
                if (!ec)
                    self->process_request();
            });
    }

    // Determine what needs to be done with the request message.
    void
        process_request()
    {
        response_.version(request_.version());
        response_.keep_alive(false);
        
        switch (request_.method())
        {
        case http::verb::get:
            response_.result(http::status::ok);
            response_.set(http::field::server, "Beast");
            create_response_get();
            break;
        case http::verb::post:
            response_.result(http::status::ok);
            response_.set(http::field::server, "Beast");
            create_response_post();
            break;

        default:
            // We return responses indicating an error if
            // we do not recognize the request method.
            response_.result(http::status::bad_request);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body())
                << "Invalid request-method '"
                << std::string(request_.method_string())
                << "'";
            break;
        }

        write_response();
    }

    // Construct a response message based on the program state.
    void create_response_get()
    {
        beast::error_code ec;
        http::file_body::value_type body;
        
        body.open(boost::filesystem::current_path().string().c_str(), beast::file_mode::scan, ec);

        // Handle the case where the file doesn't exist
        /*if (ec == beast::errc::no_such_file_or_directory)
            return send(not_found(req.target()));*/

        // Handle an unknown error
        /*if (ec)
            return send(server_error(ec.message()));*/
        if (request_.target() == "/register")
        {
            response_.set(http::field::content_type, "text/html");
          
          
            string path = (boost::filesystem::current_path().string() + "\\index.html").c_str();
            if (!boost::filesystem::is_regular_file(path))
                return;

            boost::filesystem::fstream file(path, std::ios::in | std::ios::binary);
            if (!file.is_open())
                return ;

            // Read contents
            std::string content{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };

            // Close the file
            file.close();

            response_.set(http::field::content_type, "text/html");
            beast::ostream(response_.body()) << content;
        }
        else
        {
            response_.result(http::status::not_found);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body()) << "File not found\r\n";
        }
    }
    void create_response_post()
    {

        if (request_.target() == "/register")
        {
            boost::json::error_code ec;
            boost::json::value jv = boost::json::parse(beast::buffers_to_string(request_.body().data()), ec);
            LOG("BODY")
           
            boost::json::object const& obj = jv.as_object();
          
            LOG(obj)
                m_dbmanger->RegisterUser(User(obj.at("FirstName").as_string().data(),
                    obj.at("LastName").as_string().data(),
                    obj.at("Email").as_string().data()));

            response_.set(http::field::content_type, "text/html");
            response_.result(http::status::accepted);
            
            beast::ostream(response_.body())
                << "<html>\n"
                << "<head><title>Current time</title></head>\n"
                << "<body>\n"
                << "<h1>REGISTERED</h1>\n"
                << beast::buffers_to_string(request_.body().data())
                << "</body>\n"
                << "</html>\n";
        }
        else if (request_.target() == "/registered")
        {
          

            response_.set(http::field::content_type, "text/html");
            response_.result(http::status::accepted);
        
            beast::ostream(response_.body())
                << "<html>\n"
                << "<head><title>Current time</title></head>\n"
                << "<body>\n"
                << "<h1>REGISTERED</h1>\n"
                << "</body>\n"
                << "</html>\n";
        }
        else
        {
            response_.result(http::status::not_found);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body()) << "File not found\r\n";
        }
    }
    // Asynchronously transmit the response message.
    void
        write_response()
    {
        auto self = shared_from_this();

        response_.set(http::field::content_length,boost::core::string_view(std::to_string( response_.body().size())));

        http::async_write(
            socket_,
            response_,
            [self](beast::error_code ec, std::size_t)
            {
                self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                self->deadline_.cancel();
            });
    }

    // Check whether we have spent enough time on this connection.
    void
        check_deadline()
    {
        auto self = shared_from_this();

        deadline_.async_wait(
            [self](beast::error_code ec)
            {
                if (!ec)
                {
                    // Close socket to cancel any outstanding operation.
                    self->socket_.close(ec);
                }
            });
    }
};

// "Loop" forever accepting new connections.
void
http_server(tcp::acceptor& acceptor, tcp::socket& socket)
{
    acceptor.async_accept(socket,
        [&](beast::error_code ec)
        {
            if (!ec)
                std::make_shared<http_connection>(std::move(socket))->start();
            http_server(acceptor, socket);
        });
}

int
main(int argc, char* argv[])
{
    try
    {
        // Check command line arguments.
        if (argc != 3)
        {
            std::cerr << "Usage: " << argv[0] << " <address> <port>\n";
            std::cerr << "  For IPv4, try:\n";
            std::cerr << "    receiver 0.0.0.0 80\n";
            std::cerr << "  For IPv6, try:\n";
            std::cerr << "    receiver 0::0 80\n";
            return EXIT_FAILURE;
        }
     

        auto const address = net::ip::make_address(argv[1]);
        unsigned short port = static_cast<unsigned short>(std::atoi(argv[2]));
       
        net::io_context ioc{ 1 };

        tcp::acceptor acceptor{ ioc, {address, port} };
        tcp::socket socket{ ioc };
        http_server(acceptor, socket);
        m_dbmanger = new DBManager("MYSQLHOST", "3306", "admin", "PassWord");
        m_dbmanger->ConnectToDB();
      
      
      
        
        ioc.run();
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        
    }
}

