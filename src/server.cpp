#include "mkwii/server.h"

#include "mkwii/gamespy_qr.h"

#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <vector>

namespace mkwii {
namespace {

volatile std::sig_atomic_t keep_running = 1;

void stop_server(int) {
    keep_running = 0;
}

int open_health_socket(std::uint16_t port) {
    const int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        return -1;
    }

    int reuse_address = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_address, sizeof(reuse_address));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    if (bind(socket_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0 ||
        listen(socket_fd, 8) < 0) {
        close(socket_fd);
        return -1;
    }
    return socket_fd;
}

int open_qr_socket(std::uint16_t port) {
    const int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        return -1;
    }

    int reuse_address = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_address, sizeof(reuse_address));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    if (bind(socket_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        close(socket_fd);
        return -1;
    }
    return socket_fd;
}

void handle_qr_packet(int qr_socket) {
    std::vector<std::uint8_t> packet(2048);
    sockaddr_in client_address{};
    socklen_t client_address_length = sizeof(client_address);
    const ssize_t packet_size = recvfrom(
        qr_socket,
        packet.data(),
        packet.size(),
        0,
        reinterpret_cast<sockaddr*>(&client_address),
        &client_address_length);
    if (packet_size < 0) {
        return;
    }

    packet.resize(static_cast<std::size_t>(packet_size));
    if (!is_mariokartwii_availability_request(packet)) {
        return;
    }

    const std::vector<std::uint8_t> response = availability_response();
    sendto(
        qr_socket,
        response.data(),
        response.size(),
        0,
        reinterpret_cast<sockaddr*>(&client_address),
        client_address_length);
    std::cout << "GameSpy QR availability request from "
            << inet_ntoa(client_address.sin_addr) << '\n';
}

}  // namespace

int run_server(const Config& config) {
    keep_running = 1;
    std::signal(SIGINT, stop_server);
    std::signal(SIGTERM, stop_server);

    const int health_socket = open_health_socket(config.health_port);
    if (health_socket < 0) {
        std::cerr << "could not bind health port " << config.health_port << '\n';
        return 1;
    }

    const int qr_socket = open_qr_socket(config.qr_port);
    if (qr_socket < 0) {
        std::cerr << "could not bind GameSpy QR port " << config.qr_port << '\n';
        close(health_socket);
        return 1;
    }

    std::cout << "server '" << config.server_name << "' started\n"
            << "advertised address: " << config.advertised_address << '\n'
            << "health port: " << config.health_port << '\n'
            << "DNS port (reserved): " << config.dns_port << '\n'
            << "GameSpy QR port: " << config.qr_port << '\n'
            << "game port (reserved): " << config.game_port << '\n';

    while (keep_running != 0) {
        timeval timeout{1, 0};
        fd_set readable{};
        FD_ZERO(&readable);
        FD_SET(health_socket, &readable);
        FD_SET(qr_socket, &readable);
        const int highest_socket = std::max(health_socket, qr_socket);

        if (select(highest_socket + 1, &readable, nullptr, nullptr, &timeout) <= 0) {
            continue;
        }

        if (FD_ISSET(qr_socket, &readable)) {
            handle_qr_packet(qr_socket);
        }

        if (!FD_ISSET(health_socket, &readable)) {
            continue;
        }

        const int client_socket = accept(health_socket, nullptr, nullptr);
        if (client_socket < 0) {
            continue;
        }

        constexpr char response[] =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 3\r\n"
            "Connection: close\r\n\r\n"
            "ok\n";
        send(client_socket, response, sizeof(response) - 1, MSG_NOSIGNAL);
        close(client_socket);
    }

    close(health_socket);
    close(qr_socket);
    std::cout << "server stopped\n";
    return 0;
}

}  // namespace mkwii
