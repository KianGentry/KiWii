#include "mkwii/gamespy_dns.h"

#include <sstream>

namespace mkwii {

std::vector<std::uint8_t> dns_a_response(
	const std::vector<std::uint8_t> &request, const std::string &address) {
	if (request.size() < 12) {
		return {};
	}

	std::size_t question_end = 12;
	while (question_end < request.size() && request[question_end] != 0) {
		const std::size_t label_length = request[question_end];
		if (label_length > 63 || question_end + label_length + 1 >= request.size()) {
			return {};
		}
		question_end += label_length + 1;
	}
	if (question_end + 5 > request.size()) {
		return {};
	}
	question_end += 5;

	std::istringstream address_stream(address);
	unsigned int octets[4]{};
	char separator = 0;
	if (!(address_stream >> octets[0] >> separator >> octets[1] >> separator >>
		  octets[2] >> separator >> octets[3]) ||
		separator != '.' || octets[0] > 255 || octets[1] > 255 ||
		octets[2] > 255 || octets[3] > 255) {
		return {};
	}
	address_stream >> std::ws;
	if (!address_stream.eof()) {
		return {};
	}

	std::vector<std::uint8_t> response(request.begin(), request.begin() + question_end);
	response[2] = 0x81;
	response[3] = 0x80;
	response[4] = 0x00;
	response[5] = 0x01;
	response[6] = 0x00;
	response[7] = 0x01;
	response[8] = response[9] = response[10] = response[11] = 0x00;
	response.insert(response.end(), {0xc0, 0x0c, 0x00, 0x01, 0x00, 0x01,
		0x00, 0x00, 0x00, 0x3c, 0x00, 0x04,
		static_cast<std::uint8_t>(octets[0]), static_cast<std::uint8_t>(octets[1]),
		static_cast<std::uint8_t>(octets[2]), static_cast<std::uint8_t>(octets[3])});
	return response;
}

}  // namespace mkwii