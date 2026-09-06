#include "mkwii/server_internal.h"

#include "mkwii/gamespy_natneg.h"

#include <arpa/inet.h>
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
        return;
    }
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
    auto &session = natneg_sessions[client.session_id];
    session[client.client_index] = {client, packet, client_address};
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
    std::vector<std::uint8_t> packet(4096);
    const ssize_t packet_size = recv(client_socket, packet.data(), packet.size(), 0);
    if (packet_size > 0) {
        packet.resize(static_cast<std::size_t>(packet_size));
        std::ostringstream formatted_packet;
        formatted_packet << std::hex << std::setfill('0');
        for (const std::uint8_t byte : packet) {
            formatted_packet << std::setw(2) << static_cast<unsigned int>(byte);
        }
        std::cout << "GameSpy browser request (" << packet.size()
                  << " bytes): " << formatted_packet.str() << '\n';
    }
    close(client_socket);
}

} // namespace mkwii
