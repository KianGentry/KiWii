#include "mkwii/server_internal.h"

#include "mkwii/gamespy_profile.h"
#include "mkwii/nas_http.h"

#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

namespace mkwii {

void handle_profile_connection(int profile_socket) {
    const int client_socket = accept(profile_socket, nullptr, nullptr);
    if (client_socket < 0) {
        return;
    }
    set_receive_timeout(client_socket, 5);
    const std::string login_challenge = profile_login_challenge();
    LoginCredentials credentials;
    std::string firstname;
    std::string lastname;
    std::string status;
    std::string statstring;
    std::string locstring;
    const std::size_t challenge_start = login_challenge.find("\\challenge\\") + 11;
    const std::size_t challenge_end = login_challenge.find("\\id\\", challenge_start);
    const std::string server_challenge =
        login_challenge.substr(challenge_start, challenge_end - challenge_start);
    send_all(client_socket, login_challenge.data(), login_challenge.size());

    std::string request_buffer;
    char request_chunk[4096];
    while (true) {
        const ssize_t packet_size = recv(client_socket, request_chunk,
                                         sizeof(request_chunk), 0);
        if (packet_size <= 0) {
            break;
        }
        request_buffer.append(request_chunk, static_cast<std::size_t>(packet_size));
        while (true) {
            const std::size_t message_end = request_buffer.find("\\final\\");
            if (message_end == std::string::npos) {
                break;
            }
            const std::size_t message_size = message_end + 7;
            const std::string request = request_buffer.substr(0, message_size);
            request_buffer.erase(0, message_size);
            if (is_profile_keepalive(request)) {
                const std::string &response = profile_keepalive_response();
                send_all(client_socket, response.data(), response.size());
            } else if (is_profile_login(request)) {
                const std::size_t token_marker = request.find("\\authtoken\\");
                const std::size_t token_start = token_marker == std::string::npos
                                                    ? std::string::npos
                                                    : token_marker + 11;
                const std::size_t token_end = token_start == std::string::npos
                                                  ? std::string::npos
                                                  : request.find('\\', token_start);
                const std::string token = token_start == std::string::npos
                                              ? std::string()
                                              : request.substr(token_start, token_end - token_start);
                credentials = credentials_for_token(token);
                const std::string response =
                    profile_login_response(request, server_challenge, credentials);
                send_all(client_socket, response.data(), response.size());
            } else if (is_profile_getprofile(request)) {
                const std::string response =
                    profile_getprofile_response(request, credentials, firstname, lastname);
                send_all(client_socket, response.data(), response.size());
            } else if (is_profile_updatepro(request)) {
                const std::string updated_firstname = profile_field_value(request, "firstname");
                const std::string updated_lastname = profile_field_value(request, "lastname");
                if (!updated_firstname.empty()) firstname = updated_firstname;
                if (!updated_lastname.empty()) lastname = updated_lastname;
            } else if (is_profile_status(request)) {
                status = profile_field_value(request, "status");
                statstring = profile_field_value(request, "statstring");
                locstring = profile_field_value(request, "locstring");
            }
        }
    }
    close(client_socket);
}

} // namespace mkwii
