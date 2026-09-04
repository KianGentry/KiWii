#include "mkwii/gamespy_profile.h"

#include <random>
#include <string_view>

namespace mkwii {
namespace {

constexpr char keepalive_message[] = "\\ka\\\\final\\";

std::string random_challenge() {
    static std::mt19937 generator(std::random_device{}());
    constexpr std::string_view alphabet =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::uniform_int_distribution<std::size_t> distribution(0, alphabet.size() - 1);

    std::string challenge;
    challenge.reserve(8);
    for (std::size_t index = 0; index < 8; ++index) {
        challenge += alphabet[distribution(generator)];
    }
    return challenge;
}

}  // namespace

// check if this profile message is a keepalive ping
bool is_profile_keepalive(const std::string& request) {
    return request == keepalive_message;
}

// return static keepalive response to acknowledge connection is still alive
const std::string& profile_keepalive_response() {
    static const std::string response = keepalive_message;
    return response;
}

// generate login challenge message with random 8-character string for client authentication
std::string profile_login_challenge() {
    return "\\lc\\1\\challenge\\" + random_challenge() + "\\id\\1\\final\\";
}

}  // namespace mkwii