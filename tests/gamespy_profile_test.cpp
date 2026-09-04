#include "mkwii/gamespy_profile.h"

#include <cassert>

int main() {
    assert(mkwii::is_profile_keepalive("\\ka\\\\final\\"));
    assert(mkwii::profile_keepalive_response() == "\\ka\\\\final\\");
    const std::string challenge = mkwii::profile_login_challenge();
    assert(challenge.starts_with("\\lc\\1\\challenge\\"));
    assert(challenge.ends_with("\\id\\1\\final\\"));
    assert(challenge.size() == 36);
    assert(!mkwii::is_profile_keepalive("\\ka\\\\final"));
    assert(!mkwii::is_profile_keepalive("\\login\\final\\"));
    return 0;
}