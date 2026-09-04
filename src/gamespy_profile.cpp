#include "mkwii/gamespy_profile.h"

namespace mkwii {
namespace {

constexpr char keepalive_message[] = "\\ka\\\\final\\";

}  // namespace

bool is_profile_keepalive(const std::string& request) {
    return request == keepalive_message;
}

const std::string& profile_keepalive_response() {
    static const std::string response = keepalive_message;
    return response;
}

}  // namespace mkwii