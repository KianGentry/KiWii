#ifndef MKWII_GAMESPY_PROFILE_H
#define MKWII_GAMESPY_PROFILE_H

#include <string>

namespace mkwii {

bool is_profile_keepalive(const std::string& request);

const std::string& profile_keepalive_response();

}  // namespace mkwii

#endif  // MKWII_GAMESPY_PROFILE_H