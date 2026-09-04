#ifndef MKWII_CONFIG_H
#define MKWII_CONFIG_H

#include <cstdint>
#include <string>

namespace mkwii {

struct Config {
    std::string server_name;
    std::string advertised_address;
    std::uint16_t health_port;
    std::uint16_t dns_port;
    std::uint16_t game_port;
};

Config config_from_environment();

}  // namespace mkwii

#endif  // MKWII_CONFIG_H
