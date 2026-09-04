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
    const std::string getprofile =
        "\\getprofile\\\\sesskey\\12345678\\profileid\\1\\id\\2\\final\\";
    assert(mkwii::is_profile_getprofile(getprofile));
    const std::string profile_response = mkwii::profile_getprofile_response(getprofile);
    assert(profile_response.compare(0, 5, "\\pi\\\\") == 0);
    assert(profile_response.find("\\profileid\\1\\") != std::string::npos);
    assert(profile_response.find("\\nick\\KiWii0000000000002\\") !=
           std::string::npos);
    assert(profile_response.find("\\email\\KiWii0000000000002@nds\\") !=
           std::string::npos);
    assert(profile_response.find("\\uniquenick\\KiWii0000000000002\\") !=
           std::string::npos);
    assert(profile_response.compare(profile_response.size() - 12, 12,
            "\\id\\2\\final\\") == 0);
    const std::string updatepro =
        "\\updatepro\\\\sesskey\\12345678\\firstname\\Wii:RMCP\\partnerid\\11\\final\\";
    assert(mkwii::is_profile_updatepro(updatepro));
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