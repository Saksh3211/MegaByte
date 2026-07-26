#pragma once
// megabyte::wallet — Keystore (PLACEHOLDER, not the real design)
//
// Real design is Master Spec 13: BIP-39 mnemonic + HD derivation,
// Argon2id-derived key, AES-256-GCM at-rest encryption (Roadmap Milestone 8).
//
// THIS is intentionally much simpler, so the node has *something* real to
// create/save/sign with today:
//   - one Ed25519 keypair per wallet, generated via OpenSSL
//   - saved to disk as hex, in PLAIN TEXT — no encryption at all yet
//   - address = "mbc1" + hex(first 20 bytes of SHA-512d(pubkey))
//     (not real Bech32 — Master Spec §3's actual address format)
//
// Loudly not production-safe: anyone who can read wallet.dat has the funds.
// Flagged with WALLET_SECURITY_WARNING wherever it matters.

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <openssl/evp.h>
#include "../crypto/hash.hpp"

namespace megabyte::wallet {

constexpr size_t kEd25519KeyLen = 32;
constexpr size_t kEd25519SigLen = 64;

using PrivKeyBytes = std::array<uint8_t, kEd25519KeyLen>;
using PubKeyBytes = std::array<uint8_t, kEd25519KeyLen>;
using SigBytes = std::array<uint8_t, kEd25519SigLen>;

inline std::string toHex(const uint8_t* data, size_t len) {
    static const char* hexChars = "0123456789abcdef";
    std::string out;
    out.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        out.push_back(hexChars[data[i] >> 4]);
        out.push_back(hexChars[data[i] & 0x0F]);
    }
    return out;
}

inline std::vector<uint8_t> fromHex(const std::string& hex) {
    std::vector<uint8_t> out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        out.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
    }
    return out;
}

class Wallet {
public:
    // Generates a brand-new Ed25519 keypair.
    static Wallet generate() {
        Wallet w;
        EVP_PKEY* pkey = nullptr;
        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
        EVP_PKEY_keygen_init(ctx);
        EVP_PKEY_keygen(ctx, &pkey);
        EVP_PKEY_CTX_free(ctx);

        size_t privLen = kEd25519KeyLen, pubLen = kEd25519KeyLen;
        EVP_PKEY_get_raw_private_key(pkey, w.privKey_.data(), &privLen);
        EVP_PKEY_get_raw_public_key(pkey, w.pubKey_.data(), &pubLen);
        EVP_PKEY_free(pkey);

        w.computeAddress();
        return w;
    }

    static Wallet fromPrivateKeyHex(const std::string& privHex) {
        Wallet w;
        auto bytes = fromHex(privHex);
        std::copy(bytes.begin(), bytes.end(), w.privKey_.begin());

        EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(
            EVP_PKEY_ED25519, nullptr, w.privKey_.data(), kEd25519KeyLen);
        size_t pubLen = kEd25519KeyLen;
        EVP_PKEY_get_raw_public_key(pkey, w.pubKey_.data(), &pubLen);
        EVP_PKEY_free(pkey);

        w.computeAddress();
        return w;
    }

    // WALLET_SECURITY_WARNING: plaintext hex, no encryption. See file header.
    bool save(const std::string& path) const {
        std::ofstream f(path, std::ios::trunc);
        if (!f) return false;
        f << toHex(privKey_.data(), privKey_.size()) << "\n";
        return true;
    }

    static bool exists(const std::string& path) {
        std::ifstream f(path);
        return f.good();
    }

    static Wallet load(const std::string& path) {
        std::ifstream f(path);
        std::string privHex;
        std::getline(f, privHex);
        return fromPrivateKeyHex(privHex);
    }

    SigBytes sign(const std::string& message) const {
        SigBytes sig{};
        EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(
            EVP_PKEY_ED25519, nullptr, privKey_.data(), kEd25519KeyLen);
        EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
        size_t sigLen = kEd25519SigLen;
        EVP_DigestSignInit(mdctx, nullptr, nullptr, nullptr, pkey);
        EVP_DigestSign(mdctx,
                sig.data(), &sigLen,
                    reinterpret_cast<const uint8_t*>(message.data()), message.size());
        EVP_MD_CTX_free(mdctx);
        EVP_PKEY_free(pkey);
        return sig;
    }

    static bool verify(const PubKeyBytes& pub, const std::string& message, const SigBytes& sig) {
        EVP_PKEY* pkey = EVP_PKEY_new_raw_public_key(
            EVP_PKEY_ED25519, nullptr, pub.data(), kEd25519KeyLen);
        EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
        EVP_DigestVerifyInit(mdctx, nullptr, nullptr, nullptr, pkey);
        int ok = EVP_DigestVerify(mdctx,
            sig.data(), sig.size(),
                reinterpret_cast<const uint8_t*>(message.data()), message.size());
        EVP_MD_CTX_free(mdctx);
        EVP_PKEY_free(pkey);
        return ok == 1;
    }

    const std::string& address() const { return address_; }
    const PubKeyBytes& publicKey() const { return pubKey_; }
    std::string publicKeyHex() const { return toHex(pubKey_.data(), pubKey_.size()); }

private:
    void computeAddress() {
        std::string pubStr(reinterpret_cast<char*>(pubKey_.data()), pubKey_.size());
        auto digest = crypto::sha512d(pubStr);
        // Prototype address: not real Bech32 (Master Spec) — just a
        // recognizable, deterministic, collision-resistant hex string.
        address_ = "mbc1" + toHex(digest.data(), 20);
    }

    PrivKeyBytes privKey_{};
    PubKeyBytes pubKey_{};
    std::string address_;
};

} // namespace megabyte::wallet
