//
// request_parser.cpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "request_parser.hpp"
#include "request.hpp"

namespace http {
  namespace server {

    request_parser::request_parser()
    : state_(method_start) {
    }

    void request_parser::reset() {
      state_ = method_start;
    }

    boost::tribool request_parser::consume(request& req, char input) {
      switch (state_) {
        case method_start:
          if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
            return false;
          } else {
            state_ = method;
            req.method.push_back(input);
            return boost::indeterminate;
          }
        case method:
          if (input == ' ') {
            state_ = uri;
            return boost::indeterminate;
          } else if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
            return false;
          } else {
            req.method.push_back(input);
            return boost::indeterminate;
          }
        case uri:
          switch (input) {
            case '?':
              state_ = url_param_start;
              req.uri.push_back(input);
              break;
            case ' ':
              state_ = http_version_h;
              break;
            default:
              if (is_ctl(input)) {
                return false;
              } else {
                req.uri.push_back(input);
              }
              break;
          }
          return boost::indeterminate;
        case url_param_start:
          state_ = url_param_name;
          req.parameter_key.clear();
          req.parameter_key.push_back(input);
          req.uri.push_back(input);
          return boost::indeterminate;
        case url_param_name:
          switch (input) {
            case '=':
              state_ = url_param_value;
              req.parameter_curr = req.parameters.insert(std::make_pair(req.parameter_key, ""));
              break;
            default:
              req.parameter_key.push_back(input);
              break;
          }
          req.uri.push_back(input);
          return boost::indeterminate;
        case url_param_value:
          switch (input) {
            case '&':
              state_ = url_param_start;
              break;
            case ' ':
              state_ = http_version_h;
              break;
            default:
              req.parameter_curr->second.push_back(input);
              break;
          }
          req.uri.push_back(input);
          return boost::indeterminate;
        case http_version_h:
          if (input == 'H') {
            state_ = http_version_t_1;
            return boost::indeterminate;
          } else {
            return false;
          }
        case http_version_t_1:
          if (input == 'T') {
            state_ = http_version_t_2;
            return boost::indeterminate;
          } else {
            return false;
          }
        case http_version_t_2:
          if (input == 'T') {
            state_ = http_version_p;
            return boost::indeterminate;
          } else {
            return false;
          }
        case http_version_p:
          if (input == 'P') {
            state_ = http_version_slash;
            return boost::indeterminate;
          } else {
            return false;
          }
        case http_version_slash:
          if (input == '/') {
            req.http_version_major = 0;
            req.http_version_minor = 0;
            state_ = http_version_major_start;
            return boost::indeterminate;
          } else {
            return false;
          }
        case http_version_major_start:
          if (is_digit(input)) {
            req.http_version_major = req.http_version_major * 10 + input - '0';
            state_ = http_version_major;
            return boost::indeterminate;
          } else {
            return false;
          }
        case http_version_major:
          if (input == '.') {
            state_ = http_version_minor_start;
            return boost::indeterminate;
          } else if (is_digit(input)) {
            req.http_version_major = req.http_version_major * 10 + input - '0';
            return boost::indeterminate;
          } else {
            return false;
          }
        case http_version_minor_start:
          if (is_digit(input)) {
            req.http_version_minor = req.http_version_minor * 10 + input - '0';
            state_ = http_version_minor;
            return boost::indeterminate;
          } else {
            return false;
          }
        case http_version_minor:
          if (input == '\r') {
            state_ = expecting_newline_1;
            return boost::indeterminate;
          } else if (is_digit(input)) {
            req.http_version_minor = req.http_version_minor * 10 + input - '0';
            return boost::indeterminate;
          } else {
            return false;
          }
        case expecting_newline_1:
          if (input == '\n') {
            state_ = header_line_start;
            return boost::indeterminate;
          } else {
            return false;
          }
        case header_line_start:
          if (input == '\r') {
            state_ = expecting_newline_3;
            return boost::indeterminate;
          } else if (!req.headers.empty() && (input == ' ' || input == '\t')) {
            state_ = header_lws;
            return boost::indeterminate;
          } else if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
            return false;
          } else {
            req.header_key.clear();
            req.header_key.push_back(input);
            state_ = header_name;
            return boost::indeterminate;
          }
        case header_lws:
          if (input == '\r') {
            state_ = expecting_newline_2;
            return boost::indeterminate;
          } else if (input == ' ' || input == '\t') {
            return boost::indeterminate;
          } else if (is_ctl(input)) {
            return false;
          } else {
            state_ = header_value;
            req.header_curr->second.push_back(input);
            return boost::indeterminate;
          }
        case header_name:
          if (input == ':') {
            state_ = space_before_header_value;
            req.header_curr = req.headers.insert(std::make_pair(req.header_key, ""));
            return boost::indeterminate;
          } else if (!is_char(input) || is_ctl(input) || is_tspecial(input)) {
            return false;
          } else {
            req.header_key.push_back(input);
            return boost::indeterminate;
          }
        case space_before_header_value:
          if (input == ' ') {
            state_ = header_value;
            return boost::indeterminate;
          } else {
            return false;
          }
        case header_value:
          if (input == '\r') {
            state_ = expecting_newline_2;
            return boost::indeterminate;
          } else if (is_ctl(input)) {
            return false;
          } else {
            req.header_curr->second.push_back(input);
            return boost::indeterminate;
          }
        case expecting_newline_2:
          if (input == '\n') {
            state_ = header_line_start;
            return boost::indeterminate;
          } else {
            return false;
          }
          break;
        case message_body:
          req.post.push_back(input);
          // If we have read the entirety of the post data (reserved in message_body_start) then we are done processing the request
          if (req.post.size() == req.post.capacity()) {
            return true;
          } else {
            return boost::indeterminate;
          }
          break;
        case expecting_newline_3:
          if (input == '\n') {
            if (req.method == "POST") {
              Headers::iterator header = req.headers.find("Content-Length");
              if (header != req.headers.end()) {
                req.post.reserve(atoi(header->second.c_str()));
              }
              // If there isn't a match on the form encoding, it must be a post
              state_ = message_body; // Default to standard message body
              header = req.headers.find("Content-Type");
              if (header != req.headers.end() && header->second.find("x-www-form-urlencoded") != std::string::npos) {
                state_ = post_param_start; // Override it if the content type was html forms
              }
            } else {
              return true;
            }
          }
          return boost::indeterminate;
        case post_param_start:
          req.post.push_back(input);
          req.parameter_key.clear();
          req.parameter_key.push_back(input);
          state_ = post_param_name;
          if (req.post.size() == req.post.capacity()) return true;
          else return boost::indeterminate;
          break;
        case post_param_name:
          req.post.push_back(input);
          if (input == '=') {
            req.parameter_curr = req.parameters.insert(std::make_pair(req.parameter_key, ""));
            state_ = post_param_value;
          } else {
            req.parameter_key.push_back(input);
          }
          if (req.post.size() == req.post.capacity()) return true;
          else return boost::indeterminate;
          return boost::indeterminate;
          break;
        case post_param_value:
          req.post.push_back(input);
          if (input == '&') {
            state_ = post_param_start;
          } else {
            req.parameter_curr->second.push_back(input);
          }
          if (req.post.size() == req.post.capacity()) return true;
          else return boost::indeterminate;
          break;
        default:
          return false;
          break;
      }
    }

    bool request_parser::is_char(int c) {

      return c >= 0 && c <= 127;
    }

    bool request_parser::is_ctl(int c) {

      return (c >= 0 && c <= 31) || (c == 127);
    }

    bool request_parser::is_tspecial(int c) {
      switch (c) {

        case '(': case ')': case '<': case '>': case '@':
        case ',': case ';': case ':': case '\\': case '"':
        case '/': case '[': case ']': case '?': case '=':
        case '{': case '}': case ' ': case '\t':
          return true;
        default:
          return false;
      }
    }

    bool request_parser::is_digit(int c) {
      return c >= '0' && c <= '9';
    }

  } // namespace server
} // namespace http
