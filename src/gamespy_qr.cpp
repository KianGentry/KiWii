#include "mkwii/gamespy_qr.h"

#include <algorithm>

namespace mkwii {
namespace {

constexpr std::uint8_t availability_command = 0x09;
constexpr std::uint8_t heartbeat_command = 0x03;
constexpr std::uint8_t zero_session_id[] = {0x00, 0x00, 0x00, 0x00};
constexpr char game_name[] = "mariokartwii";

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

}  // namespace mkwii