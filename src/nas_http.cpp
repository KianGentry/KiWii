#include "mkwii/nas_http.h"

#include <ctime>
#include <mutex>
#include <random>
#include <string_view>
#include <unordered_map>

namespace mkwii {
namespace {

// generate random text of specified length using alphanumeric characters
std::string random_text(std::size_t length) {
	static std::mt19937 generator(std::random_device{}());
	constexpr std::string_view alphabet =
		"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	std::uniform_int_distribution<std::size_t> distribution(0, alphabet.size() -
																   1);

	std::string value;
	value.reserve(length);
	for (std::size_t index = 0; index < length; ++index) {
		value += alphabet[distribution(generator)];
	}
	return value;
}

// base64 encode input, padding with asterisks for incomplete final groups
std::string base64_encode(std::string_view value) {
	constexpr std::string_view alphabet =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string encoded;
	for (std::size_t index = 0; index < value.size(); index += 3) {
		// read up to 3 bytes and pack into 24-bit chunk
		const unsigned int first = static_cast<unsigned char>(value[index]);
		const unsigned int second =
			index + 1 < value.size()
				? static_cast<unsigned char>(value[index + 1])
				: 0;
		const unsigned int third =
			index + 2 < value.size()
				? static_cast<unsigned char>(value[index + 2])
				: 0;
		const unsigned int chunk = (first << 16) | (second << 8) | third;

		// encode chunk as 4 base64 characters (or padding)
		encoded += alphabet[(chunk >> 18) & 0x3f];
		encoded += alphabet[(chunk >> 12) & 0x3f];
		encoded +=
			index + 1 < value.size() ? alphabet[(chunk >> 6) & 0x3f] : '*';
		encoded += index + 2 < value.size() ? alphabet[chunk & 0x3f] : '*';
	}
	return encoded;
}

// get current UTC time as Nintendo format string (YYYYMMDDhhmmss)
std::string current_datetime() {
	const std::time_t now = std::time(nullptr);
	std::tm utc_time{};
	gmtime_r(&now, &utc_time);

	char value[15];
	std::strftime(value, sizeof(value), "%Y%m%d%H%M%S", &utc_time);
	return value;
}

// extract userid parameter value from request body
std::string request_user_id(const std::string &request) {
	const std::size_t body_start = request.find("\r\n\r\n");
	const std::size_t user_id_start = request.find("userid=", body_start);
	if (user_id_start == std::string::npos) {
		return {};
	}

	const std::size_t value_start = user_id_start + 7;
	const std::size_t value_end = request.find('&', value_start);
	return request.substr(value_start, value_end - value_start);
}

struct LoginSession {
	std::string challenge;
	std::string token;
	std::string request_body;
};

std::unordered_map<std::string, LoginSession> login_sessions;
std::mutex login_sessions_mutex;

// extract HTTP body from request (everything after blank line separating
// headers)
std::string request_body(const std::string &request) {
	const std::size_t body_start = request.find("\r\n\r\n");
	if (body_start == std::string::npos) {
		return {};
	}
	return request.substr(body_start + 4);
}

std::string sake_get_my_records_response() {
	const std::string body =
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
		"<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
		"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
		"xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
		"<soap:Body>"
		"<GetMyRecordsResponse xmlns=\"http://gamespy.net/sake\">"
		"<GetMyRecordsResult>Success</GetMyRecordsResult>"
		"<values/>"
		"</GetMyRecordsResponse>"
		"</soap:Body></soap:Envelope>";
	return "HTTP/1.1 200 OK\r\n"
		   "Content-Type: text/xml; charset=utf-8\r\n"
		   "Content-Length: " + std::to_string(body.size()) + "\r\n"
		   "Connection: close\r\n\r\n" + body;
}

std::string sake_create_record_response() {
	const std::string body =
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
		"<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
		"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
		"xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
		"<soap:Body>"
		"<CreateRecordResponse xmlns=\"http://gamespy.net/sake\">"
		"<CreateRecordResult>Success</CreateRecordResult>"
		"<recordid>1</recordid>"
		"</CreateRecordResponse>"
		"</soap:Body></soap:Envelope>";
	return "HTTP/1.1 200 OK\r\n"
		   "Content-Type: text/xml; charset=utf-8\r\n"
		   "Content-Length: " + std::to_string(body.size()) + "\r\n"
		   "Connection: close\r\n\r\n" + body;
}

} // namespace

LoginCredentials credentials_for_token(const std::string &token) {
	std::lock_guard<std::mutex> lock(login_sessions_mutex);
	for (const auto &[user_id, session] : login_sessions) {
		if (session.token == token) {
			return {session.challenge, session.token, user_id};
		}
	}
	return {};
}

// generic connectivity check response
std::string nas_connectivity_response() {
	return "HTTP/1.1 200 OK\r\n"
		   "Content-Type: text/html\r\n"
		   "Content-Length: 2\r\n"
		   "X-Organization: Nintendo\r\n"
		   "Server: BigIP\r\n"
		   "Connection: close\r\n"
		   "\r\n"
		   "ok";
}

// process NAS login request and generate appropriate response
std::string nas_response_for_request(const std::string &request) {
	if (request.find("POST /SakeStorageServer/StorageServer.asmx ") !=
			std::string::npos &&
		request.find("GetMyRecords") != std::string::npos) {
		return sake_get_my_records_response();
	}
	if (request.find("POST /SakeStorageServer/StorageServer.asmx ") !=
			std::string::npos &&
		request.find("CreateRecord") != std::string::npos) {
		return sake_create_record_response();
	}

	if (request.find("POST /ac ") == std::string::npos ||
		request.find("action=bG9naW4%2A") == std::string::npos) {
		return nas_connectivity_response();
	}

	const std::string user_id = request_user_id(request);
	const LoginSession session{
		random_text(8),
		"NDS" + random_text(80),
		request_body(request),
	};
	std::lock_guard<std::mutex> lock(login_sessions_mutex);
	login_sessions[user_id] = session;

	const std::string login_body =
		"retry=" + base64_encode("0") + "&returncd=" + base64_encode("001") +
		"&locator=" + base64_encode("gamespy.com") +
		"&challenge=" + base64_encode(session.challenge) +
		"&token=" + base64_encode(session.token) +
		"&datetime=" + base64_encode(current_datetime()) + "\r\n";
	return "HTTP/1.1 200 OK\r\n"
		   "Content-Type: text/plain\r\n"
		   "Content-Length: " +
		   std::to_string(login_body.size()) +
		   "\r\n"
		   "NODE: wifiappe1\r\n"
		   "Connection: close\r\n"
		   "\r\n" +
		   login_body;
}

} // namespace mkwii