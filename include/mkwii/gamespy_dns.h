#ifndef MKWII_GAMESPY_DNS_H
#define MKWII_GAMESPY_DNS_H

#include <cstdint>
#include <string>
#include <vector>

namespace mkwii {

std::vector<std::uint8_t> dns_a_response(
	const std::vector<std::uint8_t> &request, const std::string &address);

}  // namespace mkwii

#endif  // MKWII_GAMESPY_DNS_H