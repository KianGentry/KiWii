#ifndef MKWII_NAS_HTTP_H
#define MKWII_NAS_HTTP_H

#include <string>

namespace mkwii {

std::string nas_connectivity_response();

std::string nas_response_for_request(const std::string& request);

}  // namespace mkwii

#endif  // MKWII_NAS_HTTP_H