#include "mkwii/server_internal.h"

#include <algorithm>
#include <array>
#include <random>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>

namespace mkwii {
namespace {

constexpr std::string_view crypt_key = "GameSpy3D";

std::string random_challenge() {
    static std::mt19937 generator(std::random_device{}());
    constexpr std::string_view alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::uniform_int_distribution<std::size_t> distribution(0, alphabet.size() - 1);
    std::string challenge;
    challenge.reserve(10);
    for (int index = 0; index < 10; ++index) {
        challenge += alphabet[distribution(generator)];
    }
    return challenge;
}

std::string gamespy_message(const std::string &message) {
    std::string output = message;
    const std::size_t final_start = output.find("\\final\\");
    const std::size_t encrypted_size = final_start == std::string::npos
        ? output.size() : final_start;
    for (std::size_t index = 0; index < encrypted_size; ++index) {
        output[index] ^= crypt_key[index % crypt_key.size()];
    }
    return output;
}

bool send_gamespy_message(int socket_fd, const std::string &message) {
    const std::string encrypted = gamespy_message(message);
    return send_all(socket_fd, encrypted.data(), encrypted.size());
}

void handle_message(int client_socket, const std::string &encrypted_message) {
    const std::string message = gamespy_message(encrypted_message);
    if (message == "\\ka\\\\final\\") {
        send_gamespy_message(client_socket, "\\ka\\\\final\\");
    }
}

} // namespace

void handle_gamestats_connection(int gamestats_socket) {
    const int client_socket = accept(gamestats_socket, nullptr, nullptr);
    if (client_socket < 0) {
        return;
    }

    const std::string challenge =
        "\\lc\\1\\challenge\\" + random_challenge() + "\\id\\1\\final\\";
    if (!send_gamespy_message(client_socket, challenge)) {
        close(client_socket);
        return;
    }

    std::string request_buffer;
    std::array<char, 4096> request_chunk{};
    while (true) {
        const ssize_t packet_size = recv(client_socket, request_chunk.data(),
                                         request_chunk.size(), 0);
        if (packet_size <= 0) {
            break;
        }
        request_buffer.append(request_chunk.data(), static_cast<std::size_t>(packet_size));
        while (true) {
            const std::size_t message_end = request_buffer.find("\\final\\");
            if (message_end == std::string::npos) {
                break;
            }
            const std::size_t message_size = message_end + 7;
            const std::string message = request_buffer.substr(0, message_size);
            request_buffer.erase(0, message_size);
            handle_message(client_socket, message);
        }
    }
    close(client_socket);
}

} // namespace mkwii
