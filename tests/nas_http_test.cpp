#include "mkwii/nas_http.h"

#include <cassert>

int main() {
    const std::string response = mkwii::nas_connectivity_response();
    assert(response.find("HTTP/1.1 200 OK\r\n") == 0);
    assert(response.find("Content-Type: text/html\r\n") != std::string::npos);
    assert(response.find("Content-Length: 2\r\n") != std::string::npos);
    assert(response.find("X-Organization: Nintendo\r\n") != std::string::npos);
    assert(response.find("Server: BigIP\r\n") != std::string::npos);
    assert(response.ends_with("\r\n\r\nok"));
    return 0;
}