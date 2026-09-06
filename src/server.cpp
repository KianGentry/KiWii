#include "mkwii/server.h"

#include "mkwii/gamespy_profile.h"
#include "mkwii/gamespy_qr.h"
#include "mkwii/nas_http.h"

#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mkwii {
namespace {

volatile std::sig_atomic_t keep_running = 1;

void stop_server(int) { keep_running = 0; }

bool send_all(int socket_fd, const char *data, std::size_t size) {
	std::size_t sent = 0;
	while (sent < size) {
		const ssize_t result =
			send(socket_fd, data + sent, size - sent, MSG_NOSIGNAL);
		if (result < 0) {
			if (errno == EINTR) {
				continue;
			}
			return false;
		}
		if (result == 0) {
			return false;
		}
		sent += static_cast<std::size_t>(result);
	}
	return true;
}

void set_receive_timeout(int socket_fd, int seconds) {
	const timeval receive_timeout{seconds, 0};
	setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &receive_timeout,
			   sizeof(receive_timeout));
}

/* SERVICES, socket binding */

int open_health_socket(std::uint16_t port) {
	// create tcp socket (SOCK_STREAM) for health service
	const int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0) {
		return -1;
	}

	// set SO_REUSEADDR to allow binding to the same port after restart,
	// avoiding "Address already in use" errors
	int reuse_address = 1;
	setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_address,
			   sizeof(reuse_address));

	// configure address structure to listen on all interfaces (INADDR_ANY)
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(port);

	// bind socket to the configured address and start listening for incoming
	// connections
	if (bind(socket_fd, reinterpret_cast<sockaddr *>(&address),
			 sizeof(address)) < 0 ||
		listen(socket_fd, 8) < 0) {
		close(socket_fd);
		return -1;
	}
	return socket_fd;
}

int open_qr_socket(std::uint16_t port) {
	// create udp socket (SOCK_DGRAM) for qr service
	const int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_fd < 0) {
		return -1;
	}

	// set SO_REUSEADDR for same reasons as health socket
	int reuse_address = 1;
	setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_address,
			   sizeof(reuse_address));

	// configure to listen on all interfaces
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(port);

	// bind udp socket (udp doesn't require listen)
	if (bind(socket_fd, reinterpret_cast<sockaddr *>(&address),
			 sizeof(address)) < 0) {
		close(socket_fd);
		return -1;
	}
	return socket_fd;
}

int open_game_socket(std::uint16_t port) {
	// create tcp socket for game service
	const int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0) {
		return -1;
	}

	// set SO_REUSEADDR for address reuse
	int reuse_address = 1;
	setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_address,
			   sizeof(reuse_address));

	// configure to listen on all interfaces
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(port);

	// bind and listen for incoming tcp connections
	if (bind(socket_fd, reinterpret_cast<sockaddr *>(&address),
			 sizeof(address)) < 0 ||
		listen(socket_fd, 8) < 0) {
		close(socket_fd);
		return -1;
	}
	return socket_fd;
}

void handle_qr_packet(int qr_socket, const std::string &secret_key) {
	static std::unordered_set<std::uint32_t> qr_sessions;
	static std::unordered_map<std::uint32_t, std::string> qr_challenges;
	// allocate buffer and receive a packet from qr socket
	std::vector<std::uint8_t> packet(2048);
	sockaddr_in client_address{};
	socklen_t client_address_length = sizeof(client_address);

	const ssize_t packet_size = recvfrom(
		qr_socket, packet.data(), packet.size(), 0,
		reinterpret_cast<sockaddr *>(&client_address), &client_address_length);
	if (packet_size < 0) {
		return;
	}

	// trim buffer to actual received packet size
	packet.resize(static_cast<std::size_t>(packet_size));
	const std::uint32_t session_id = qr_session_id(packet);
	const auto challenge = qr_challenges.find(session_id);
	if (packet.size() >= 5 && packet[0] == 0x01 &&
		challenge != qr_challenges.end() &&
		qr_challenge_matches(packet, challenge->second, secret_key)) {
		const std::vector<std::uint8_t> response =
			qr_registered_response(session_id);
		sendto(qr_socket, response.data(), response.size(), 0,
			   reinterpret_cast<sockaddr *>(&client_address),
			   client_address_length);
		qr_challenges.erase(challenge);
		std::cout << "GameSpy QR session registered: " << session_id << "\n";
		return;
	}

	if (is_mariokartwii_heartbeat(packet)) {
		const std::string state_changed =
			qr_field_value(packet, "statechanged");
		if (state_changed == "2") {
			qr_sessions.erase(session_id);
			qr_challenges.erase(session_id);
		} else {
			const bool new_session = qr_sessions.insert(session_id).second;
			if (new_session) {
				const std::vector<std::uint8_t> response =
					qr_challenge_response(session_id,
										  inet_ntoa(client_address.sin_addr),
										  ntohs(client_address.sin_port));
				sendto(qr_socket, response.data(), response.size(), 0,
					   reinterpret_cast<sockaddr *>(&client_address),
					   client_address_length);
				const auto challenge_end =
					std::find(response.begin() + 7, response.end(), 0x00);
				qr_challenges.emplace(
					session_id,
					std::string(response.begin() + 7, challenge_end));
				std::cout << "GameSpy QR challenge sent for session "
						  << session_id << "\n";
			}
		}
		std::cout << "GameSpy QR heartbeat for session " << session_id
				  << " (state "
				  << (state_changed.empty() ? "unchanged" : state_changed)
				  << ")\n";
		return;
	}

	// verify this is a mario kart wii availability request, ignore if not
	if (!is_mariokartwii_availability_request(packet)) {
		return;
	}

	// generate appropriate response and send back to requesting client
	const std::vector<std::uint8_t> response = availability_response();
	sendto(qr_socket, response.data(), response.size(), 0,
		   reinterpret_cast<sockaddr *>(&client_address),
		   client_address_length);
	std::cout << "GameSpy QR availability request from "
			  << inet_ntoa(client_address.sin_addr) << '\n';
}

void handle_natneg_packet(int natneg_socket) {
	constexpr std::uint8_t magic[] = {0xfd, 0xfc, 0x1e, 0x66, 0x6a, 0xb2};
	std::vector<std::uint8_t> packet(2048);
	sockaddr_in client_address{};
	socklen_t client_address_length = sizeof(client_address);
	const ssize_t packet_size = recvfrom(
		natneg_socket, packet.data(), packet.size(), 0,
		reinterpret_cast<sockaddr *>(&client_address), &client_address_length);
	if (packet_size < 8) {
		return;
	}
	packet.resize(static_cast<std::size_t>(packet_size));
	if (!std::equal(std::begin(magic), std::end(magic), packet.begin()) ||
		packet[6] != 0x03) {
		return;
	}

	std::ostringstream formatted_packet;
	formatted_packet << std::hex << std::setfill('0');
	for (const std::uint8_t byte : packet) {
		formatted_packet << std::setw(2) << static_cast<unsigned int>(byte);
	}
	std::cout << "GameSpy NATNEG record 0x" << std::setw(2)
			  << static_cast<unsigned int>(packet[7]) << " ("
			  << packet.size() << " bytes): " << formatted_packet.str() << '\n';

	if (packet[7] != 0x00 || packet.size() < 14) {
		return;
	}

	std::vector<std::uint8_t> response(packet.begin(), packet.begin() + 14);
	response.insert(response.end(), {0xff, 0xff, 0x6d, 0x16, 0xb5, 0x7d, 0xea});
	response[7] = 0x01;
	sendto(natneg_socket, response.data(), response.size(), 0,
		   reinterpret_cast<sockaddr *>(&client_address), client_address_length);
	std::cout << "GameSpy NATNEG initialization acknowledged\n";
}

void handle_game_connection(int game_socket) {
	// accept incoming tcp connection from client
	const int client_socket = accept(game_socket, nullptr, nullptr);
	if (client_socket < 0) {
		return;
	}
	set_receive_timeout(client_socket, 5);

	// receive initial request packet from the client
	std::vector<std::uint8_t> packet(4096);
	const ssize_t packet_size =
		recv(client_socket, packet.data(), packet.size(), 0);
	if (packet_size > 0) {
		packet.resize(static_cast<std::size_t>(packet_size));
		// format packet bytes as hex string for logging
		std::ostringstream formatted_packet;
		formatted_packet << std::hex << std::setfill('0');

		for (const std::uint8_t byte : packet) {
			formatted_packet << std::setw(2) << static_cast<unsigned int>(byte);
		}
		std::cout << "GameSpy browser request (" << packet.size()
				  << " bytes): " << formatted_packet.str() << '\n';
	}
	close(client_socket);
}

void handle_relay_connection(int relay_socket) {
	const int client_socket = accept(relay_socket, nullptr, nullptr);
	if (client_socket < 0) {
		return;
	}
	set_receive_timeout(client_socket, 5);

	std::vector<std::uint8_t> packet(4096);
	const ssize_t packet_size =
		recv(client_socket, packet.data(), packet.size(), 0);
	if (packet_size > 0) {
		packet.resize(static_cast<std::size_t>(packet_size));
		std::ostringstream formatted_packet;
		formatted_packet << std::hex << std::setfill('0');
		for (const std::uint8_t byte : packet) {
			formatted_packet << std::setw(2)
								 << static_cast<unsigned int>(byte);
		}
		std::cout << "GameSpy relay request (" << packet.size()
				  << " bytes): " << formatted_packet.str() << '\n';
	} else {
		std::cout << "GameSpy relay connection opened without payload\n";
	}
	close(client_socket);
}

std::string player_search_response(const std::string &request) {
	std::string response = "\\otherslist\\";
	const std::size_t opids_marker = request.find("\\opids\\");
	if (opids_marker != std::string::npos) {
		const std::size_t value_start = opids_marker + 7;
		const std::size_t value_end = request.find('\\', value_start);
		const std::string opids = request.substr(
			value_start, value_end == std::string::npos
											? std::string::npos
											: value_end - value_start);
			std::size_t start = 0;
		while (start <= opids.size()) {
			const std::size_t separator = opids.find('|', start);
			const std::string opid = opids.substr(
				start, separator == std::string::npos ? std::string::npos
															 : separator - start);
			if (!opid.empty()) {
				response += "\\o\\" + opid + "\\uniquenick\\";
			}
			if (separator == std::string::npos) {
				break;
			}
			start = separator + 1;
		}
	}
	return response + "\\oldone\\\\final\\";
}

void handle_player_search_connection(int player_search_socket) {
	const int client_socket =
		accept(player_search_socket, nullptr, nullptr);
	if (client_socket < 0) {
		return;
	}
	set_receive_timeout(client_socket, 5);

	std::string request_buffer;
	char request_chunk[4096];
	while (true) {
		const ssize_t packet_size =
			recv(client_socket, request_chunk, sizeof(request_chunk), 0);
		if (packet_size <= 0) {
			break;
		}
		request_buffer.append(request_chunk, static_cast<std::size_t>(packet_size));
		while (true) {
			const std::size_t message_end = request_buffer.find("\\final\\");
			if (message_end == std::string::npos) {
				break;
			}
			const std::size_t message_size = message_end + 7;
			const std::string request = request_buffer.substr(0, message_size);
			request_buffer.erase(0, message_size);
			std::cout << "GameSpy player search request: " << request << '\n';
			const std::string response = player_search_response(request);
			send_all(client_socket, response.data(), response.size());
			std::cout << "GameSpy player search response sent\n";
		}
	}
	close(client_socket);
}

void handle_profile_connection(int profile_socket) {
	// accept incoming tcp connection from client
	const int client_socket = accept(profile_socket, nullptr, nullptr);
	if (client_socket < 0) {
		return;
	}

	// Bound idle clients so shutdown can join active workers.
	set_receive_timeout(client_socket, 5);

	// send initial login challenge to initiate GameSpy profile authentication
	const std::string login_challenge = profile_login_challenge();
	LoginCredentials credentials;
	std::string firstname;
	std::string lastname;
	std::string status;
	std::string statstring;
	std::string locstring;
	const std::size_t challenge_start =
		login_challenge.find("\\challenge\\") + 11;
	const std::size_t challenge_end =
		login_challenge.find("\\id\\", challenge_start);
	const std::string server_challenge = login_challenge.substr(
		challenge_start, challenge_end - challenge_start);
	send_all(client_socket, login_challenge.data(), login_challenge.size());
	std::cout << "GameSpy profile login challenge sent\n";

	// receive and process profile requests from the client
	std::string request_buffer;
	char request_chunk[4096];
	while (true) {
		// attempt to receive more data from client
		const ssize_t packet_size =
			recv(client_socket, request_chunk, sizeof(request_chunk), 0);
		if (packet_size <= 0) {
			break;
		}
		request_buffer.append(request_chunk,
							  static_cast<std::size_t>(packet_size));

		// GameSpy profile messages are delimited by "\final\", extract and
		// process complete messages
		while (true) {
			const std::size_t message_end = request_buffer.find("\\final\\");
			if (message_end == std::string::npos) {
				// no complete message yet, wait for more data
				break;
			}

			// extract complete message including delimiter (7 bytes for
			// "\final\")
			const std::size_t message_size = message_end + 7;
			const std::string request = request_buffer.substr(0, message_size);
			request_buffer.erase(0, message_size);

			std::cout << "GameSpy profile request (" << request.size()
					  << " bytes): " << request << '\n';
			// if this is a keepalive message, send keepalive response to
			// maintain connection
			if (is_profile_keepalive(request)) {
				const std::string &response = profile_keepalive_response();
				send_all(client_socket, response.data(), response.size());
				std::cout << "GameSpy profile keepalive response sent\n";
			} else if (is_profile_login(request)) {
				const std::size_t token_marker = request.find("\\authtoken\\");
				const std::size_t token_start =
					token_marker == std::string::npos ? std::string::npos
													  : token_marker + 11;
				const std::size_t token_end =
					token_start == std::string::npos
						? std::string::npos
						: request.find('\\', token_start);
				const std::string token =
					token_start == std::string::npos
						? std::string()
						: request.substr(token_start, token_end - token_start);
				credentials = credentials_for_token(token);
				const std::string response = profile_login_response(
					request, server_challenge, credentials);
				send_all(client_socket, response.data(), response.size());
				std::cout << "GameSpy profile login response sent\n";
			} else if (is_profile_getprofile(request)) {
				const std::string response = profile_getprofile_response(
					request, credentials, firstname, lastname);
				send_all(client_socket, response.data(), response.size());
				std::cout << "GameSpy profile response sent\n";
			} else if (is_profile_updatepro(request)) {
				const std::string updated_firstname =
					profile_field_value(request, "firstname");
				const std::string updated_lastname =
					profile_field_value(request, "lastname");
				if (!updated_firstname.empty()) {
					firstname = updated_firstname;
				}
				if (!updated_lastname.empty()) {
					lastname = updated_lastname;
				}
				std::cout << "GameSpy profile update consumed\n";
			} else if (is_profile_status(request)) {
				status = profile_field_value(request, "status");
				statstring = profile_field_value(request, "statstring");
				locstring = profile_field_value(request, "locstring");
				std::cout << "GameSpy profile status updated (status "
						  << (status.empty() ? "unchanged" : status)
						  << ", statstring " << statstring << ", locstring "
						  << locstring << ")\n";
			}
		}
	}
	close(client_socket);
}

void handle_nas_connection(int nas_socket) {
	// accept incoming tcp connection from client
	const int client_socket = accept(nas_socket, nullptr, nullptr);
	if (client_socket < 0) {
		return;
	}

	// Bound slow clients so the worker can be joined during shutdown.
	set_receive_timeout(client_socket, 2);

	// receive http request from client, reading until complete
	std::string request_text;
	char request_chunk[1024];
	while (true) {
		// look for end of http headers (blank line separating headers from
		// body)
		const std::size_t header_end = request_text.find("\r\n\r\n");
		// if we have the header end, we can determine if we've received the
		// full request
		if (header_end != std::string::npos) {
			std::size_t content_length = 0;
			// extract Content-Length from headers to know how much body data to
			// expect
			const std::size_t length_start =
				request_text.find("Content-Length:");
			if (length_start != std::string::npos) {
				const std::size_t value_start = length_start + 15;
				const std::size_t value_end =
					request_text.find("\r\n", value_start);
				// parse the content length value
				try {
					content_length = std::stoul(request_text.substr(
						value_start, value_end - value_start));
				} catch (const std::exception &) {
					content_length = 0;
				}
			}
			// check if we've received headers + body (header_end + 4 for
			// "\r\n\r\n" + body)
			if (request_text.size() >= header_end + 4 + content_length) {
				break;
			}
		}

		// receive more data from client
		const ssize_t request_size =
			recv(client_socket, request_chunk, sizeof(request_chunk), 0);
		if (request_size <= 0) {
			break;
		}
		request_text.append(request_chunk,
							static_cast<std::size_t>(request_size));
		// safety limit to prevent memory bloat from malformed requests
		if (request_text.size() > 8192) {
			break;
		}
	}

	// generate and send http response based on the received request
	const std::string response = nas_response_for_request(request_text);
	send_all(client_socket, response.data(), response.size());
	close(client_socket);
	std::cout << "NAS connectivity request served\n";
}

} // namespace

/* MAIN LOOP */

int run_server(const Config &config) {
	// reset running flag and set up signal handlers for graceful shutdown
	keep_running = 1;
	std::signal(SIGINT, stop_server);
	std::signal(SIGTERM, stop_server);

	// bind all service ports, return early with error if any fails
	const int health_socket = open_health_socket(config.health_port);
	if (health_socket < 0) {
		std::cerr << "could not bind health port " << config.health_port
				  << '\n';
		return 1;
	}

	const int qr_socket = open_qr_socket(config.qr_port);
	if (qr_socket < 0) {
		std::cerr << "could not bind GameSpy QR port " << config.qr_port
				  << '\n';
		close(health_socket);
		return 1;
	}

	const int natneg_socket = open_qr_socket(config.natneg_port);
	if (natneg_socket < 0) {
		std::cerr << "could not bind GameSpy NATNEG port "
				  << config.natneg_port << '\n';
		close(health_socket);
		close(qr_socket);
		return 1;
	}

	const int game_socket = open_game_socket(config.game_port);
	if (game_socket < 0) {
		std::cerr << "could not bind GameSpy browser port " << config.game_port
				  << '\n';
		close(health_socket);
		close(qr_socket);
		close(natneg_socket);
		return 1;
	}

	const int nas_socket = open_game_socket(config.nas_port);
	if (nas_socket < 0) {
		std::cerr << "could not bind NAS port " << config.nas_port << '\n';
		close(health_socket);
		close(qr_socket);
		close(natneg_socket);
		close(game_socket);
		return 1;
	}

	const int profile_socket = open_game_socket(config.profile_port);
	if (profile_socket < 0) {
		std::cerr << "could not bind GameSpy profile port "
				  << config.profile_port << '\n';
		close(health_socket);
		close(qr_socket);
		close(natneg_socket);
		close(game_socket);
		close(nas_socket);
		return 1;
	}

	const int player_search_socket =
		open_game_socket(config.player_search_port);
	if (player_search_socket < 0) {
		std::cerr << "could not bind GameSpy player search port "
				  << config.player_search_port << '\n';
		close(health_socket);
		close(qr_socket);
		close(natneg_socket);
		close(game_socket);
		close(nas_socket);
		close(profile_socket);
		return 1;
	}

	const int relay_socket = open_game_socket(config.relay_port);
	if (relay_socket < 0) {
		std::cerr << "could not bind GameSpy relay port " << config.relay_port
				  << '\n';
		close(health_socket);
		close(qr_socket);
		close(natneg_socket);
		close(game_socket);
		close(nas_socket);
		close(profile_socket);
		close(player_search_socket);
		return 1;
	}

	std::cout << "server '" << config.server_name << "' started\n"
			  << "advertised address: " << config.advertised_address << '\n'
			  << "health port: " << config.health_port << '\n'
			  << "DNS port (reserved): " << config.dns_port << '\n'
			  << "NAS HTTP port: " << config.nas_port << '\n'
			  << "GameSpy QR port: " << config.qr_port << '\n'
			  << "GameSpy NATNEG port: " << config.natneg_port << '\n'
			  << "GameSpy profile port: " << config.profile_port << '\n'
			  << "GameSpy player search port: " << config.player_search_port
			  << '\n'
			  << "GameSpy relay port: " << config.relay_port << '\n'
			  << "GameSpy browser port: " << config.game_port << '\n';

	std::vector<std::thread> connection_workers;
	// Process incoming connections until shutdown signal.
	while (keep_running != 0) {
		// use select() to multiplex listening on all sockets with 1 second
		// timeout
		timeval timeout{1, 0};
		fd_set readable{};
		// initialise the set and add all service sockets
		FD_ZERO(&readable);
		FD_SET(health_socket, &readable);
		FD_SET(qr_socket, &readable);
		FD_SET(natneg_socket, &readable);
		FD_SET(game_socket, &readable);
		FD_SET(nas_socket, &readable);
		FD_SET(profile_socket, &readable);
		FD_SET(player_search_socket, &readable);
		FD_SET(relay_socket, &readable);
		const int highest_socket =
			std::max({health_socket, qr_socket, natneg_socket, game_socket, nas_socket,
					  profile_socket, player_search_socket, relay_socket});

		// select waits for any of these sockets to become readable, or timeout
		// after 1 second
		if (select(highest_socket + 1, &readable, nullptr, nullptr, &timeout) <=
			0) {
			// timeout or error, loop and try again
			continue;
		}

		// check which sockets are ready to read and dispatch to appropriate
		// handler
		if (FD_ISSET(qr_socket, &readable)) {
			handle_qr_packet(qr_socket, config.gamespy_secret_key);
		}

		if (FD_ISSET(natneg_socket, &readable)) {
			handle_natneg_packet(natneg_socket);
		}

		if (FD_ISSET(game_socket, &readable)) {
			connection_workers.emplace_back(handle_game_connection,
											game_socket);
		}

		if (FD_ISSET(nas_socket, &readable)) {
			connection_workers.emplace_back(handle_nas_connection, nas_socket);
		}

		if (FD_ISSET(profile_socket, &readable)) {
			connection_workers.emplace_back(handle_profile_connection,
											profile_socket);
		}

		if (FD_ISSET(player_search_socket, &readable)) {
			connection_workers.emplace_back(handle_player_search_connection,
											player_search_socket);
		}

		if (FD_ISSET(relay_socket, &readable)) {
			connection_workers.emplace_back(handle_relay_connection,
											relay_socket);
		}

		// handle health check requests
		if (!FD_ISSET(health_socket, &readable)) {
			continue;
		}

		const int client_socket = accept(health_socket, nullptr, nullptr);
		if (client_socket < 0) {
			continue;
		}

		// send simple HTTP 200 OK response for health checks
		constexpr char response[] = "HTTP/1.1 200 OK\r\n"
									"Content-Type: text/plain\r\n"
									"Content-Length: 3\r\n"
									"Connection: close\r\n\r\n"
									"ok\n";
		send_all(client_socket, response, sizeof(response) - 1);
		close(client_socket);
	}

	for (std::thread &worker : connection_workers) {
		worker.join();
	}

	// clean up: close all listening sockets
	close(health_socket);
	close(qr_socket);
	close(natneg_socket);
	close(game_socket);
	close(nas_socket);
	close(profile_socket);
	close(player_search_socket);
	close(relay_socket);
	std::cout << "server stopped\n";
	return 0;
}

} // namespace mkwii
