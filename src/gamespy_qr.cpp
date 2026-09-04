#include "mkwii/gamespy_qr.h"

#include <algorithm>

namespace mkwii {
namespace {

constexpr std::uint8_t availability_command = 0x09;
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

// generate GameSpy QR availability response to indicate this server is running
std::vector<std::uint8_t> availability_response() {
    return {0xfe, 0xfd, 0x09, 0x00, 0x00, 0x00, 0x00};
}

}  // namespace mkwii