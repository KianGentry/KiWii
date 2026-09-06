#include "mkwii/gamespy_natneg.h"

#include <algorithm>

namespace mkwii {
namespace {

constexpr std::uint8_t magic[] = {0xfd, 0xfc, 0x1e, 0x66, 0x6a, 0xb2};
constexpr std::size_t minimum_init_size = 22;

bool has_natneg_header(const std::vector<std::uint8_t> &packet) {
	return packet.size() >= 14 &&
		std::equal(std::begin(magic), std::end(magic), packet.begin()) &&
		packet[6] == 0x03;
}

std::uint32_t read_u32_le(const std::vector<std::uint8_t> &packet,
	std::size_t offset) {
	return static_cast<std::uint32_t>(packet[offset]) |
		(static_cast<std::uint32_t>(packet[offset + 1]) << 8) |
		(static_cast<std::uint32_t>(packet[offset + 2]) << 16) |
		(static_cast<std::uint32_t>(packet[offset + 3]) << 24);
}

}  // namespace

bool is_natneg_init(const std::vector<std::uint8_t> &packet) {
	return has_natneg_header(packet) && packet[7] == 0x00 &&
		packet.size() >= minimum_init_size;
}

bool parse_natneg_init(const std::vector<std::uint8_t> &packet,
	NatNegClient &client) {
	if (!is_natneg_init(packet)) {
		return false;
	}
	client.session_id = read_u32_le(packet, 8);
	client.client_index = packet[13];
	client.address = read_u32_le(packet, 15);
	client.port = static_cast<std::uint16_t>(packet[19] << 8 | packet[20]);
	const auto game_start = packet.begin() + 21;
	const auto game_end = std::find(game_start, packet.end(), 0x00);
	client.game_name.assign(game_start, game_end);
	return !client.game_name.empty();
}

std::vector<std::uint8_t> natneg_init_ack(
	const std::vector<std::uint8_t> &init_packet) {
	if (!is_natneg_init(init_packet)) {
		return {};
	}
	std::vector<std::uint8_t> response(init_packet.begin(), init_packet.begin() + 14);
	response[7] = 0x01;
	response.insert(response.end(), {0xff, 0xff, 0x6d, 0x16, 0xb5, 0x7d, 0xea});
	return response;
}

std::vector<std::uint8_t> natneg_connect(
	const std::vector<std::uint8_t> &init_packet, const NatNegClient &peer) {
	if (!is_natneg_init(init_packet)) {
		return {};
	}
	std::vector<std::uint8_t> response(init_packet.begin(), init_packet.begin() + 12);
	response[7] = 0x05;
	response.push_back(static_cast<std::uint8_t>(peer.address & 0xff));
	response.push_back(static_cast<std::uint8_t>((peer.address >> 8) & 0xff));
	response.push_back(static_cast<std::uint8_t>((peer.address >> 16) & 0xff));
	response.push_back(static_cast<std::uint8_t>((peer.address >> 24) & 0xff));
	response.push_back(static_cast<std::uint8_t>((peer.port >> 8) & 0xff));
	response.push_back(static_cast<std::uint8_t>(peer.port & 0xff));
	response.insert(response.end(), {0x42, 0x00});
	return response;
}

}  // namespace mkwii