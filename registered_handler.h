/* 
 * File:   registered_handler.h
 * Author: vortarian
 *
 * Created on February 27, 2013, 5:41 PM
 */

#ifndef REGISTERED_HANDLER_H
#define	REGISTERED_HANDLER_H

#include "request.hpp"
#include "reply.hpp"
#include "request_handler.hpp"
#include "mime_types.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <memory>
#include <sstream>
#include <set>

namespace http {
namespace server {

class registered_handler
{
public:

  registered_handler(const std::string& web_service_port, const std::set<std::string>&& parameters_required = {}) :
      web_service_port(web_service_port), parameters_required(std::move(parameters_required))
  {
    update_parameter_spec();
  }

  registered_handler(std::string&& web_service_port, const std::set<std::string>&& parameters_required = {}) :
    web_service_port(web_service_port), parameters_required(std::move(parameters_required))
  {
    update_parameter_spec();
  }

  registered_handler(const registered_handler& orig) :
      web_service_port(orig.web_service_port), parameters_required(orig.parameters_required), parameter_spec(orig.parameter_spec)
  {
  }

  registered_handler(const registered_handler && orig) :
      web_service_port(std::move(orig.web_service_port)), parameters_required(std::move(orig.parameters_required)), parameter_spec(std::move(orig.parameter_spec))
  {
  }

  virtual ~registered_handler()
  {
  }

  /**
   * Process the request, required to be implemented by concrete class
   */
  virtual void handle_request(const request& req, reply& rep) const = 0;

  const std::string& get_parameter_spec() {
    return parameter_spec;
  }

  const std::string& get_service_port()
  {
    return web_service_port;
  }

  inline bool is_default_parameter(const std::string& parameter) const
  {
    return parameters_required.find(parameter) != parameters_required.end();
  }

  virtual const char* usage_info() const = 0;

  /**
   * Verify the incoming request is valid for this object.
   * @return True if the request should be processed, false otherwise.
   * Upon a false return, the reply will have the appropriate error populated
   */
  bool verify_request(const request& req) const
  {
    bool ret = true;
    if(parameters_required.empty() == false)
    {
      for(const std::string& req_param : parameters_required)
      {
        if(req.parameters.find(req_param) == req.parameters.end())
        {
          ret = false;
          break;
        }
      }
    }
    return ret;
  }
private:
  /**
   * Set the parameters required specification for a request to be verified and processed.
   */
  void update_parameter_spec()
  {
    std::stringstream str;
    str << "Parameters required: ";
    for(const std::string& param : parameters_required)
    {
      str << boost::format("[%1%] ") % param;
    }
    parameter_spec = str.str();
  }

  std::string parameter_spec;
  std::string web_service_port;
  std::set<std::string> parameters_required;
};

} // namespace server
} // namespace http
#endif	/* REGISTERED_HANDLER_H */

