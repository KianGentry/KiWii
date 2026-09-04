#ifndef MKWII_GAMESPY_QR_H
#define MKWII_GAMESPY_QR_H

#include <cstdint>
#include <vector>

namespace mkwii {

bool is_mariokartwii_availability_request(const std::vector<std::uint8_t>& packet);

std::vector<std::uint8_t> availability_response();

}  // namespace mkwii

#endif  // MKWII_GAMESPY_QR_H