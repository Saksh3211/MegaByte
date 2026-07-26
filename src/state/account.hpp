#pragma once
// megabyte::state â€” Account + StateStore (prototype scope)
// Real design: Master Spec Â§5 (Sparse Merkle Tree, RocksDB). This is an
// in-memory map â€” no state root, no persistence (Roadmap Milestone 2/3
// upgrade this later). Good enough to make balances/usernames real today.

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include "../blockchain/block.hpp"
#include "../wallet/transaction.hpp"

namespace megabyte::state {

struct Account {
    std::string address;
    uint64_t balance = 0;
    uint64_t nonce = 0;
    std::string username; // empty = none registered
};

class StateStore {
public:
    uint64_t getBalance(const std::string& addr) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = accounts_.find(addr);
        return it == accounts_.end() ? 0 : it->second.balance;
    }

    uint64_t getNonce(const std::string& addr) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = accounts_.find(addr);
        return it == accounts_.end() ? 0 : it->second.nonce;
    }

    std::optional<std::string> resolveUsername(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = usernames_.find(name);
        if (it == usernames_.end()) return std::nullopt;
        return it->second;
    }

    std::string usernameOf(const std::string& addr) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = accounts_.find(addr);
        return it == accounts_.end() ? "" : it->second.username;
    }

    // Applies every tx/coinbase line in a block. Best-effort: an invalid
    // entry is skipped rather than aborting the whole block (a real
    // BlockValidator, Master Spec Â§2, would reject the block instead â€”
    // acceptable simplification since nothing feeds this an untrusted
    // block yet other than our own miner and init-node's already-PoW-
    // validated chain).
    void applyBlock(const blockchain::Block& b) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& line : b.transactions) {
            if (line.rfind("COINBASE|", 0) == 0) {
                applyCoinbaseLocked(line);
            } else if (line.find('|') != std::string::npos) {
                applyTxLocked(wallet::Transaction::fromLine(line));
            }
        }
    }

    // Full state doesn't persist (no storage module yet), so after a chain
    // replacement (init-node/sync) we just replay every block from scratch.
    void rebuildFromBlocks(const std::vector<blockchain::Block>& blocks) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            accounts_.clear();
            usernames_.clear();
        }
        for (const auto& b : blocks) applyBlock(b);
    }

private:
    Account& getOrCreateLocked(const std::string& addr) {
        auto it = accounts_.find(addr);
        if (it != accounts_.end()) return it->second;
        Account a;
        a.address = addr;
        return accounts_.emplace(addr, a).first->second;
    }

    void applyCoinbaseLocked(const std::string& line) {
        // "COINBASE|<address>|<amount>"
        auto p1 = line.find('|');
        auto p2 = line.find('|', p1 + 1);
        if (p1 == std::string::npos || p2 == std::string::npos) return;
        std::string addr = line.substr(p1 + 1, p2 - p1 - 1);
        uint64_t amount = std::stoull(line.substr(p2 + 1));
        getOrCreateLocked(addr).balance += amount;
    }

    void applyTxLocked(const wallet::Transaction& tx) {
        if (tx.sender.empty() || !tx.verify()) return;
        Account& sender = getOrCreateLocked(tx.sender);
        if (tx.nonce != sender.nonce) return; // out-of-order/replay, drop

        if (tx.type == "REGISTER_USERNAME") {
            const std::string& username = tx.receiver; // repurposed field
            if (username.empty() || usernames_.count(username)) return;
            sender.username = username;
            usernames_[username] = sender.address;
            sender.nonce++;
            return;
        }

        // TRANSFER
        if (sender.balance < tx.amount) return;
        sender.balance -= tx.amount;
        sender.nonce++;
        getOrCreateLocked(tx.receiver).balance += tx.amount;
    }

    mutable std::mutex mutex_;
    std::unordered_map<std::string, Account> accounts_;
    std::unordered_map<std::string, std::string> usernames_; // username -> address
};

} // namespace megabyte::state
