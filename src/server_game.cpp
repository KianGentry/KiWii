#include "mkwii/server_internal.h"

#include "mkwii/gamespy_browser.h"
#include "mkwii/gamespy_natneg.h"

#include <arpa/inet.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace mkwii {

namespace {

struct NatNegSessionClient {
    NatNegClient client;
    std::vector<std::uint8_t> init_packet;
    sockaddr_in endpoint;
    std::chrono::steady_clock::time_point last_seen;
};

std::map<std::uint32_t, std::map<std::uint8_t, NatNegSessionClient>> natneg_sessions;
std::mutex natneg_sessions_mutex;

} // namespace

void handle_natneg_packet(int natneg_socket) {
    std::vector<std::uint8_t> packet(2048);
    sockaddr_in client_address{};
    socklen_t client_address_length = sizeof(client_address);
    const ssize_t packet_size = recvfrom(
        natneg_socket, packet.data(), packet.size(), 0,
        reinterpret_cast<sockaddr *>(&client_address), &client_address_length);
    if (packet_size <= 0) {
        return;
    }
    packet.resize(static_cast<std::size_t>(packet_size));
    NatNegClient client{};
    if (!parse_natneg_init(packet, client)) {
        if (packet.size() >= 8 && packet[7] == 0x06) {
            const std::vector<std::uint8_t> response = natneg_connect_ack(packet);
            sendto(natneg_socket, response.data(), response.size(), 0,
                   reinterpret_cast<sockaddr *>(&client_address), client_address_length);
        } else if (packet.size() >= 8 && packet[7] == 0x0a) {
            const std::vector<std::uint8_t> response = natneg_address_reply(
                packet, client_address.sin_addr.s_addr, ntohs(client_address.sin_port));
            if (!response.empty()) {
                sendto(natneg_socket, response.data(), response.size(), 0,
                       reinterpret_cast<sockaddr *>(&client_address), client_address_length);
            }
        }
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    std::ostringstream formatted_packet;
    formatted_packet << std::hex << std::setfill('0');
    for (const std::uint8_t byte : packet) {
        formatted_packet << std::setw(2) << static_cast<unsigned int>(byte);
    }
    std::cout << "GameSpy NATNEG record 0x" << std::setw(2)
              << static_cast<unsigned int>(packet[7]) << " ("
              << packet.size() << " bytes): " << formatted_packet.str() << '\n';
    client.address = client_address.sin_addr.s_addr;
    client.port = ntohs(client_address.sin_port);
    const std::vector<std::uint8_t> response = natneg_init_ack(packet);
    sendto(natneg_socket, response.data(), response.size(), 0,
           reinterpret_cast<sockaddr *>(&client_address), client_address_length);

    std::lock_guard<std::mutex> lock(natneg_sessions_mutex);
    for (auto session_iterator = natneg_sessions.begin();
         session_iterator != natneg_sessions.end();) {
        bool active = false;
        for (const auto &[index, stored_client] : session_iterator->second) {
            if (now - stored_client.last_seen < std::chrono::seconds(30)) {
                active = true;
                break;
            }
        }
        if (!active) {
            session_iterator = natneg_sessions.erase(session_iterator);
        } else {
            ++session_iterator;
        }
    }
    auto &session = natneg_sessions[client.session_id];
    session[client.client_index] = {client, packet, client_address, now};
    if (session.size() == 2) {
        auto first = session.begin();
        auto second = std::next(first);
        const std::vector<std::uint8_t> first_connect =
            natneg_connect(first->second.init_packet, second->second.client);
        const std::vector<std::uint8_t> second_connect =
            natneg_connect(second->second.init_packet, first->second.client);
        sendto(natneg_socket, first_connect.data(), first_connect.size(), 0,
               reinterpret_cast<sockaddr *>(&first->second.endpoint), sizeof(sockaddr_in));
        sendto(natneg_socket, second_connect.data(), second_connect.size(), 0,
               reinterpret_cast<sockaddr *>(&second->second.endpoint), sizeof(sockaddr_in));
    }
    std::cout << "GameSpy NATNEG initialization acknowledged\n";
}

void handle_game_connection(int game_socket) {
    const int client_socket = accept(game_socket, nullptr, nullptr);
    if (client_socket < 0) {
        return;
    }
    set_receive_timeout(client_socket, 5);
    std::uint8_t length_bytes[2]{};
    if (recv(client_socket, length_bytes, sizeof(length_bytes), MSG_WAITALL) !=
        static_cast<ssize_t>(sizeof(length_bytes))) {
        close(client_socket);
        return;
    }
    const std::size_t packet_size =
        (static_cast<std::size_t>(length_bytes[0]) << 8) | length_bytes[1];
    if (packet_size < 3 || packet_size > 2048) {
        close(client_socket);
        return;
    }
    std::vector<std::uint8_t> packet(packet_size);
    packet[0] = length_bytes[0];
    packet[1] = length_bytes[1];
    if (recv(client_socket, packet.data() + 2, packet.size() - 2, MSG_WAITALL) !=
        static_cast<ssize_t>(packet.size() - 2)) {
        close(client_socket);
        return;
    }
    BrowserRequest request{};
    if (parse_browser_request(packet, request)) {
        const std::vector<std::uint8_t> response =
            browser_empty_server_list(request, "0.0.0.0", 0);
        if (!response.empty()) {
            send_all(client_socket, reinterpret_cast<const char *>(response.data()),
                     response.size());
        }
    }
    close(client_socket);
}

} // namespace mkwii
