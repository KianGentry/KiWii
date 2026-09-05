#include "mkwii/gamespy_qr.h"

#include <algorithm>
#include <iomanip>
#include <random>
#include <sstream>
#include <string_view>

namespace mkwii {
namespace {

constexpr std::uint8_t availability_command = 0x09;
constexpr std::uint8_t heartbeat_command = 0x03;
constexpr std::uint8_t zero_session_id[] = {0x00, 0x00, 0x00, 0x00};
constexpr char game_name[] = "mariokartwii";

std::string challenge_text() {
	static std::mt19937 generator(std::random_device{}());
	constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	std::uniform_int_distribution<std::size_t> distribution(
		0, sizeof(alphabet) - 2);

	std::string value;
	for (int index = 0; index < 6; ++index) {
		value += alphabet[distribution(generator)];
	}
	return value;
}

std::vector<std::uint8_t>
rc4_transform(std::string_view key, const std::vector<std::uint8_t> &input) {
	std::vector<std::uint8_t> state(256);
	for (std::size_t index = 0; index < state.size(); ++index) {
		state[index] = static_cast<std::uint8_t>(index);
	}

	std::size_t swap_index = 0;
	for (std::size_t index = 0; index < state.size(); ++index) {
		swap_index = (swap_index + state[index] +
					  static_cast<std::uint8_t>(key[index % key.size()])) &
					 0xff;
		std::swap(state[index], state[swap_index]);
	}

	std::vector<std::uint8_t> output(input.begin(), input.end());
	std::size_t first = 0;
	std::size_t second = 0;
	for (std::uint8_t &byte : output) {
		first = (first + 1 + byte) & 0xff;
		second = (second + state[first]) & 0xff;
		std::swap(state[first], state[second]);
		byte ^= state[(state[first] + state[second]) & 0xff];
	}
	return output;
}

std::string base64_encode(const std::vector<std::uint8_t> &input) {
	constexpr char alphabet[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string output;
	for (std::size_t index = 0; index < input.size(); index += 3) {
		const std::size_t remaining = input.size() - index;
		const std::uint32_t value =
			static_cast<std::uint32_t>(input[index]) << 16 |
			(remaining > 1 ? static_cast<std::uint32_t>(input[index + 1]) << 8
						   : 0) |
			(remaining > 2 ? input[index + 2] : 0);
		output += alphabet[(value >> 18) & 0x3f];
		output += alphabet[(value >> 12) & 0x3f];
		output += remaining > 1 ? alphabet[(value >> 6) & 0x3f] : '=';
		output += remaining > 2 ? alphabet[value & 0x3f] : '=';
	}
	return output;
}

} // namespace

// verify packet is a valid mario kart wii availability request, checking
// command byte, session id, and game name
bool is_mariokartwii_availability_request(
	const std::vector<std::uint8_t> &packet) {
	constexpr std::size_t expected_size =
		1 + sizeof(zero_session_id) + sizeof(game_name);
	if (packet.size() != expected_size || packet[0] != availability_command) {
		return false;
	}

	if (!std::equal(std::begin(zero_session_id), std::end(zero_session_id),
					packet.begin() + 1)) {
		return false;
	}

	return std::equal(std::begin(game_name), std::end(game_name),
					  packet.begin() + 5);
}

bool is_mariokartwii_heartbeat(const std::vector<std::uint8_t> &packet) {
	if (packet.size() < 5 || packet[0] != heartbeat_command) {
		return false;
	}

	const std::string payload(packet.begin() + 5, packet.end());
	const std::string local_ip_marker = std::string("localip0") + '\0';
	const std::string game_name_marker =
		std::string("gamename") + '\0' + game_name + '\0';
	return payload.find(local_ip_marker) == 0 &&
		   payload.find(game_name_marker) != std::string::npos;
}

std::uint32_t qr_session_id(const std::vector<std::uint8_t> &packet) {
	if (packet.size() < 5) {
		return 0;
	}
	return static_cast<std::uint32_t>(packet[1]) |
		   (static_cast<std::uint32_t>(packet[2]) << 8) |
		   (static_cast<std::uint32_t>(packet[3]) << 16) |
		   (static_cast<std::uint32_t>(packet[4]) << 24);
}

std::string qr_field_value(const std::vector<std::uint8_t> &packet,
						   const std::string &key) {
	if (packet.size() <= 5) {
		return {};
	}
	const std::string payload(packet.begin() + 5, packet.end());
	const std::string marker = key + '\0';
	const std::size_t value_start = payload.find(marker);
	if (value_start == std::string::npos) {
		return {};
	}
	const std::size_t first = value_start + marker.size();
	const std::size_t value_end = payload.find('\0', first);
	return payload.substr(first, value_end - first);
}

// generate GameSpy QR availability response to indicate this server is running
std::vector<std::uint8_t> availability_response() {
	return {0xfe, 0xfd, 0x09, 0x00, 0x00, 0x00, 0x00};
}

std::vector<std::uint8_t> qr_challenge_response(std::uint32_t session_id,
												const std::string &address,
												std::uint16_t port) {
	std::istringstream address_stream(address);
	unsigned int first = 0;
	unsigned int second = 0;
	unsigned int third = 0;
	unsigned int fourth = 0;
	char separator = 0;
	address_stream >> first >> separator >> second >> separator >> third >>
		separator >> fourth;

	std::ostringstream challenge;
	challenge << challenge_text() << "00" << std::uppercase << std::hex
			  << std::setfill('0') << std::setw(2) << first << std::setw(2)
			  << second << std::setw(2) << third << std::setw(2) << fourth
			  << std::setw(4) << port;

	std::vector<std::uint8_t> response = {
		0xfe,
		0xfd,
		0x01,
		static_cast<std::uint8_t>(session_id & 0xff),
		static_cast<std::uint8_t>((session_id >> 8) & 0xff),
		static_cast<std::uint8_t>((session_id >> 16) & 0xff),
		static_cast<std::uint8_t>((session_id >> 24) & 0xff),
	};
	const std::string challenge_value = challenge.str();
	response.insert(response.end(), challenge_value.begin(),
					challenge_value.end());
	response.push_back(0x00);
	return response;
}

std::vector<std::uint8_t> qr_registered_response(std::uint32_t session_id) {
	return {
		0xfe,
		0xfd,
		0x0a,
		static_cast<std::uint8_t>(session_id & 0xff),
		static_cast<std::uint8_t>((session_id >> 8) & 0xff),
		static_cast<std::uint8_t>((session_id >> 16) & 0xff),
		static_cast<std::uint8_t>((session_id >> 24) & 0xff),
	};
}

bool qr_challenge_matches(const std::vector<std::uint8_t> &packet,
						  const std::string &challenge,
						  const std::string &secret_key) {
	if (packet.size() < 7 || packet[0] != 0x01 || secret_key.empty()) {
		return false;
	}

	std::vector<std::uint8_t> plaintext(challenge.begin(), challenge.end());
	plaintext.push_back(0x00);
	const std::vector<std::uint8_t> encrypted =
		rc4_transform(secret_key, plaintext);
	const std::string expected = base64_encode(encrypted);
	const auto response_begin = packet.begin() + 5;
	const auto response_end =
		packet.back() == 0x00 ? packet.end() - 1 : packet.end();
	return std::string(response_begin, response_end) == expected;
}

std::vector<std::uint8_t>
qr_client_challenge_response(std::uint32_t session_id,
							 const std::string &challenge,
							 const std::string &secret_key) {
	if (secret_key.empty()) {
		return {};
	}
	std::vector<std::uint8_t> plaintext(challenge.begin(), challenge.end());
	plaintext.push_back(0x00);
	const std::string encoded =
		base64_encode(rc4_transform(secret_key, plaintext));
	std::vector<std::uint8_t> response = {
		0x01,
		static_cast<std::uint8_t>(session_id & 0xff),
		static_cast<std::uint8_t>((session_id >> 8) & 0xff),
		static_cast<std::uint8_t>((session_id >> 16) & 0xff),
		static_cast<std::uint8_t>((session_id >> 24) & 0xff),
	};
	response.insert(response.end(), encoded.begin(), encoded.end());
	response.push_back(0x00);
	return response;
}

} // namespace mkwii