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

std::string nas_response_for_request(const std::string& request) {
    if (request.find("POST /ac ") == std::string::npos ||
        request.find("action=bG9naW4%2A") == std::string::npos) {
        return nas_connectivity_response();
    }

    constexpr char login_body[] =
        "retry=0&returncd=001&locator=gamespy.com&"
        "challenge=kiwii-challenge&token=kiwii-token&"
        "datetime=20260904000000";
    return "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: " + std::to_string(sizeof(login_body) - 1) +
           "\r\n"
           "Connection: close\r\n"
           "\r\n" + login_body;
}

}  // namespace mkwii