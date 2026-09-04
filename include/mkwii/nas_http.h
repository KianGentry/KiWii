#ifndef MKWII_NAS_HTTP_H
#define MKWII_NAS_HTTP_H

#include <string>

namespace mkwii {

struct LoginCredentials {
	std::string challenge;
	std::string token;
	std::string user_id;
};

std::string nas_connectivity_response();

std::string nas_response_for_request(const std::string& request);

LoginCredentials credentials_for_token(const std::string& token);

}  // namespace mkwii

#endif  // MKWII_NAS_HTTP_H