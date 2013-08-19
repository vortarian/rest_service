//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "request_handler.hpp"
#include <fstream>
#include <list>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include "mime_types.hpp"
#include "reply.hpp"
#include "request.hpp"

namespace http {
  namespace server {

    request_handler::request_handler(const std::string& doc_root)
    : doc_root_(doc_root) {
    }

    void request_handler::handle_request(const request& req, reply& rep) {
      // Decode url to path.
      std::string request_path;
      if (!url_decode(req.uri, request_path)) {
        rep = reply::stock_reply(reply::bad_request);
        return;
      }

      // Request path must be absolute and not contain "..".
      if (request_path.empty() || request_path[0] != '/'
              || request_path.find("..") != std::string::npos) {
        rep = reply::stock_reply(reply::bad_request);
        return;
      }

      // Check for a custom handler
      std::shared_ptr<registered_handler> custom_handler;
      for (auto iter = custom_handlers.begin(); iter != custom_handlers.end(); iter++) {
        if (req.uri.find((*iter)->get_service_port()) == 0) {
          // use the custom handler
          custom_handler = *iter;
          break;
        }
      }

      if (custom_handler) {
        bool verified = custom_handler->verify_request(req);
        if(verified == true) {
          custom_handler->handle_request(req, rep);
        } else {
            rep.headers.insert(std::make_pair(std::string("Content-Type"), mime_types::extension_to_type("text")));
            rep.content = custom_handler->get_parameter_spec();
            rep.status = rep.bad_request;
        }
      } else {
        // Assume it is a file request
        // If path ends in slash (i.e. is a directory) then add "index.html".
        if (request_path[request_path.size() - 1] == '/') {
          request_path += "index.html";
        }

        // Determine the file extension.
        std::size_t last_slash_pos = request_path.find_last_of("/");
        std::size_t last_dot_pos = request_path.find_last_of(".");
        std::string extension;
        if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos) {
          extension = request_path.substr(last_dot_pos + 1);
        }

        // Open the file to send back.
        std::string full_path = doc_root_ + request_path;
        std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
        if (!is) {
          rep = reply::stock_reply(reply::not_found);
          return;
        }
        char buf[512];
        while (is.read(buf, sizeof (buf)).gcount() > 0)
          rep.content.append(buf, is.gcount());
        ;
        rep.headers.insert(make_pair(std::string("Content-Type"), mime_types::extension_to_type(extension)));
      }
      // Fill out the reply to be sent to the client if one was not filled out by the handler
      if(rep.status == reply::uninitialized)
        rep.status = reply::ok;
      rep.headers.insert(make_pair("Content-Length", boost::lexical_cast<std::string > (rep.content.size())));
    }

    void request_handler::register_handler(std::shared_ptr<registered_handler>& handler) {
      custom_handlers.push_back(handler);
    }

    bool request_handler::url_decode(const std::string& in, std::string& out) {
      out.clear();
      out.reserve(in.size());
      for (std::size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '%') {
          if (i + 3 <= in.size()) {
            int value = 0;
            std::istringstream is(in.substr(i + 1, 2));
            if (is >> std::hex >> value) {
              out += static_cast<char> (value);
              i += 2;
            } else {
              return false;
            }
          } else {
            return false;
          }
        } else if (in[i] == '+') {
          out += ' ';
        } else {
          out += in[i];
        }
      }
      return true;
    }

  } // namespace server
} // namespace http
