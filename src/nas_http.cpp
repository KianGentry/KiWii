#include "mkwii/nas_http.h"

#include <cstdint>
#include <ctime>
#include <mutex>
#include <random>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mkwii {
namespace {

// generate random text of specified length using alphanumeric characters
std::string random_text(std::size_t length) {
	static std::mt19937 generator(std::random_device{}());
	constexpr std::string_view alphabet =
		"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	std::uniform_int_distribution<std::size_t> distribution(0, alphabet.size() - 1);

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

struct FriendInfoRecord {
	std::uint64_t record_id;
	std::string owner;
	std::string game_id;
	std::string table_id;
	std::string info;
};

std::vector<FriendInfoRecord> friend_info_records;
std::mutex friend_info_mutex;
std::uint64_t next_friend_info_record_id = 1;

// extract HTTP body from request (everything after blank line separating
// headers)
std::string request_body(const std::string &request) {
	const std::size_t body_start = request.find("\r\n\r\n");
	if (body_start == std::string::npos) {
		return {};
	}
	return request.substr(body_start + 4);
}

std::string xml_value(const std::string &xml, std::string_view name) {
	const std::string open = "<" + std::string(name) + ">";
	const std::string close = "</" + std::string(name) + ">";
	const std::size_t value_start = xml.find(open);
	if (value_start == std::string::npos) {
		return {};
	}
	const std::size_t content_start = value_start + open.size();
	const std::size_t value_end = xml.find(close, content_start);
	if (value_end == std::string::npos) {
		return {};
	}
	return xml.substr(content_start, value_end - content_start);
}

std::string sake_action(const std::string &request) {
	const std::string marker = "SOAPAction:";
	const std::size_t marker_start = request.find(marker);
	if (marker_start == std::string::npos) {
		return {};
	}
	const std::size_t value_start = request.find_first_not_of(" \t\"", marker_start + marker.size());
	if (value_start == std::string::npos) {
		return {};
	}
	const std::size_t value_end = request.find_first_of("\"\r\n", value_start);
	const std::string value = request.substr(
		value_start, value_end == std::string::npos ? std::string::npos : value_end - value_start);
	const std::size_t separator = value.rfind('/');
	return separator == std::string::npos ? value : value.substr(separator + 1);
}

std::string sake_response(std::string body) {
	const std::string envelope =
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
		"<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" "
		"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
		"xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">"
		"<soap:Body>" + body + "</soap:Body></soap:Envelope>";
	return "HTTP/1.1 200 OK\r\n"
		   "Content-Type: text/xml; charset=utf-8\r\n"
		   "Content-Length: " + std::to_string(envelope.size()) + "\r\n"
		   "Connection: close\r\n\r\n" + envelope;
}

std::string sake_get_my_records_response(const std::string &owner) {
	std::ostringstream values;
	values << "<values>";
	{
		std::lock_guard<std::mutex> lock(friend_info_mutex);
		for (const FriendInfoRecord &record : friend_info_records) {
			if (record.owner != owner) {
				continue;
			}
			values << "<value><recordid>" << record.record_id
			       << "</recordid><gameid>" << record.game_id
			       << "</gameid><tableid>" << record.table_id
			       << "</tableid><info>" << record.info << "</info></value>";
		}
	}
	values << "</values>";
	const std::string body =
		"<GetMyRecordsResponse xmlns=\"http://gamespy.net/sake\">"
		"<GetMyRecordsResult>Success</GetMyRecordsResult>"
		+ values.str() + "</GetMyRecordsResponse>";
	return sake_response(body);
}

std::string sake_create_record_response(const std::string &request) {
	const std::string body_text = request_body(request);
	FriendInfoRecord record{
		0,
		xml_value(body_text, "loginTicket"),
		xml_value(body_text, "gameid"),
		xml_value(body_text, "tableid"),
		xml_value(body_text, "info")};
	{
		std::lock_guard<std::mutex> lock(friend_info_mutex);
		record.record_id = next_friend_info_record_id++;
		friend_info_records.push_back(record);
	}
	const std::string body =
		"<CreateRecordResponse xmlns=\"http://gamespy.net/sake\">"
		"<CreateRecordResult>Success</CreateRecordResult>"
		"<recordid>" + std::to_string(record.record_id) + "</recordid>"
		"</CreateRecordResponse>";
	return sake_response(body);
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
	if (request.find("POST /SakeStorageServer/StorageServer.asmx ") != std::string::npos) {
		const std::string action = sake_action(request);
		const std::string body = request_body(request);
		const std::string owner = xml_value(body, "loginTicket");
		if (action == "GetMyRecords") {
			return sake_get_my_records_response(owner);
		}
		if (action == "CreateRecord") {
			return sake_create_record_response(request);
		}
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