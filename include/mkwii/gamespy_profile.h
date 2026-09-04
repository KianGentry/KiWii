#ifndef MKWII_GAMESPY_PROFILE_H
#define MKWII_GAMESPY_PROFILE_H

#include <string>

#include "mkwii/nas_http.h"

namespace mkwii {

bool is_profile_keepalive(const std::string& request);

bool is_profile_login(const std::string& request);

bool is_profile_getprofile(const std::string& request);

bool is_profile_updatepro(const std::string& request);

std::string profile_field_value(const std::string& request, const std::string& key);

const std::string& profile_keepalive_response();

std::string profile_login_challenge();

std::string profile_login_response(const std::string& request,
	const std::string& server_challenge,
	const LoginCredentials& credentials);

std::string profile_getprofile_response(const std::string& request,
	const LoginCredentials& credentials,
	const std::string& firstname,
	const std::string& lastname);

}  // namespace mkwii

#endif  // MKWII_GAMESPY_PROFILE_H