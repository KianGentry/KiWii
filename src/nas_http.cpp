#include "mkwii/nas_http.h"

namespace mkwii {

std::string nas_connectivity_response() {
    return "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/html\r\n"
           "Content-Length: 2\r\n"
           "X-Organization: Nintendo\r\n"
           "Server: BigIP\r\n"
           "Connection: close\r\n"
           "\r\n"
           "ok";
}

}  // namespace mkwii