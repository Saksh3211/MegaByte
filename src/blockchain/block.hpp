#pragma once
// megabyte::blockchain â€” Block
// Prototype scope: header fields match Master Architecture Spec Â§4.3, but
// `transactions` is a plain string list for now (real Transaction type with
// signatures/fees arrives with the wallet+mempool milestones â€” see roadmap
// Milestone 4). merkleRoot/stateRoot are simple placeholder hashes here,
// upgraded to a real Merkle tree / Sparse Merkle Tree in Milestone 2/5.

#include <chrono>
#include <sstream>
#include <string>
#include <vector>
#include "../crypto/hash.hpp"

namespace megabyte::blockchain {

struct Block {
    uint64_t height = 0;
    int64_t timestamp = 0;
    uint32_t difficulty = 1;      // leading hex zero digits required (prototype-only rule)
    uint64_t nonce = 0;
    std::string previousHash;
    std::vector<std::string> transactions; // placeholder for real Transaction objects

    // Canonical serialization used for hashing. This is a stand-in for the
    // real Wire Format Specification (Missing Documents #1) â€” every field
    // must be included here in a fixed order so hashing is deterministic.
    std::string serializeForHash() const {
        std::ostringstream oss;
        oss << height << '|' << timestamp << '|' << difficulty << '|' << nonce
            << '|' << previousHash << '|';
        for (const auto& tx : transactions) oss << tx << ';';
        return oss.str();
    }

    std::string computeHash() const {
        return crypto::toHex(crypto::sha512d(serializeForHash()));
    }

    static int64_t nowUnix() {
        return std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    // Prototype wire format: one block per line, '|' separated fields,
    // transactions ','-joined. NOT the real Wire Format Specification
    // (Missing Documents #1) â€” placeholder for peer sync only.
    std::string toLine() const {
        std::ostringstream oss;
        oss << height << '|' << timestamp << '|' << difficulty << '|' << nonce
            << '|' << previousHash << '|';
        for (size_t i = 0; i < transactions.size(); ++i) {
            if (i) oss << ',';
            oss << transactions[i];
        }
        return oss.str();
    }

    static Block fromLine(const std::string& line) {
        Block b;
        std::vector<std::string> fields;
        std::string cur;
        size_t i = 0;
        // Only the first 5 fields (height/timestamp/difficulty/nonce/prevHash)
        // are '|'-delimited â€” the transactions blob after that may itself
        // contain '|' (transaction wire lines use '|' internally), so it must
        // NOT be split further here.
        for (; i < line.size() && fields.size() < 5; ++i) {
            char c = line[i];
            if (c == '|') { fields.push_back(cur); cur.clear(); }
            else cur.push_back(c);
        }
        if (fields.size() < 5) return b; // malformed, caller should validate
        std::string txBlob = line.substr(i);

        b.height = std::stoull(fields[0]);
        b.timestamp = std::stoll(fields[1]);
        b.difficulty = static_cast<uint32_t>(std::stoul(fields[2]));
        b.nonce = std::stoull(fields[3]);
        b.previousHash = fields[4];
        if (!txBlob.empty()) {
            std::string tx;
            for (char c : txBlob) {
                if (c == ',') { b.transactions.push_back(tx); tx.clear(); }
                else tx.push_back(c);
            }
            b.transactions.push_back(tx);
        }
        return b;
    }
};

} // namespace megabyte::blockchain
