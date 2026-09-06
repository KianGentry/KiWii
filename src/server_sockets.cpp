#include "mkwii/server_internal.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include <sstream>

namespace mkwii {

bool send_all(int socket_fd, const char *data, std::size_t size) {
    std::size_t sent = 0;
    while (sent < size) {
        const ssize_t result = send(socket_fd, data + sent, size - sent, MSG_NOSIGNAL);
        if (result < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (result == 0) {
            return false;
        }
        sent += static_cast<std::size_t>(result);
    }
    return true;
}

void set_receive_timeout(int socket_fd, int seconds) {
    const timeval receive_timeout{seconds, 0};
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &receive_timeout,
               sizeof(receive_timeout));
}

namespace {

int open_socket(int type, std::uint16_t port) {
    const int socket_fd = socket(AF_INET, type, 0);
    if (socket_fd < 0) {
        return -1;
    }
    int reuse_address = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_address, sizeof(reuse_address));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    if (bind(socket_fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        close(socket_fd);
        return -1;
    }
    return socket_fd;
}

} // namespace

int open_tcp_socket(std::uint16_t port) {
    const int socket_fd = open_socket(SOCK_STREAM, port);
    if (socket_fd >= 0 && listen(socket_fd, 8) < 0) {
        close(socket_fd);
        return -1;
    }
    return socket_fd;
}

int open_udp_socket(std::uint16_t port) {
    return open_socket(SOCK_DGRAM, port);
}

SSL_CTX *create_relay_ssl_context() {
    SSL_CTX *context = SSL_CTX_new(TLS_server_method());
    if (context == nullptr) {
        return nullptr;
    }
    SSL_CTX_set_min_proto_version(context, TLS1_2_VERSION);
    EVP_PKEY *key = EVP_PKEY_new();
    RSA *rsa = RSA_new();
    BIGNUM *exponent = BN_new();
    X509 *certificate = X509_new();
    if (key == nullptr || rsa == nullptr || exponent == nullptr ||
        certificate == nullptr || BN_set_word(exponent, RSA_F4) != 1 ||
        RSA_generate_key_ex(rsa, 2048, exponent, nullptr) != 1 ||
        EVP_PKEY_assign_RSA(key, rsa) != 1) {
        BN_free(exponent);
        RSA_free(rsa);
        EVP_PKEY_free(key);
        X509_free(certificate);
        SSL_CTX_free(context);
        return nullptr;
    }
    rsa = nullptr;
    BN_free(exponent);
    X509_set_version(certificate, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(certificate), 1);
    X509_gmtime_adj(X509_get_notBefore(certificate), 0);
    X509_gmtime_adj(X509_get_notAfter(certificate), 60 * 60 * 24 * 365);
    X509_set_pubkey(certificate, key);
    X509_NAME *name = X509_get_subject_name(certificate);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                               reinterpret_cast<const unsigned char *>("mkwii"),
                               -1, -1, 0);
    X509_set_issuer_name(certificate, name);
    if (X509_sign(certificate, key, EVP_sha256()) == 0 ||
        SSL_CTX_use_certificate(context, certificate) != 1 ||
        SSL_CTX_use_PrivateKey(context, key) != 1) {
        X509_free(certificate);
        EVP_PKEY_free(key);
        SSL_CTX_free(context);
        return nullptr;
    }
    X509_free(certificate);
    EVP_PKEY_free(key);
    return context;
}

} // namespace mkwii
