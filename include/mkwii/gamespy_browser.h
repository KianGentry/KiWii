#ifndef MKWII_GAMESPY_BROWSER_H
#define MKWII_GAMESPY_BROWSER_H

#include <cstdint>
#include <string>
#include <vector>

namespace mkwii {

struct BrowserRequest {
	std::string game_name;
	std::string challenge;
	std::string filter;
	std::vector<std::string> fields;
};

bool parse_browser_request(const std::vector<std::uint8_t> &packet,
	BrowserRequest &request);
std::vector<std::uint8_t> browser_empty_server_list(
	const BrowserRequest &request, const std::string &address,
	std::uint16_t port);

}  // namespace mkwii

#endif  // MKWII_GAMESPY_BROWSER_H