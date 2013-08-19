//
// request.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER_REQUEST_HPP
#define HTTP_SERVER_REQUEST_HPP

#include <string>
#include <vector>
#include <map>
#include "http_server_types.h"

namespace http {
namespace server {

/// A request received from a client.
struct request
{
  std::string method;
  std::string post;
  std::string uri;
  int http_version_major;
  int http_version_minor;

  Headers headers;
  Headers::key_type header_key;
  Headers::iterator header_curr;

  Parameters parameters;
  Parameters::key_type parameter_key;
  Parameters::iterator parameter_curr;

};

} // namespace server
} // namespace http

#endif // HTTP_SERVER_REQUEST_HPP
