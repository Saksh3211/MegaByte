#pragma once
// megabyte::wallet â€” Transaction (PLACEHOLDER, not the real design)
//
// Real Transaction is Master Spec Â§4.2: goes through a real mempool
// (Milestone 4), gets included in blocks by the miner, fee is size-based
// (Â§9). THIS placeholder does none of that â€” it's signed and can be
// broadcast to peers (who just print it), but it is NOT validated against
// balances/nonces and NOT included in mined blocks yet. That wiring is the
// natural next step once a real mempool exists.

#include <ctime>
#include <sstream>
#include <string>
#include <vector>
#include "keystore.hpp"

namespace megabyte::wallet {

struct Transaction {
    std::string type = "TRANSFER"; // "TRANSFER" | "REGISTER_USERNAME"
    std::string sender;
    std::string receiver; // for REGISTER_USERNAME: the desired username instead
    uint64_t amount = 0;
    uint64_t nonce = 0;
    int64_t timestamp = 0;
    std::string senderPubKeyHex;
    std::string signatureHex;

    std::string signingPayload() const {
        std::ostringstream oss;
        oss << type << '|' << sender << '|' << receiver << '|' << amount << '|' << nonce << '|' << timestamp;
        return oss.str();
    }

    static Transaction createSigned(const Wallet& from, const std::string& toAddress,
                                     uint64_t amount, uint64_t nonce) {
        Transaction tx;
        tx.type = "TRANSFER";
        tx.sender = from.address();
        tx.receiver = toAddress;
        tx.amount = amount;
        tx.nonce = nonce;
        tx.timestamp = static_cast<int64_t>(std::time(nullptr));
        tx.senderPubKeyHex = from.publicKeyHex();

        SigBytes sig = from.sign(tx.signingPayload());
        tx.signatureHex = toHex(sig.data(), sig.size());
        return tx;
    }

    static Transaction createUsernameRegistration(const Wallet& from, const std::string& username,
                                                    uint64_t nonce) {
        Transaction tx;
        tx.type = "REGISTER_USERNAME";
        tx.sender = from.address();
        tx.receiver = username;
        tx.amount = 0;
        tx.nonce = nonce;
        tx.timestamp = static_cast<int64_t>(std::time(nullptr));
        tx.senderPubKeyHex = from.publicKeyHex();

        SigBytes sig = from.sign(tx.signingPayload());
        tx.signatureHex = toHex(sig.data(), sig.size());
        return tx;
    }

    bool verify() const {
        auto pubBytes = fromHex(senderPubKeyHex);
        auto sigBytes = fromHex(signatureHex);
        if (pubBytes.size() != kEd25519KeyLen || sigBytes.size() != kEd25519SigLen) return false;

        PubKeyBytes pub{};
        std::copy(pubBytes.begin(), pubBytes.end(), pub.begin());
        SigBytes sig{};
        std::copy(sigBytes.begin(), sigBytes.end(), sig.begin());

        return Wallet::verify(pub, signingPayload(), sig);
    }

    // Prototype wire line, '|'-separated. Not the real Wire Format Spec.
    std::string toLine() const {
        std::ostringstream oss;
        oss << type << '|' << sender << '|' << receiver << '|' << amount << '|' << nonce << '|'
            << timestamp << '|' << senderPubKeyHex << '|' << signatureHex;
        return oss.str();
    }

    static Transaction fromLine(const std::string& line) {
        Transaction tx;
        std::vector<std::string> f;
        std::string cur;
        for (char c : line) {
            if (c == '|') { f.push_back(cur); cur.clear(); }
            else cur.push_back(c);
        }
        f.push_back(cur);
        if (f.size() < 8) return tx;
        tx.type = f[0];
        tx.sender = f[1];
        tx.receiver = f[2];
        tx.amount = std::stoull(f[3]);
        tx.nonce = std::stoull(f[4]);
        tx.timestamp = std::stoll(f[5]);
        tx.senderPubKeyHex = f[6];
        tx.signatureHex = f[7];
        return tx;
    }
};

} // namespace megabyte::wallet
