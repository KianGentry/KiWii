#include "mkwii/server_internal.h"

#include "mkwii/gamespy_qr.h"

#include <arpa/inet.h>
#include <iomanip>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace mkwii {

void handle_natneg_packet(int natneg_socket) {
    constexpr std::uint8_t magic[] = {0xfd, 0xfc, 0x1e, 0x66, 0x6a, 0xb2};
    std::vector<std::uint8_t> packet(2048);
    sockaddr_in client_address{};
    socklen_t client_address_length = sizeof(client_address);
    const ssize_t packet_size = recvfrom(
        natneg_socket, packet.data(), packet.size(), 0,
        reinterpret_cast<sockaddr *>(&client_address), &client_address_length);
    if (packet_size < 8) {
        return;
    }
    packet.resize(static_cast<std::size_t>(packet_size));
    if (!std::equal(std::begin(magic), std::end(magic), packet.begin()) ||
        packet[6] != 0x03) {
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
    if (packet[7] != 0x00 || packet.size() < 14) {
        return;
    }
    std::vector<std::uint8_t> response(packet.begin(), packet.begin() + 14);
    response.insert(response.end(), {0xff, 0xff, 0x6d, 0x16, 0xb5, 0x7d, 0xea});
    response[7] = 0x01;
    sendto(natneg_socket, response.data(), response.size(), 0,
           reinterpret_cast<sockaddr *>(&client_address), client_address_length);
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
