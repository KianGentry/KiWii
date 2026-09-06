#include "mkwii/gamespy_natneg.h"

#include <cassert>
#include <cstdint>
#include <vector>

int main() {
	const std::vector<std::uint8_t> init = {
		0xfd, 0xfc, 0x1e, 0x66, 0x6a, 0xb2, 0x03, 0x00,
		0x3d, 0xf1, 0x00, 0x71, 0x00, 0x01, 0x01,
		0x0a, 0x00, 0x01, 0xe2, 0x00, 0x00,
		'm', 'a', 'r', 'i', 'o', 'k', 'a', 'r', 't', 'w', 'i', 'i', 0x00};

	assert(mkwii::is_natneg_init(init));
	mkwii::NatNegClient client{};
	assert(mkwii::parse_natneg_init(init, client));
	assert(client.session_id == 0x7100f13d);
	assert(client.client_index == 1);
	assert(client.address == 0xe201000a);
	assert(client.port == 0);
	assert(client.game_name == "mariokartwii");

	assert((mkwii::natneg_init_ack(init) == std::vector<std::uint8_t>{
		0xfd, 0xfc, 0x1e, 0x66, 0x6a, 0xb2, 0x03, 0x01,
		0x3d, 0xf1, 0x00, 0x71, 0x00, 0x01,
		0xff, 0xff, 0x6d, 0x16, 0xb5, 0x7d, 0xea}));

	const mkwii::NatNegClient peer{0x7100f13d, 0, "mariokartwii", 0x04030201, 0x1234};
	assert((mkwii::natneg_connect(init, peer) == std::vector<std::uint8_t>{
		0xfd, 0xfc, 0x1e, 0x66, 0x6a, 0xb2, 0x03, 0x05,
		0x3d, 0xf1, 0x00, 0x71, 0x01, 0x02, 0x03, 0x04,
		0x12, 0x34, 0x42, 0x00}));

	const std::vector<std::uint8_t> connect_ack = {
		0xfd, 0xfc, 0x1e, 0x66, 0x6a, 0xb2, 0x03, 0x06,
		0x3d, 0xf1, 0x00, 0x71, 0x90, 0x00, 0xcd, 0xa0,
		0x80, 0x00, 0x00, 0x90, 0x00};
	assert(mkwii::natneg_connect_ack(connect_ack) == connect_ack);

	const std::vector<std::uint8_t> address_check(64, 0x00);
	std::vector<std::uint8_t> address_packet = address_check;
	address_packet[0] = 0xfd;
	address_packet[1] = 0xfc;
	address_packet[2] = 0x1e;
	address_packet[3] = 0x66;
	address_packet[4] = 0x6a;
	address_packet[5] = 0xb2;
	address_packet[6] = 0x03;
	address_packet[7] = 0x0a;
	const std::vector<std::uint8_t> address_reply =
		mkwii::natneg_address_reply(address_packet, 0x04030201, 0x1234);
	assert(address_reply[7] == 0x0b);
	assert(address_reply[15] == 0x01 && address_reply[18] == 0x04);
	assert(address_reply[19] == 0x12 && address_reply[20] == 0x34);

	std::vector<std::uint8_t> invalid = init;
	invalid[7] = 0x01;
	assert(!mkwii::is_natneg_init(invalid));
	return 0;
}