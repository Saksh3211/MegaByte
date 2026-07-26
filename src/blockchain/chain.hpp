#pragma once
// megabyte::blockchain â€” Chain
// Prototype scope: single-process, in-memory only (no storage/database
// module yet â€” Roadmap Milestone 3). Now thread-safe: multiple mining
// threads and the P2P server thread all touch this concurrently.
// Fork choice here is "longest valid chain wins" â€” NOT the real
// cumulative-work rule from Master Spec Â§2 (that's Milestone 5); fine for
// a same-difficulty prototype, wrong once difficulty varies.

#include <iostream>
#include <mutex>
#include <vector>
#include "block.hpp"
#include "../consensus/proof_of_work.hpp"

namespace megabyte::blockchain {

class Chain {
public:
    Chain() {
        // Genesis block â€” zero-premine, per Master Spec Â§17.
        Block genesis;
        genesis.height = 0;
        genesis.timestamp = Block::nowUnix();
        genesis.difficulty = difficulty_;
        genesis.previousHash = std::string(128, '0'); // 64 bytes hex = 128 chars
        genesis.transactions = {"genesis"};
        consensus::mineBlock(genesis);
        blocks_.push_back(genesis);
    }

    // A ready-to-mine candidate block (height/prevHash/difficulty filled
    // in, nonce still 0, transactions = whatever the miner passes in â€”
    // normally a coinbase line + mempool-sourced tx lines). Miner threads
    // call this, then search the nonce themselves without holding the lock.
    Block buildCandidate(const std::vector<std::string>& txLines) {
        std::lock_guard<std::mutex> lock(mutex_);
        Block b;
        b.height = blocks_.back().height + 1;
        b.timestamp = Block::nowUnix();
        b.difficulty = difficulty_;
        b.previousHash = blocks_.back().computeHash();
        b.transactions = txLines;
        return b;
    }

    // Called by a miner thread after finding a valid nonce. Returns false
    // (and does nothing) if the tip moved on while this thread was mining
    // â€” the thread should discard its candidate and call buildCandidate again.
    bool tryAppendMinedBlock(const Block& b) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (b.previousHash != blocks_.back().computeHash()) return false;
        if (!consensus::meetsDifficulty(b.computeHash(), b.difficulty)) return false;
        blocks_.push_back(b);
        return true;
    }

    // Used by the CLI's synchronous "add" command (single-threaded path).
    const Block addTransactionAndMineSync(const std::string& txData) {
        while (true) {
            Block b = buildCandidate({txData});
            consensus::mineBlock(b);
            if (tryAppendMinedBlock(b)) return b;
            // tip moved during mining (a background miner thread beat us) â€” retry
        }
    }

    bool isValid() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return isValidLocked();
    }

    void print() const {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& b : blocks_) {
            std::cout << "Block #" << b.height
                      << " | nonce=" << b.nonce
                      << " | hash=" << b.computeHash().substr(0, 16) << "..."
                      << " | prev=" << b.previousHash.substr(0, 16) << "..."
                      << " | data=";
            for (const auto& tx : b.transactions) std::cout << tx << " ";
            std::cout << "\n";
        }
    }

    size_t height() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return blocks_.back().height;
    }

    std::string tipHash() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return blocks_.back().computeHash();
    }

    void setDifficulty(uint32_t d) {
        std::lock_guard<std::mutex> lock(mutex_);
        difficulty_ = d;
    }

    uint32_t difficulty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return difficulty_;
    }

    // Dump every block as wire-lines, for GET_CHAIN responses (p2p/peer_server.hpp).
    std::vector<std::string> dumpLines() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> lines;
        lines.reserve(blocks_.size());
        for (const auto& b : blocks_) lines.push_back(b.toLine());
        return lines;
    }

    // Replace the local chain with `candidate` if it is longer AND fully
    // valid. Used by init-node / sync (p2p/peer_client.hpp). This is
    // "longest chain wins" â€” see the class-level note on why that's a
    // prototype simplification, not the real fork-choice rule.
    bool replaceIfLongerAndValid(const std::vector<Block>& candidate) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (candidate.empty()) return false;
        if (candidate.size() <= blocks_.size()) return false;
        if (candidate.front().height != 0) return false;
        for (size_t i = 1; i < candidate.size(); ++i) {
            const Block& prev = candidate[i - 1];
            const Block& cur = candidate[i];
            if (cur.height != prev.height + 1) return false;
            if (cur.previousHash != prev.computeHash()) return false;
            if (!consensus::meetsDifficulty(cur.computeHash(), cur.difficulty)) return false;
        }
        blocks_ = candidate;
        difficulty_ = blocks_.back().difficulty;
        return true;
    }

private:
    bool isValidLocked() const {
        for (size_t i = 1; i < blocks_.size(); ++i) {
            const Block& prev = blocks_[i - 1];
            const Block& cur = blocks_[i];
            if (cur.previousHash != prev.computeHash()) return false;
            if (!consensus::meetsDifficulty(cur.computeHash(), cur.difficulty)) return false;
        }
        return true;
    }

    mutable std::mutex mutex_;
    std::vector<Block> blocks_;
    uint32_t difficulty_ = 3; // 3 leading hex zeros â€” a few seconds/block on a laptop
};

} // namespace megabyte::blockchain
