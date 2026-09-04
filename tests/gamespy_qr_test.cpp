#include "mkwii/gamespy_qr.h"

#include <cassert>
#include <cstdint>
#include <vector>

int main() {
    const std::vector<std::uint8_t> request = {
        0x09, 0x00, 0x00, 0x00, 0x00,
        'm', 'a', 'r', 'i', 'o', 'k', 'a', 'r', 't', 'w', 'i', 'i', 0x00,
    };

    assert(mkwii::is_mariokartwii_availability_request(request));
    assert((mkwii::availability_response() ==
            std::vector<std::uint8_t>{0xfe, 0xfd, 0x09, 0x00, 0x00, 0x00, 0x00}));

    const std::vector<std::uint8_t> heartbeat = {
        0x03, 0xef, 0x95, 0x37, 0x05,
        'l', 'o', 'c', 'a', 'l', 'i', 'p', '0', 0x00,
        '1', '9', '2', '.', '1', '6', '8', '.', '1', '.', '8', '4', 0x00,
        'l', 'o', 'c', 'a', 'l', 'p', 'o', 'r', 't', 0x00, '6', '0', '6', '1', '7', 0x00,
        'n', 'a', 't', 'n', 'e', 'g', 0x00, '1', 0x00,
        's', 't', 'a', 't', 'e', 'c', 'h', 'a', 'n', 'g', 'e', 'd', 0x00, '1', 0x00,
        'g', 'a', 'm', 'e', 'n', 'a', 'm', 'e', 0x00,
        'm', 'a', 'r', 'i', 'o', 'k', 'a', 'r', 't', 'w', 'i', 'i', 0x00,
    };
    assert(mkwii::is_mariokartwii_heartbeat(heartbeat));
    assert(mkwii::qr_session_id(heartbeat) == 0x053795ef);
    assert(mkwii::qr_field_value(heartbeat, "statechanged") == "1");
    assert(mkwii::qr_field_value(heartbeat, "gamename") == "mariokartwii");

    std::vector<std::uint8_t> wrong_command = request;
    wrong_command[0] = 0x00;
    assert(!mkwii::is_mariokartwii_availability_request(wrong_command));

    std::vector<std::uint8_t> wrong_game = request;
    wrong_game[5] = 'M';
    assert(!mkwii::is_mariokartwii_availability_request(wrong_game));

    std::vector<std::uint8_t> truncated = request;
    truncated.pop_back();
    assert(!mkwii::is_mariokartwii_availability_request(truncated));
    return 0;
}