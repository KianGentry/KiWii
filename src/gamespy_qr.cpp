#include "mkwii/gamespy_qr.h"

#include <algorithm>
#include <iomanip>
#include <random>
#include <sstream>

namespace mkwii {
namespace {

constexpr std::uint8_t availability_command = 0x09;
constexpr std::uint8_t heartbeat_command = 0x03;
constexpr std::uint8_t zero_session_id[] = {0x00, 0x00, 0x00, 0x00};
constexpr char game_name[] = "mariokartwii";

std::string challenge_text() {
    static std::mt19937 generator(std::random_device{}());
    constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::uniform_int_distribution<std::size_t> distribution(0, sizeof(alphabet) - 2);

    std::string value;
    for (int index = 0; index < 6; ++index) {
        value += alphabet[distribution(generator)];
    }
    return value;
}

}  // namespace

// verify packet is a valid mario kart wii availability request, checking command byte, session id, and game name
bool is_mariokartwii_availability_request(const std::vector<std::uint8_t>& packet) {
    constexpr std::size_t expected_size = 1 + sizeof(zero_session_id) + sizeof(game_name);
    if (packet.size() != expected_size || packet[0] != availability_command) {
        return false;
    }

    if (!std::equal(std::begin(zero_session_id), std::end(zero_session_id), packet.begin() + 1)) {
        return false;
    }

    return std::equal(std::begin(game_name), std::end(game_name), packet.begin() + 5);
}

bool is_mariokartwii_heartbeat(const std::vector<std::uint8_t>& packet) {
    if (packet.size() < 5 || packet[0] != heartbeat_command) {
        return false;
    }

    const std::string payload(packet.begin() + 5, packet.end());
    const std::string local_ip_marker = std::string("localip0") + '\0';
    const std::string game_name_marker = std::string("gamename") + '\0' + game_name + '\0';
    return payload.find(local_ip_marker) == 0 &&
        payload.find(game_name_marker) != std::string::npos;
}

std::uint32_t qr_session_id(const std::vector<std::uint8_t>& packet) {
    if (packet.size() < 5) {
        return 0;
    }
    return static_cast<std::uint32_t>(packet[1]) |
        (static_cast<std::uint32_t>(packet[2]) << 8) |
        (static_cast<std::uint32_t>(packet[3]) << 16) |
        (static_cast<std::uint32_t>(packet[4]) << 24);
}

std::string qr_field_value(const std::vector<std::uint8_t>& packet,
    const std::string& key) {
    if (packet.size() <= 5) {
        return {};
    }
    const std::string payload(packet.begin() + 5, packet.end());
    const std::string marker = key + '\0';
    const std::size_t value_start = payload.find(marker);
    if (value_start == std::string::npos) {
        return {};
    }
    const std::size_t first = value_start + marker.size();
    const std::size_t value_end = payload.find('\0', first);
    return payload.substr(first, value_end - first);
}

// generate GameSpy QR availability response to indicate this server is running
std::vector<std::uint8_t> availability_response() {
    return {0xfe, 0xfd, 0x09, 0x00, 0x00, 0x00, 0x00};
}

std::vector<std::uint8_t> qr_challenge_response(std::uint32_t session_id,
    const std::string& address,
    std::uint16_t port) {
    std::istringstream address_stream(address);
    unsigned int first = 0;
    unsigned int second = 0;
    unsigned int third = 0;
    unsigned int fourth = 0;
    char separator = 0;
    address_stream >> first >> separator >> second >> separator >> third >> separator >> fourth;

    std::ostringstream challenge;
    challenge << challenge_text() << "00"
            << std::uppercase << std::hex << std::setfill('0')
            << std::setw(2) << first << std::setw(2) << second
            << std::setw(2) << third << std::setw(2) << fourth
            << std::setw(4) << port;

    std::vector<std::uint8_t> response = {
        0xfe,
        0xfd,
        0x01,
        static_cast<std::uint8_t>(session_id & 0xff),
        static_cast<std::uint8_t>((session_id >> 8) & 0xff),
        static_cast<std::uint8_t>((session_id >> 16) & 0xff),
        static_cast<std::uint8_t>((session_id >> 24) & 0xff),
    };
    const std::string challenge_value = challenge.str();
    response.insert(response.end(), challenge_value.begin(), challenge_value.end());
    response.push_back(0x00);
    return response;
}

std::vector<std::uint8_t> qr_registered_response(std::uint32_t session_id) {
    return {
        0xfe,
        0xfd,
        0x0a,
        static_cast<std::uint8_t>(session_id & 0xff),
        static_cast<std::uint8_t>((session_id >> 8) & 0xff),
        static_cast<std::uint8_t>((session_id >> 16) & 0xff),
        static_cast<std::uint8_t>((session_id >> 24) & 0xff),
    };
}

}  // namespace mkwii