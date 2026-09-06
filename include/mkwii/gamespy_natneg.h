#ifndef MKWII_GAMESPY_NATNEG_H
#define MKWII_GAMESPY_NATNEG_H

#include <cstdint>
#include <string>
#include <vector>

namespace mkwii {

struct NatNegClient {
	std::uint32_t session_id;
	std::uint8_t client_index;
	std::string game_name;
	std::uint32_t address;
	std::uint16_t port;
};

bool is_natneg_init(const std::vector<std::uint8_t> &packet);
bool parse_natneg_init(const std::vector<std::uint8_t> &packet,
	NatNegClient &client);
std::vector<std::uint8_t> natneg_init_ack(
	const std::vector<std::uint8_t> &init_packet);
std::vector<std::uint8_t> natneg_connect(const std::vector<std::uint8_t> &init_packet,
	const NatNegClient &peer);

}  // namespace mkwii

#endif  // MKWII_GAMESPY_NATNEG_H