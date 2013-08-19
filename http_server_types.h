#ifndef HTTP_SERVER_TYPES_H
#define	HTTP_SERVER_TYPES_H

namespace http {
namespace server {

// key is name, entry is value
typedef std::multimap<std::string, std::string> Headers;
typedef std::multimap<std::string, std::string> Parameters;

} // server
} // http

#endif	/* HTTP_SERVER_TYPES_H */

