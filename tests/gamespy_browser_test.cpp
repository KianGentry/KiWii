#include "mkwii/gamespy_browser.h"

#include <cassert>
#include <cstdint>
#include <vector>

int main() {
	std::vector<std::uint8_t> request = {
		0x00, 0xab, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00,
		'm', 'a', 'r', 'i', 'o', 'k', 'a', 'r', 't', 'w', 'i', 'i', 0x00,
		'm', 'a', 'r', 'i', 'o', 'k', 'a', 'r', 't', 'w', 'i', 'i', 0x00,
		'g', 'S', '.', 'k', '{', 'H', 'X', '?', 'd', 'w', 'c', '_', 'p', 'i', 'd', ' ', '=', ' ', '1', 0x00,
		'\\', 'n', 'u', 'm', 'p', 'l', 'a', 'y', 'e', 'r', 's', '\\', 'm', 'a', 'x', 'p', 'l', 'a', 'y', 'e', 'r', 's', 0x00,
		'd', 'w', 'c', '_', 'p', 'i', 'd', '\\', 'd', 'w', 'c', '_', 'm', 't', 'y', 'p', 'e', 0x00,
		0x00, 0x00, 0x00, 0x00};
	request[0] = static_cast<std::uint8_t>(request.size() >> 8);
	request[1] = static_cast<std::uint8_t>(request.size() & 0xff);
	assert(request.size() > 20);
	mkwii::BrowserRequest parsed{};
	assert(mkwii::parse_browser_request(request, parsed));
	assert(parsed.game_name == "mariokartwii");
	assert(parsed.challenge == "gS.k{HX?");
	assert(parsed.fields.size() == 2);
	assert(parsed.fields[0] == "numplayers");
	assert(parsed.fields[1] == "maxplayers");
	assert(!mkwii::browser_empty_server_list(parsed, "192.168.1.200", 28910).empty());

	std::vector<std::uint8_t> invalid = request;
	invalid[1] = 0xaa;
	assert(!mkwii::parse_browser_request(invalid, parsed));
	return 0;
}