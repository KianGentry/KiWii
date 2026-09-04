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