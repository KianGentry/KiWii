#include "mkwii/gamespy_profile.h"

#include <cassert>

int main() {
    assert(mkwii::is_profile_keepalive("\\ka\\\\final\\"));
    assert(mkwii::profile_keepalive_response() == "\\ka\\\\final\\");
    const std::string challenge = mkwii::profile_login_challenge();
    assert(challenge.compare(0, 16, "\\lc\\1\\challenge\\") == 0);
    assert(challenge.compare(challenge.size() - 12, 12, "\\id\\1\\final\\") == 0);
    assert(challenge.size() == 38);
    const std::string login = "\\login\\\\challenge\\client\\authtoken\\NDStoken\\id\\1\\final\\";
    assert(mkwii::is_profile_login(login));
    const mkwii::LoginCredentials credentials{
        "nas-challenge",
        "NDS-test-token",
        "0000000000002",
    };
    const std::string login_response = mkwii::profile_login_response(
        login, "SERVERCHALL", credentials);
    assert(login_response.compare(0, 14, "\\lc\\2\\sesskey\\") == 0);
    assert(login_response.find("\\proof\\") != std::string::npos);
    assert(login_response.find("\\profileid\\1\\") != std::string::npos);
        assert(login_response.compare(login_response.size() - 12, 12,
            "\\id\\1\\final\\") == 0);
    assert(!mkwii::is_profile_keepalive("\\ka\\\\final"));
    assert(!mkwii::is_profile_keepalive("\\login\\final\\"));
    return 0;
}