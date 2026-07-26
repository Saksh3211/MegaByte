#pragma once
// megabyte::crypto — Hash512 (SHA-512d = SHA-512 applied twice)
// Matches Master Architecture Spec (hash function).
// Grows into: crypto/hash.hpp, crypto/sha512.hpp per the full repo layout.

#include <array>
#include <cstdint>
#include <string>
#include <openssl/evp.h>

namespace megabyte::crypto {

using Digest512 = std::array<uint8_t, 64>;

inline Digest512 sha512(const std::string& data) {
    Digest512 out{};
    unsigned int len = 0;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha512(), nullptr);
    EVP_DigestUpdate(ctx, data.data(), data.size());
    EVP_DigestFinal_ex(ctx, out.data(), &len);
    EVP_MD_CTX_free(ctx);
    return out;
}

// SHA-512d: double hash, closes the length-extension gap (Master Spec §2).
inline Digest512 sha512d(const std::string& data) {
    Digest512 first = sha512(data);
    std::string firstStr(reinterpret_cast<char*>(first.data()), first.size());
    return sha512(firstStr);
}

inline std::string toHex(const Digest512& d) {
    static const char* hexChars = "0123456789abcdef";
    std::string out;
    out.reserve(d.size() * 2);
    for (uint8_t b : d) {
        out.push_back(hexChars[b >> 4]);
        out.push_back(hexChars[b & 0x0F]);
    }
    return out;
}

// Leading-zero-hex-digit count, used by the (placeholder) difficulty check.
inline int leadingZeroHexDigits(const std::string& hexHash) {
    int count = 0;
    for (char c : hexHash) {
        if (c == '0') count++;
        else break;
    }
    return count;
}

} // namespace megabyte::crypto
