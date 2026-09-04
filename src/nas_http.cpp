#include "mkwii/nas_http.h"

#include <chrono>
#include <ctime>
#include <random>
#include <string_view>
#include <unordered_map>

namespace mkwii {
namespace {

std::string random_text(std::size_t length) {
    static std::mt19937 generator(std::random_device{}());
    constexpr std::string_view alphabet =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::uniform_int_distribution<std::size_t> distribution(0, alphabet.size() - 1);

    std::string value;
    value.reserve(length);
    for (std::size_t index = 0; index < length; ++index) {
        value += alphabet[distribution(generator)];
    }
    return value;
}

std::string current_datetime() {
    const std::time_t now = std::time(nullptr);
    std::tm utc_time{};
    gmtime_r(&now, &utc_time);

    char value[15];
    std::strftime(value, sizeof(value), "%Y%m%d%H%M%S", &utc_time);
    return value;
}

std::string request_user_id(const std::string& request) {
    const std::size_t body_start = request.find("\r\n\r\n");
    const std::size_t user_id_start = request.find("userid=", body_start);
    if (user_id_start == std::string::npos) {
        return {};
    }

    const std::size_t value_start = user_id_start + 7;
    const std::size_t value_end = request.find('&', value_start);
    return request.substr(value_start, value_end - value_start);
}

struct LoginSession {
    std::string challenge;
    std::string token;
};

std::unordered_map<std::string, LoginSession> login_sessions;

}  // namespace

std::string nas_connectivity_response() {
    return "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/html\r\n"
           "Content-Length: 2\r\n"
           "X-Organization: Nintendo\r\n"
           "Server: BigIP\r\n"
           "Connection: close\r\n"
           "\r\n"
           "ok";
}

std::string nas_response_for_request(const std::string& request) {
    if (request.find("POST /ac ") == std::string::npos ||
        request.find("action=bG9naW4%2A") == std::string::npos) {
        return nas_connectivity_response();
    }

    const std::string user_id = request_user_id(request);
    const LoginSession session{
        random_text(8),
        "NDS" + random_text(80),
    };
    login_sessions[user_id] = session;

    const std::string login_body =
        "retry=0&returncd=001&locator=gamespy.com&challenge=" +
        session.challenge + "&token=" + session.token +
        "&datetime=" + current_datetime();
    return "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: " + std::to_string(login_body.size()) + "\r\n"
           "NODE: wifiappe1\r\n"
           "Connection: close\r\n"
           "\r\n" + login_body;
}

}  // namespace mkwii