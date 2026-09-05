#include "mkwii/gamespy_qr.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <vector>

int main() {
	const std::vector<std::uint8_t> request = {
		0x09, 0x00, 0x00, 0x00, 0x00, 'm', 'a', 'r', 'i',
		'o',  'k',	'a',  'r',	't',  'w', 'i', 'i', 0x00,
	};

	assert(mkwii::is_mariokartwii_availability_request(request));
	assert(
		(mkwii::availability_response() ==
		 std::vector<std::uint8_t>{0xfe, 0xfd, 0x09, 0x00, 0x00, 0x00, 0x00}));

	const std::vector<std::uint8_t> heartbeat = {
		0x03, 0xef, 0x95, 0x37, 0x05, 'l', 'o', 'c', 'a', 'l', 'i',	 'p', '0',
		0x00, '1',	'9',  '2',	'.',  '1', '6', '8', '.', '1', '.',	 '8', '4',
		0x00, 'l',	'o',  'c',	'a',  'l', 'p', 'o', 'r', 't', 0x00, '6', '0',
		'6',  '1',	'7',  0x00, 'n',  'a', 't', 'n', 'e', 'g', 0x00, '1', 0x00,
		's',  't',	'a',  't',	'e',  'c', 'h', 'a', 'n', 'g', 'e',	 'd', 0x00,
		'1',  0x00, 'g',  'a',	'm',  'e', 'n', 'a', 'm', 'e', 0x00, 'm', 'a',
		'r',  'i',	'o',  'k',	'a',  'r', 't', 'w', 'i', 'i', 0x00,
	};
	assert(mkwii::is_mariokartwii_heartbeat(heartbeat));
	assert(mkwii::qr_session_id(heartbeat) == 0x053795ef);
	assert(mkwii::qr_field_value(heartbeat, "statechanged") == "1");
	assert(mkwii::qr_field_value(heartbeat, "gamename") == "mariokartwii");

	const std::vector<std::uint8_t> challenge =
		mkwii::qr_challenge_response(0x053795ef, "192.168.1.84", 60617);
	assert(challenge.size() == 28);
	assert(challenge[0] == 0xfe);
	assert(challenge[1] == 0xfd);
	assert(challenge[2] == 0x01);
	assert(challenge[3] == 0xef);
	assert(challenge[4] == 0x95);
	assert(challenge[5] == 0x37);
	assert(challenge[6] == 0x05);
	assert(challenge[13] == '0');
	assert(challenge[14] == '0');
	assert(challenge[15] == 'C');
	assert(challenge[16] == '0');
	assert(challenge[17] == 'A');
	assert(challenge[18] == '8');
	assert(challenge[19] == '0');
	assert(challenge[20] == '1');
	assert(challenge[21] == '5');
	assert(challenge[22] == '4');
	assert(challenge[23] == 'E');
	assert(challenge[24] == 'C');
	assert(challenge[25] == 'C');
	assert(challenge[26] == '9');
	assert(challenge[27] == 0x00);

	assert(
		(mkwii::qr_registered_response(0x053795ef) ==
		 std::vector<std::uint8_t>{0xfe, 0xfd, 0x0a, 0xef, 0x95, 0x37, 0x05}));
	const std::string challenge_text(
		challenge.begin() + 7,
		std::find(challenge.begin() + 7, challenge.end(), 0x00));
	const std::vector<std::uint8_t> client_response =
		mkwii::qr_client_challenge_response(0x053795ef, challenge_text,
											"9r3Rmy");
	assert(
		mkwii::qr_challenge_matches(client_response, challenge_text, "9r3Rmy"));
	std::vector<std::uint8_t> invalid_response = client_response;
	invalid_response[5] ^= 0x01;
	assert(!mkwii::qr_challenge_matches(invalid_response, challenge_text,
										"9r3Rmy"));

	std::vector<std::uint8_t> wrong_command = request;
	wrong_command[0] = 0x00;
	assert(!mkwii::is_mariokartwii_availability_request(wrong_command));

	std::vector<std::uint8_t> wrong_game = request;
	wrong_game[5] = 'M';
	assert(!mkwii::is_mariokartwii_availability_request(wrong_game));

	std::vector<std::uint8_t> truncated = request;
	truncated.pop_back();
	assert(!mkwii::is_mariokartwii_availability_request(truncated));
	return 0;
}