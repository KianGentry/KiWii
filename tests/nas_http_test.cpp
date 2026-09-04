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

    const std::string login_request =
        "POST /ac HTTP/1.1\r\n"
        "Host: naswii.nintendowifi.net\r\n\r\n"
        "action=bG9naW4%2A&userid=test";
    const std::string login_response = mkwii::nas_response_for_request(login_request);
        assert(login_response.find("Content-Type: text/plain\r\n") != std::string::npos);
        assert(login_response.find("NODE: wifiappe1\r\n") != std::string::npos);
    assert(login_response.find("retry=0&returncd=001&locator=gamespy.com&") !=
           std::string::npos);
        assert(login_response.find("&challenge=") != std::string::npos);
        assert(login_response.find("&token=NDS") != std::string::npos);
        assert(login_response.find("&datetime=") != std::string::npos);
        assert(login_response.size() > response.size());

    assert(mkwii::nas_response_for_request("GET / HTTP/1.1\r\n\r\n") == response);
    return 0;
}