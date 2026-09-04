#include "mkwii/config.h"

#include <cstdlib>
#include <stdexcept>
#include <string>

namespace mkwii {
namespace {

std::string required_environment(const char* name) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        throw std::runtime_error(std::string("missing required environment variable: ") + name);
    }
    return value;
}

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

}  // namespace

Config config_from_environment() {
    return Config{
        required_environment("MKWII_SERVER_NAME"),
        required_environment("MKWII_ADVERTISED_ADDRESS"),
        port_environment("MKWII_HEALTH_PORT", 8080),
        port_environment("MKWII_DNS_PORT", 5353),
        port_environment("MKWII_GAME_PORT", 28910),
    };
}

}  // namespace mkwii
