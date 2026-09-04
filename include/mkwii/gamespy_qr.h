#ifndef MKWII_GAMESPY_QR_H
#define MKWII_GAMESPY_QR_H

#include <cstdint>
#include <string>
#include <vector>

namespace mkwii {

bool is_mariokartwii_availability_request(const std::vector<std::uint8_t>& packet);

bool is_mariokartwii_heartbeat(const std::vector<std::uint8_t>& packet);

std::uint32_t qr_session_id(const std::vector<std::uint8_t>& packet);

std::string qr_field_value(const std::vector<std::uint8_t>& packet,
	const std::string& key);

std::vector<std::uint8_t> availability_response();

}  // namespace mkwii

#endif  // MKWII_GAMESPY_QR_H