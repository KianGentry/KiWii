#include "mkwii/server_internal.h"

#include "mkwii/gamespy_qr.h"
#include "mkwii/gamespy_sessions.h"

#include <arpa/inet.h>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

namespace mkwii {

void handle_dns_packet(int dns_socket, const std::string &address) {
    std::vector<std::uint8_t> request(512);
    sockaddr_in client_address{};
    socklen_t client_address_length = sizeof(client_address);
    const ssize_t packet_size = recvfrom(
        dns_socket, request.data(), request.size(), 0,
        reinterpret_cast<sockaddr *>(&client_address), &client_address_length);
    if (packet_size < 12) {
        return;
    }
    request.resize(static_cast<std::size_t>(packet_size));
    std::size_t question_end = 12;
    while (question_end < request.size() && request[question_end] != 0) {
        const std::size_t label_length = request[question_end];
        if (label_length > 63 || question_end + label_length + 1 >= request.size()) {
            return;
        }
        question_end += label_length + 1;
    }
    if (question_end + 5 > request.size()) {
        return;
    }
    question_end += 5;
    std::istringstream address_stream(address);
    unsigned int first = 0;
    unsigned int second = 0;
    unsigned int third = 0;
    unsigned int fourth = 0;
    char separator = 0;
    if (!(address_stream >> first >> separator >> second >> separator >> third >>
          separator >> fourth) || first > 255 || second > 255 || third > 255 ||
        fourth > 255) {
        return;
    }
    std::vector<std::uint8_t> response(request.begin(), request.begin() + question_end);
    response[2] = 0x81;
    response[3] = 0x80;
    response[4] = 0x00;
    response[5] = 0x01;
    response[6] = 0x00;
    response[7] = 0x01;
    response[8] = response[9] = response[10] = response[11] = 0x00;
    response.insert(response.end(), {0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01,
                                     0x00, 0x00, 0x00, 0x3c, 0x00, 0x04,
                                     static_cast<std::uint8_t>(first),
                                     static_cast<std::uint8_t>(second),
                                     static_cast<std::uint8_t>(third),
                                     static_cast<std::uint8_t>(fourth)});
    sendto(dns_socket, response.data(), response.size(), 0,
           reinterpret_cast<sockaddr *>(&client_address), client_address_length);
}

void handle_qr_packet(int qr_socket, const std::string &secret_key) {
    static std::unordered_map<std::uint32_t, std::string> qr_challenges;
    std::vector<std::uint8_t> packet(2048);
    sockaddr_in client_address{};
    socklen_t client_address_length = sizeof(client_address);
    const ssize_t packet_size = recvfrom(
        qr_socket, packet.data(), packet.size(), 0,
        reinterpret_cast<sockaddr *>(&client_address), &client_address_length);
    if (packet_size < 0) {
        return;
    }
    packet.resize(static_cast<std::size_t>(packet_size));
    const std::uint32_t session_id = qr_session_id(packet);
    const auto challenge = qr_challenges.find(session_id);
    if (packet.size() >= 5 && packet[0] == 0x01 &&
        challenge != qr_challenges.end() &&
        qr_challenge_matches(packet, challenge->second, secret_key)) {
        const std::vector<std::uint8_t> response = qr_registered_response(session_id);
        sendto(qr_socket, response.data(), response.size(), 0,
               reinterpret_cast<sockaddr *>(&client_address), client_address_length);
        qr_challenges.erase(challenge);
        std::cout << "GameSpy QR session registered: " << session_id << "\n";
        return;
    }
    if (is_mariokartwii_heartbeat(packet)) {
        const std::string state_changed = qr_field_value(packet, "statechanged");
        if (state_changed == "2") {
            remove_online_session(session_id);
            qr_challenges.erase(session_id);
        } else {
            const std::string profile_id = qr_field_value(packet, "profileid");
            const std::string user_id = qr_field_value(packet, "userid");
            const std::string unique_nick = qr_field_value(packet, "uniquenick");
            upsert_online_session({
                session_id,
                !profile_id.empty() ? profile_id
                                     : (!user_id.empty() ? user_id
                                                         : std::to_string(session_id)),
                unique_nick,
                qr_field_value(packet, "gamename"),
                inet_ntoa(client_address.sin_addr),
                ntohs(client_address.sin_port)});
        }
        if (qr_challenges.find(session_id) == qr_challenges.end()) {
            const std::vector<std::uint8_t> response = qr_challenge_response(
                session_id, inet_ntoa(client_address.sin_addr),
                ntohs(client_address.sin_port));
            sendto(qr_socket, response.data(), response.size(), 0,
                   reinterpret_cast<sockaddr *>(&client_address), client_address_length);
            const auto challenge_end = std::find(response.begin() + 7, response.end(), 0x00);
            qr_challenges.emplace(session_id,
                                  std::string(response.begin() + 7, challenge_end));
            std::cout << "GameSpy QR challenge sent for session " << session_id << "\n";
        }
        std::cout << "GameSpy QR heartbeat for session " << session_id << " (state "
                  << (state_changed.empty() ? "unchanged" : state_changed) << ")\n";
        return;
    }
    if (!is_mariokartwii_availability_request(packet)) {
        return;
    }
    const std::vector<std::uint8_t> response = availability_response();
    sendto(qr_socket, response.data(), response.size(), 0,
           reinterpret_cast<sockaddr *>(&client_address), client_address_length);
    std::cout << "GameSpy QR availability request from "
              << inet_ntoa(client_address.sin_addr) << '\n';
}

void handle_relay_connection(int relay_socket, SSL_CTX *ssl_context) {
    const int client_socket = accept(relay_socket, nullptr, nullptr);
    if (client_socket < 0) {
        return;
    }
    set_receive_timeout(client_socket, 5);
    SSL *ssl = SSL_new(ssl_context);
    if (ssl == nullptr || SSL_set_fd(ssl, client_socket) != 1 || SSL_accept(ssl) != 1) {
        std::cerr << "GameSpy relay TLS handshake failed\n";
        SSL_free(ssl);
        close(client_socket);
        return;
    }
    std::vector<std::uint8_t> packet(4096);
    const int packet_size = SSL_read(ssl, packet.data(), static_cast<int>(packet.size()));
    if (packet_size > 0) {
        packet.resize(static_cast<std::size_t>(packet_size));
        std::ostringstream formatted_packet;
        formatted_packet << std::hex << std::setfill('0');
        for (const std::uint8_t byte : packet) {
            formatted_packet << std::setw(2) << static_cast<unsigned int>(byte);
        }
        std::cout << "GameSpy relay request (" << packet.size()
                  << " bytes): " << formatted_packet.str() << '\n';
    }
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(client_socket);
}

std::string player_search_response(const std::string &request) {
    std::string response = "\\otherslist\\";
    std::vector<std::string> requested_profiles;
    const std::size_t opids_marker = request.find("\\opids\\");
    if (opids_marker != std::string::npos) {
        const std::size_t value_start = opids_marker + 7;
        const std::size_t value_end = request.find('\\', value_start);
        const std::string opids = request.substr(
            value_start, value_end == std::string::npos ? std::string::npos
                                                         : value_end - value_start);
        std::size_t start = 0;
        while (start <= opids.size()) {
            const std::size_t separator = opids.find('|', start);
            const std::string opid = opids.substr(
                start, separator == std::string::npos ? std::string::npos
                                                       : separator - start);
            if (!opid.empty()) {
                requested_profiles.push_back(opid);
            }
            if (separator == std::string::npos) {
                break;
            }
            start = separator + 1;
        }
    }
    for (const OnlineSession &session : online_sessions_for_profiles(requested_profiles)) {
        response += "\\o\\" + session.profile_id + "\\uniquenick\\" +
                    session.unique_nick;
    }
    return response + "\\oldone\\\\final\\";
}

void handle_player_search_connection(int player_search_socket) {
    const int client_socket = accept(player_search_socket, nullptr, nullptr);
    if (client_socket < 0) {
        return;
    }
    set_receive_timeout(client_socket, 5);
    std::string request_buffer;
    char request_chunk[4096];
    while (true) {
        const ssize_t packet_size = recv(client_socket, request_chunk, sizeof(request_chunk), 0);
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
            const std::string response = player_search_response(request);
            send_all(client_socket, response.data(), response.size());
        }
    }
    close(client_socket);
}

} // namespace mkwii
