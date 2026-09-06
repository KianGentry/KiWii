#include "mkwii/gamespy_dns.h"

#include <cassert>
#include <cstdint>
#include <vector>

int main() {
	const std::vector<std::uint8_t> request = {
		0x12, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x03, 'n', 'a', 's', 0x00, 0x00, 0x01, 0x00, 0x01};
	const std::vector<std::uint8_t> response =
		mkwii::dns_a_response(request, "192.168.1.42");
	assert(response.size() == request.size() + 16);
	assert(response[0] == 0x12 && response[1] == 0x34);
	assert(response[2] == 0x81 && response[3] == 0x80);
	assert(response[5] == 0x01 && response[7] == 0x01);
	assert(response[response.size() - 4] == 192);
	assert(response[response.size() - 3] == 168);
	assert(response[response.size() - 2] == 1);
	assert(response[response.size() - 1] == 42);

	std::vector<std::uint8_t> truncated(request.begin(), request.begin() + 11);
	assert(mkwii::dns_a_response(truncated, "192.168.1.42").empty());
	assert(mkwii::dns_a_response(request, "256.168.1.42").empty());
	assert(mkwii::dns_a_response(request, "192.168.1.42.extra").empty());
	return 0;
}