#include "mkwii/server.h"
#include "mkwii/server_internal.h"

#include <csignal>
#include <iostream>
#include <sys/select.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace mkwii {
namespace {

volatile std::sig_atomic_t keep_running = 1;

void stop_server(int) { keep_running = 0; }

void close_sockets(const std::vector<int> &sockets) {
    for (const int socket_fd : sockets) {
        if (socket_fd >= 0) {
            close(socket_fd);
        }
    }
}

} // namespace

int run_server(const Config &config) {
    keep_running = 1;
    std::signal(SIGINT, stop_server);
    std::signal(SIGTERM, stop_server);

    std::vector<int> sockets;
    const auto open_socket = [&](int socket_fd, const char *service) {
        if (socket_fd >= 0) {
            sockets.push_back(socket_fd);
            return true;
        }
        std::cerr << "could not bind " << service << '\n';
        close_sockets(sockets);
        return false;
    };

    const int health_socket = open_tcp_socket(config.health_port);
    if (!open_socket(health_socket, "health port")) return 1;
    const int qr_socket = open_udp_socket(config.qr_port);
    if (!open_socket(qr_socket, "GameSpy QR port")) return 1;
    const int dns_socket = open_udp_socket(config.dns_port);
    if (!open_socket(dns_socket, "DNS port")) return 1;
    const int natneg_socket = open_udp_socket(config.natneg_port);
    if (!open_socket(natneg_socket, "GameSpy NATNEG port")) return 1;
    const int game_socket = open_tcp_socket(config.game_port);
    if (!open_socket(game_socket, "GameSpy browser port")) return 1;
    const int nas_socket = open_tcp_socket(config.nas_port);
    if (!open_socket(nas_socket, "NAS port")) return 1;
    const int profile_socket = open_tcp_socket(config.profile_port);
    if (!open_socket(profile_socket, "GameSpy profile port")) return 1;
    const int player_search_socket = open_tcp_socket(config.player_search_port);
    if (!open_socket(player_search_socket, "GameSpy player search port")) return 1;
    const int relay_socket = open_tcp_socket(config.relay_port);
    if (!open_socket(relay_socket, "GameSpy relay port")) return 1;

    SSL_CTX *relay_ssl_context = create_relay_ssl_context();
    if (relay_ssl_context == nullptr) {
        std::cerr << "could not initialize relay TLS context\n";
        close_sockets(sockets);
        return 1;
    }

    std::cout << "server '" << config.server_name << "' started\n"
              << "advertised address: " << config.advertised_address << '\n'
              << "DNS port: " << config.dns_port << '\n'
              << "NAS HTTP port: " << config.nas_port << '\n'
              << "GameSpy QR port: " << config.qr_port << '\n'
              << "GameSpy NATNEG port: " << config.natneg_port << '\n'
              << "GameSpy profile port: " << config.profile_port << '\n'
              << "GameSpy player search port: " << config.player_search_port << '\n'
              << "GameSpy relay port: " << config.relay_port << '\n'
              << "GameSpy browser port: " << config.game_port << '\n';

    std::vector<std::thread> workers;
    while (keep_running != 0) {
        timeval timeout{1, 0};
        fd_set readable{};
        FD_ZERO(&readable);
        for (const int socket_fd : sockets) {
            FD_SET(socket_fd, &readable);
        }
        const int highest_socket = sockets.back();
        if (select(highest_socket + 1, &readable, nullptr, nullptr, &timeout) <= 0) {
            continue;
        }

        if (FD_ISSET(qr_socket, &readable)) {
            handle_qr_packet(qr_socket, config.gamespy_secret_key);
        }
        if (FD_ISSET(dns_socket, &readable)) {
            handle_dns_packet(dns_socket, config.advertised_address);
        }
        if (FD_ISSET(natneg_socket, &readable)) {
            handle_natneg_packet(natneg_socket);
        }
        if (FD_ISSET(game_socket, &readable)) {
            workers.emplace_back(handle_game_connection, game_socket);
        }
        if (FD_ISSET(nas_socket, &readable)) {
            workers.emplace_back(handle_nas_connection, nas_socket);
        }
        if (FD_ISSET(profile_socket, &readable)) {
            workers.emplace_back(handle_profile_connection, profile_socket);
        }
        if (FD_ISSET(player_search_socket, &readable)) {
            workers.emplace_back(handle_player_search_connection, player_search_socket);
        }
        if (FD_ISSET(relay_socket, &readable)) {
            workers.emplace_back(handle_relay_connection, relay_socket, relay_ssl_context);
        }
        if (FD_ISSET(health_socket, &readable)) {
            const int client_socket = accept(health_socket, nullptr, nullptr);
            if (client_socket >= 0) {
                constexpr char response[] =
                    "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n"
                    "Content-Length: 3\r\nConnection: close\r\n\r\nok\n";
                send_all(client_socket, response, sizeof(response) - 1);
                close(client_socket);
            }
        }
    }

    for (std::thread &worker : workers) {
        worker.join();
    }
    SSL_CTX_free(relay_ssl_context);
    close_sockets(sockets);
    std::cout << "server stopped\n";
    return 0;
}

} // namespace mkwii
