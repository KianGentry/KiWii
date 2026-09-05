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
    std::uint16_t nas_port;
    std::uint16_t qr_port;
    std::uint16_t natneg_port;
    std::uint16_t profile_port;
    std::uint16_t player_search_port;
    std::uint16_t game_port;
    std::string gamespy_secret_key;
};

Config config_from_environment();

}  // namespace mkwii

#endif  // MKWII_CONFIG_H
