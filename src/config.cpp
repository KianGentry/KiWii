#include "mkwii/config.h"

#include <cstdlib>
#include <stdexcept>
#include <string>

namespace mkwii {
namespace {

// load required environment variable and throw if missing or empty
std::string required_environment(const char* name) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        throw std::runtime_error(std::string("missing required environment variable: ") + name);
    }
    return value;
}

// load port from environment variable, falling back to default if not set, validate range
std::uint16_t port_environment(const char* name, std::uint16_t fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }

    const unsigned long port = std::stoul(value);
    if (port == 0 || port > 65535) {
        throw std::runtime_error(std::string("invalid port in environment variable: ") + name);
    }
    return static_cast<std::uint16_t>(port);
}

std::string optional_environment(const char* name, const char* fallback) {
    const char* value = std::getenv(name);
    return value == nullptr || *value == '\0' ? fallback : value;
}

}  // namespace

// construct config from environment variables, loading required settings and optional port overrides
Config config_from_environment() {
    return Config{
        required_environment("MKWII_SERVER_NAME"),
        required_environment("MKWII_ADVERTISED_ADDRESS"),
        port_environment("MKWII_HEALTH_PORT", 8080),
        port_environment("MKWII_DNS_PORT", 53),
        port_environment("MKWII_NAS_PORT", 80),
        port_environment("MKWII_QR_PORT", 27900),
        port_environment("MKWII_NATNEG_PORT", 27901),
        port_environment("MKWII_PROFILE_PORT", 29900),
        port_environment("MKWII_PLAYER_SEARCH_PORT", 29901),
        port_environment("MKWII_RELAY_PORT", 22000),
        port_environment("MKWII_GAME_PORT", 28910),
        optional_environment("MKWII_GAMESPY_SECRET_KEY", "9r3Rmy"),
    };
}

}  // namespace mkwii
