#include "mkwii/gamespy_profile.h"

#include <random>
#include <sstream>
#include <string_view>

namespace mkwii {
namespace {

constexpr char keepalive_message[] = "\\ka\\\\final\\";
constexpr std::string_view text_alphabet =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

std::string random_text(std::size_t length, std::string_view alphabet) {
    static std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<std::size_t> distribution(0, alphabet.size() - 1);

    std::string value;
    value.reserve(length);
    for (std::size_t index = 0; index < length; ++index) {
        value += alphabet[distribution(generator)];
    }
    return value;
}

std::string random_hex(std::size_t length) {
    return random_text(length, "0123456789abcdef");
}

std::string request_value(const std::string& request, std::string_view key) {
    const std::string search = "\\" + std::string(key) + "\\";
    const std::size_t value_start = request.find(search);
    if (value_start == std::string::npos) {
        return {};
    }

    const std::size_t first = value_start + search.size();
    const std::size_t last = request.find('\\', first);
    return request.substr(first, last - first);
}

}  // namespace

bool is_profile_keepalive(const std::string& request) {
    return request == keepalive_message;
}

bool is_profile_login(const std::string& request) {
    return request.starts_with("\\login\\") && request.ends_with("\\final\\");
}

const std::string& profile_keepalive_response() {
    static const std::string response = keepalive_message;
    return response;
}

std::string profile_login_challenge() {
    return "\\lc\\1\\challenge\\" + random_text(10, "ABCDEFGHIJKLMNOPQRSTUVWXYZ") +
           "\\id\\1\\final\\";
}

std::string profile_login_response(const std::string& request) {
    const std::string session_key = random_text(8, "0123456789");
    const std::string login_ticket = random_text(16, text_alphabet);
    const std::string user_id = "0000000000002";
    const std::string request_id = request_value(request, "id");

    std::ostringstream response;
    response << "\\lc\\2"
            << "\\sesskey\\" << session_key
            << "\\proof\\" << random_hex(32)
            << "\\userid\\" << user_id
            << "\\profileid\\1"
            << "\\uniquenick\\KiWii" << user_id
            << "\\lt\\" << login_ticket
            << "\\id\\" << request_id
            << "\\final\\";
    return response.str();
}

}  // namespace mkwii
