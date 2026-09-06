#include "mkwii/gamespy_browser.h"

#include <arpa/inet.h>

namespace mkwii {
namespace {

std::string read_c_string(const std::vector<std::uint8_t> &packet,
	std::size_t &offset) {
	const std::size_t start = offset;
	while (offset < packet.size() && packet[offset] != 0) {
		++offset;
	}
	if (offset == packet.size()) {
		return {};
	}
	++offset;
	return std::string(packet.begin() + start, packet.begin() + offset - 1);
}

void append_u16_be(std::vector<std::uint8_t> &output, std::uint16_t value) {
	output.push_back(static_cast<std::uint8_t>(value >> 8));
	output.push_back(static_cast<std::uint8_t>(value & 0xff));
}

}  // namespace

bool parse_browser_request(const std::vector<std::uint8_t> &packet,
	BrowserRequest &request) {
	if (packet.size() < 15 || packet[2] != 0x00) {
		return false;
	}
	const std::size_t declared_size =
		(static_cast<std::size_t>(packet[0]) << 8) | packet[1];
	if (declared_size != packet.size()) {
		return false;
	}
	std::size_t offset = 3 + 1 + 1 + 4;
	const std::string query_game = read_c_string(packet, offset);
	request.game_name = read_c_string(packet, offset);
	if (query_game.empty() || request.game_name.empty() || offset + 8 > packet.size()) {
		return false;
	}
	request.challenge.assign(packet.begin() + offset, packet.begin() + offset + 8);
	offset += 8;
	request.filter = read_c_string(packet, offset);
	const std::string field_text = read_c_string(packet, offset);
	if (offset + 4 > packet.size()) {
		return false;
	}
	std::size_t field_start = 0;
	while (field_start < field_text.size()) {
		const std::size_t separator = field_text.find('\\', field_start);
		const std::string field = field_text.substr(
			field_start, separator == std::string::npos ? std::string::npos
			                                           : separator - field_start);
		if (!field.empty()) {
			request.fields.push_back(field);
		}
		if (separator == std::string::npos) {
			break;
		}
		field_start = separator + 1;
	}
	return true;
}

std::vector<std::uint8_t> browser_empty_server_list(
	const BrowserRequest &request, const std::string &address,
	std::uint16_t port) {
	in_addr parsed_address{};
	if (inet_pton(AF_INET, address.c_str(), &parsed_address) != 1) {
		return {};
	}
	std::vector<std::uint8_t> response{
		static_cast<std::uint8_t>((port >> 8) & 0xff),
		static_cast<std::uint8_t>(port & 0xff)};
	const auto *address_bytes = reinterpret_cast<const std::uint8_t *>(&parsed_address.s_addr);
	response.insert(response.begin(), address_bytes, address_bytes + 4);
	append_u16_be(response, static_cast<std::uint16_t>(request.fields.size()));
	for (const std::string &field : request.fields) {
		response.insert(response.end(), field.begin(), field.end());
		response.push_back(0x00);
		response.push_back(0x00);
	}
	response.push_back(0x00);
	response.insert(response.end(), {0xff, 0xff, 0xff, 0xff});
	return response;
}

}  // namespace mkwii