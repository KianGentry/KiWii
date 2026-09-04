#include "mkwii/gamespy_profile.h"

#include <cassert>

int main() {
    assert(mkwii::is_profile_keepalive("\\ka\\\\final\\"));
    assert(mkwii::profile_keepalive_response() == "\\ka\\\\final\\");
    const std::string challenge = mkwii::profile_login_challenge();
    assert(challenge.starts_with("\\lc\\1\\challenge\\"));
    assert(challenge.ends_with("\\id\\1\\final\\"));
        assert(challenge.size() == 38);
        const std::string login = "\\login\\\\challenge\\client\\authtoken\\NDStoken\\id\\1\\final\\";
        assert(mkwii::is_profile_login(login));
        const std::string login_response = mkwii::profile_login_response(login);
        assert(login_response.starts_with("\\lc\\2\\sesskey\\"));
        assert(login_response.find("\\proof\\") != std::string::npos);
        assert(login_response.find("\\profileid\\1\\") != std::string::npos);
        assert(login_response.ends_with("\\id\\1\\final\\"));
    assert(!mkwii::is_profile_keepalive("\\ka\\\\final"));
    assert(!mkwii::is_profile_keepalive("\\login\\final\\"));
    return 0;
}