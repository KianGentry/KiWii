#include "mkwii/server_internal.h"

#include "mkwii/nas_http.h"

#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace mkwii {

void handle_nas_connection(int nas_socket) {
    const int client_socket = accept(nas_socket, nullptr, nullptr);
    if (client_socket < 0) {
        return;
    }
    set_receive_timeout(client_socket, 2);
    std::string request_text;
    char request_chunk[1024];
    while (true) {
        const std::size_t header_end = request_text.find("\r\n\r\n");
        if (header_end != std::string::npos) {
            std::size_t content_length = 0;
            const std::size_t length_start = request_text.find("Content-Length:");
            if (length_start != std::string::npos) {
                const std::size_t value_start = length_start + 15;
                const std::size_t value_end = request_text.find("\r\n", value_start);
                try {
                    content_length = std::stoul(
                        request_text.substr(value_start, value_end - value_start));
                } catch (const std::exception &) {
                    content_length = 0;
                }
            }
            if (request_text.size() >= header_end + 4 + content_length) {
                break;
            }
        }
        const ssize_t request_size = recv(client_socket, request_chunk,
                                          sizeof(request_chunk), 0);
        if (request_size <= 0) {
            break;
        }
        request_text.append(request_chunk, static_cast<std::size_t>(request_size));
        if (request_text.size() > 8192) {
            break;
        }
    }
    const std::string response = nas_response_for_request(request_text);
    send_all(client_socket, response.data(), response.size());
    close(client_socket);
}

} // namespace mkwii
