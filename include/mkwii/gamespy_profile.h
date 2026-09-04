#ifndef MKWII_GAMESPY_PROFILE_H
#define MKWII_GAMESPY_PROFILE_H

#include <string>

namespace mkwii {

bool is_profile_keepalive(const std::string& request);

bool is_profile_login(const std::string& request);

const std::string& profile_keepalive_response();

std::string profile_login_challenge();

std::string profile_login_response(const std::string& request);

}  // namespace mkwii

#endif  // MKWII_GAMESPY_PROFILE_H