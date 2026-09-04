#include "mkwii/gamespy_profile.h"

#include <cassert>

int main() {
    assert(mkwii::is_profile_keepalive("\\ka\\\\final\\"));
    assert(mkwii::profile_keepalive_response() == "\\ka\\\\final\\");
    assert(!mkwii::is_profile_keepalive("\\ka\\\\final"));
    assert(!mkwii::is_profile_keepalive("\\login\\final\\"));
    return 0;
}