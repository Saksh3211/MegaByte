#pragma once
// megabyte::consensus — ProofOfWork
// Prototype scope: "difficulty" here is just a count of required leading hex
// zero digits, and there is NO difficulty adjustment yet (Master Spec §2
// specifies LWMA — that's Roadmap Milestone 5, not this prototype). This
// file is deliberately small so it's easy to see exactly what to replace
// when you implement the real algorithm.

#include "../blockchain/block.hpp"
#include "../crypto/hash.hpp"

namespace megabyte::consensus {

inline bool meetsDifficulty(const std::string& hexHash, uint32_t difficulty) {
    return crypto::leadingZeroHexDigits(hexHash) >= static_cast<int>(difficulty);
}

// Brute-force nonce search. Real node code (Roadmap Milestone 7) runs this on
// a dedicated mining thread and aborts early if a new tip arrives from the
// network — this prototype just runs it to completion, single-threaded.
inline void mineBlock(blockchain::Block& block) {
    while (true) {
        std::string hash = block.computeHash();
        if (meetsDifficulty(hash, block.difficulty)) {
            return;
        }
        block.nonce++;
    }
}

} // namespace megabyte::consensus
