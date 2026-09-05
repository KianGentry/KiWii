#include "mkwii/config.h"

#include <cassert>
#include <cstdlib>

int main() {
    setenv("MKWII_SERVER_NAME", "test-server", 1);
    setenv("MKWII_ADVERTISED_ADDRESS", "192.0.2.10", 1);
    setenv("MKWII_HEALTH_PORT", "18080", 1);
    setenv("MKWII_DNS_PORT", "15353", 1);
    setenv("MKWII_NAS_PORT", "18080", 1);
    setenv("MKWII_QR_PORT", "27900", 1);
    setenv("MKWII_NATNEG_PORT", "27901", 1);
    setenv("MKWII_PROFILE_PORT", "29900", 1);
    setenv("MKWII_PLAYER_SEARCH_PORT", "29901", 1);
    setenv("MKWII_GAME_PORT", "28910", 1);

    const mkwii::Config config = mkwii::config_from_environment();
    assert(config.server_name == "test-server");
    assert(config.advertised_address == "192.0.2.10");
    assert(config.health_port == 18080);
    assert(config.dns_port == 15353);
    assert(config.nas_port == 18080);
    assert(config.qr_port == 27900);
    assert(config.natneg_port == 27901);
    assert(config.profile_port == 29900);
    assert(config.player_search_port == 29901);
    assert(config.game_port == 28910);
    assert(config.gamespy_secret_key == "9r3Rmy");
    return 0;
}
