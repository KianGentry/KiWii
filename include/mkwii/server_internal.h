#ifndef MKWII_SERVER_INTERNAL_H
#define MKWII_SERVER_INTERNAL_H

#include <cstdint>
#include <string>

#include <openssl/ssl.h>

namespace mkwii {

bool send_all(int socket_fd, const char *data, std::size_t size);
void set_receive_timeout(int socket_fd, int seconds);
int open_tcp_socket(std::uint16_t port);
int open_udp_socket(std::uint16_t port);
SSL_CTX *create_relay_ssl_context();
void handle_dns_packet(int dns_socket, const std::string &address);
void handle_qr_packet(int qr_socket, const std::string &secret_key);
void handle_relay_connection(int relay_socket, SSL_CTX *ssl_context);
void handle_player_search_connection(int player_search_socket);
void handle_profile_connection(int profile_socket);
void handle_nas_connection(int nas_socket);
void handle_natneg_packet(int natneg_socket);
void handle_game_connection(int game_socket);

} // namespace mkwii

#endif
