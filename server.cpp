//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "server.hpp"
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <memory>

namespace http
{
namespace server
{

class echo_handler: public registered_handler
{
public:

  echo_handler() :
      registered_handler("/echo")
  {
  }

  void handle_request(const request& req, reply& rep) const
  {
    std::stringstream response;
    response << "<html><body><table>" << std::endl;
    response << "<tr><td colspan='2'><bold>Service</bold></td></tr>" << std::endl;
    response << "<tr><td>Http Version</td><td>" << req.http_version_major << "." << req.http_version_minor << "</td></tr>" << std::endl;
    response << "<tr><td>Http Method</td><td>" << req.method << "</td></tr>" << std::endl;
    response << "<tr><td colspan='2'><bold>Request</bold></td></tr>" << std::endl;
    response << "<tr><td>URI</td><td>" << req.uri << "</td></tr>" << std::endl;
    response << "<tr><td colspan='2'><bold>Headers</bold></td></tr>" << std::endl;
    for(const Headers::value_type& h : req.headers)
    {
      response << "<tr><td>" << h.first << "</td><td>" << h.second << "</td></tr>" << std::endl;
    }
    response << "<tr><td colspan='2'><bold>Parameters</bold></td></tr>";
    for(auto p : req.parameters)
    {
      response << "<tr><td>" << p.first << "</td><td>" << p.second << "</td></tr>" << std::endl;
    }
    response << "</table>" << std::endl;
    response << "<b>Post Data:</b>" << std::endl;
    response << "<br/>------BEGIN POST DATA------<br/>" << std::endl;
    response << "<pre><![CDATA[" << req.post << "]]></pre>" << std::endl;
    response << "<br/>------END POST DATA------<br/>" << std::endl;
    response << "</body></html>";
    rep.content = response.str();
    rep.status = reply::ok;
    rep.headers.insert(std::make_pair(std::string("Content-Type"), mime_types::extension_to_type("html")));
  }

  const char* usage_info() const
  {
    return "Echo's back all the headers, parameters and post data sent to the request";
  }
};

class status_handler : public registered_handler {
public:

  status_handler(const http::server::server& server) : registered_handler("/server_status"), server(server) {
  }

  void handle_request(const request& req, reply& rep) const {
    std::stringstream response;
    response << "<html><body>" << std::endl;
    response << "<br><h1>General Server Statistics</h1>" << std::endl;
    response << "<br>Thread Pool Size: " << server.thread_pool_size_ << "</br>" << std::endl;
    response << "<br>Max Connections: " << server.acceptor_.max_connections << "</br>" << std::endl;
    response << "<br>Messages - Do not route: " << server.acceptor_.message_do_not_route << "</br>" << std::endl;
    response << "<br>Messages - End of Record: " << server.acceptor_.message_end_of_record << "</br>" << std::endl;
    response << "<br>Messages - Out of Band: " << server.acceptor_.message_out_of_band << "</br>" << std::endl;
    response << "<br>Messages - Peek: " << server.acceptor_.message_peek << "</br>" << std::endl;
    /**
     * // TODO:  Get a string representation of the signals this service is handling
    for(auto s : server.signals_) 
    {
    }
     */
    response << "<br><h1>Registered Web Service Ports:</h1></br>" << std::endl;
    for (auto h : server.request_handler_.custom_handlers) 
    {
      response << "<br><h2>" << h->get_service_port() << "</h2></br>" << std::endl;
      response << "<br>" << h->get_parameter_spec() << "</br>" << std::endl;
      response << "<br>Usage:&nbsp;" << h->usage_info() << "</br>" << std::endl;
    }
    response << "</body></html>";
    rep.content = response.str();
    rep.status = reply::ok;
    rep.headers.insert(std::make_pair(std::string("Content-Type"), mime_types::extension_to_type("html")));
  }

  const char* usage_info() const 
  {
    return "Reports basic status on the web service";
  }
private:
  const http::server::server& server;
};

server::server(const std::string& address, const std::string& port,
               const std::string& doc_root, std::size_t thread_pool_size)
: thread_pool_size_(thread_pool_size),
signals_(io_service_),
acceptor_(io_service_),
new_connection_(),
request_handler_(doc_root)
{
    // Register to handle the signals that indicate when the server should exit.
    // It is safe to register for the same signal multiple times in a program,
    // provided all registration for the specified signal is made through Asio.
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
#if defined(SIGQUIT)
    signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)
    signals_.async_wait(boost::bind(&server::handle_stop, this));

    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::resolver::query query(address, port);
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();

    register_handler(std::shared_ptr<registered_handler>(new echo_handler()));    
    register_handler(std::shared_ptr<registered_handler>(new status_handler(*this)));
    start_accept();
}

server::~server()
{
    stop();
}

void server::register_handler(std::shared_ptr<registered_handler> handler)
{
    request_handler_.register_handler(handler);
}

void server::run()
{
    std::vector<boost::shared_ptr<boost::thread> > threads;
    {
        // Create a pool of threads to run all of the io_services.
        for (std::size_t i = 0; i < thread_pool_size_; ++i)
        {
            boost::shared_ptr<boost::thread> thread(new boost::thread(boost::bind(&boost::asio::io_service::run, &io_service_)));
            threads.push_back(thread);
        }
    }

    // Wait for all threads in the pool to exit.
    for (std::size_t i = 0; i < threads.size(); ++i)
        threads[i]->join();
}

void server::stop()
{
    handle_stop();
}

void server::start_accept()
{
    new_connection_.reset(new connection(io_service_, request_handler_));
    acceptor_.async_accept(new_connection_->socket(),
                           boost::bind(&server::handle_accept, this,
                                       boost::asio::placeholders::error));
}

void server::handle_accept(const boost::system::error_code & e)
{
    if (!e)
    {
        new_connection_->start();
    }

    start_accept();
}

void server::handle_stop()
{
    if (io_service_.stopped() == false)
    {
        io_service_.stop();
    }
}

} // namespace server
} // namespace http
