#include "mkwii/server.h"

#include "mkwii/gamespy_qr.h"
#include "mkwii/gamespy_profile.h"
#include "mkwii/nas_http.h"

#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
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

int open_game_socket(std::uint16_t port) {
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

void handle_game_connection(int game_socket) {
    const int client_socket = accept(game_socket, nullptr, nullptr);
    if (client_socket < 0) {
        return;
    }

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

void handle_profile_connection(int profile_socket) {
    const int client_socket = accept(profile_socket, nullptr, nullptr);
    if (client_socket < 0) {
        return;
    }

    std::vector<std::uint8_t> packet(4096);
    const ssize_t packet_size = recv(client_socket, packet.data(), packet.size(), 0);
    if (packet_size > 0) {
        packet.resize(static_cast<std::size_t>(packet_size));
        std::ostringstream formatted_packet;
        formatted_packet << std::hex << std::setfill('0');
        for (const std::uint8_t byte : packet) {
            formatted_packet << std::setw(2) << static_cast<unsigned int>(byte);
        }
        std::cout << "GameSpy profile request (" << packet.size()
                << " bytes): " << formatted_packet.str() << '\n';

        const std::string request(packet.begin(), packet.end());
        if (is_profile_keepalive(request)) {
            const std::string& response = profile_keepalive_response();
            send(client_socket, response.data(), response.size(), MSG_NOSIGNAL);
            std::cout << "GameSpy profile keepalive response sent\n";
        }
    }
    close(client_socket);
}

void handle_nas_connection(int nas_socket) {
    const int client_socket = accept(nas_socket, nullptr, nullptr);
    if (client_socket < 0) {
        return;
    }

    timeval receive_timeout{2, 0};
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &receive_timeout,
            sizeof(receive_timeout));

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
                    content_length = std::stoul(request_text.substr(
                        value_start, value_end - value_start));
                } catch (const std::exception&) {
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
    send(client_socket, response.data(), response.size(), MSG_NOSIGNAL);
    close(client_socket);
    std::cout << "NAS connectivity request served\n";
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

    const int game_socket = open_game_socket(config.game_port);
    if (game_socket < 0) {
        std::cerr << "could not bind GameSpy browser port " << config.game_port << '\n';
        close(health_socket);
        close(qr_socket);
        return 1;
    }

    const int nas_socket = open_game_socket(config.nas_port);
    if (nas_socket < 0) {
        std::cerr << "could not bind NAS port " << config.nas_port << '\n';
        close(health_socket);
        close(qr_socket);
        close(game_socket);
        return 1;
    }

    const int profile_socket = open_game_socket(config.profile_port);
    if (profile_socket < 0) {
        std::cerr << "could not bind GameSpy profile port " << config.profile_port << '\n';
        close(health_socket);
        close(qr_socket);
        close(game_socket);
        close(nas_socket);
        return 1;
    }

    std::cout << "server '" << config.server_name << "' started\n"
            << "advertised address: " << config.advertised_address << '\n'
            << "health port: " << config.health_port << '\n'
            << "DNS port (reserved): " << config.dns_port << '\n'
            << "NAS HTTP port: " << config.nas_port << '\n'
            << "GameSpy QR port: " << config.qr_port << '\n'
            << "GameSpy profile port: " << config.profile_port << '\n'
            << "GameSpy browser port: " << config.game_port << '\n';

    while (keep_running != 0) {
        timeval timeout{1, 0};
        fd_set readable{};
        FD_ZERO(&readable);
        FD_SET(health_socket, &readable);
        FD_SET(qr_socket, &readable);
        FD_SET(game_socket, &readable);
        FD_SET(nas_socket, &readable);
        FD_SET(profile_socket, &readable);
        const int highest_socket = std::max({health_socket, qr_socket, game_socket, nas_socket, profile_socket});

        if (select(highest_socket + 1, &readable, nullptr, nullptr, &timeout) <= 0) {
            continue;
        }

        if (FD_ISSET(qr_socket, &readable)) {
            handle_qr_packet(qr_socket);
        }

        if (FD_ISSET(game_socket, &readable)) {
            handle_game_connection(game_socket);
        }

        if (FD_ISSET(nas_socket, &readable)) {
            handle_nas_connection(nas_socket);
        }

        if (FD_ISSET(profile_socket, &readable)) {
            handle_profile_connection(profile_socket);
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
    close(game_socket);
    close(nas_socket);
    close(profile_socket);
    std::cout << "server stopped\n";
    return 0;
}

}  // namespace mkwii
