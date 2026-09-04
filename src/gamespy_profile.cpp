#include "mkwii/gamespy_profile.h"

#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string_view>

#include <openssl/evp.h>

namespace mkwii {
namespace {

constexpr char keepalive_message[] = "\\ka\\\\final\\";
constexpr std::string_view text_alphabet =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

std::string random_text(std::size_t length, std::string_view alphabet) {
    static std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<std::size_t> distribution(0, alphabet.size() - 1);

    std::string value;
    value.reserve(length);
    for (std::size_t index = 0; index < length; ++index) {
        value += alphabet[distribution(generator)];
    }
    return value;
}

std::string random_hex(std::size_t length) {
    return random_text(length, "0123456789abcdef");
}

std::string request_value(const std::string& request, std::string_view key) {
    const std::string search = "\\" + std::string(key) + "\\";
    const std::size_t value_start = request.find(search);
    if (value_start == std::string::npos) {
        return {};
    }

    const std::size_t first = value_start + search.size();
    const std::size_t last = request.find('\\', first);
    return request.substr(first, last - first);
}

std::string md5_hex(std::string_view value) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_size = 0;
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (context == nullptr ||
        EVP_DigestInit_ex(context, EVP_md5(), nullptr) != 1 ||
        EVP_DigestUpdate(context, value.data(), value.size()) != 1 ||
        EVP_DigestFinal_ex(context, digest, &digest_size) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("could not calculate MD5 digest");
    }
    EVP_MD_CTX_free(context);

    std::ostringstream result;
    result << std::hex << std::setfill('0');
    for (unsigned int index = 0; index < digest_size; ++index) {
        const unsigned char byte = digest[index];
        result << std::setw(2) << static_cast<unsigned int>(byte);
    }
    return result.str();
}

}  // namespace

bool is_profile_keepalive(const std::string& request) {
    return request == keepalive_message;
}

bool is_profile_login(const std::string& request) {
    constexpr std::string_view prefix = "\\login\\";
    constexpr std::string_view suffix = "\\final\\";
    return request.compare(0, prefix.size(), prefix) == 0 &&
        request.size() >= suffix.size() &&
        request.compare(request.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool is_profile_getprofile(const std::string& request) {
    return request.compare(0, 12, "\\getprofile\\") == 0 &&
        request.size() >= 7 &&
        request.compare(request.size() - 7, 7, "\\final\\") == 0;
}

bool is_profile_updatepro(const std::string& request) {
    return request.compare(0, 11, "\\updatepro\\") == 0 &&
        request.size() >= 7 &&
        request.compare(request.size() - 7, 7, "\\final\\") == 0;
}

const std::string& profile_keepalive_response() {
    static const std::string response = keepalive_message;
    return response;
}

std::string profile_login_challenge() {
    return "\\lc\\1\\challenge\\" + random_text(10, "ABCDEFGHIJKLMNOPQRSTUVWXYZ") +
            "\\id\\1\\final\\";
}

std::string profile_login_response(const std::string& request,
    const std::string& server_challenge,
    const LoginCredentials& credentials) {
    const std::string session_key = random_text(8, "0123456789");
    const std::string login_ticket = random_text(16, text_alphabet);
    const std::string user_id = credentials.user_id.empty()
        ? "0000000000002" : credentials.user_id;
    const std::string request_id = request_value(request, "id");
    const std::string client_challenge = request_value(request, "challenge");
    const std::string digest = md5_hex(credentials.challenge);
    const std::string proof_input = digest + std::string(0x30, ' ') +
        credentials.token + server_challenge + client_challenge + digest;
    const std::string proof = md5_hex(proof_input);

    std::ostringstream response;
    response << "\\lc\\2"
            << "\\sesskey\\" << session_key
            << "\\proof\\" << proof
            << "\\userid\\" << user_id
            << "\\profileid\\1"
            << "\\uniquenick\\KiWii" << user_id
            << "\\lt\\" << login_ticket
            << "\\id\\" << request_id
            << "\\final\\";
    return response.str();
}

std::string profile_getprofile_response(const std::string& request) {
    std::ostringstream response;
    response << "\\pi\\\\"
            << "profileid\\1"
            << "\\nick\\KiWii"
            << "\\userid\\0000000000002"
            << "\\email\\KiWii@nds"
            << "\\sig\\" << random_hex(32)
            << "\\uniquenick\\KiWii0000000000002"
            << "\\pid\\11"
            << "\\lon\\0.000000"
            << "\\lat\\0.000000"
            << "\\loc\\"
            << "\\id\\" << request_value(request, "id")
            << "\\final\\";
    return response.str();
}

}  // namespace mkwii
