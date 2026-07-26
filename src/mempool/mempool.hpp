#pragma once
// megabyte::mempool â€” Mempool (prototype scope)
// Real design: Master Spec Â§7/Â§9 (fee-ordered, size-based fees, moving-
// average base fee). This is deliberately simple: FIFO, one pending tx per
// sender (nonce must exactly match their current account nonce), no fee
// concept yet. Good enough to make transactions actually get mined, which
// is the missing piece from the last round.

#include <deque>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>
#include "../state/account.hpp"
#include "../wallet/transaction.hpp"

namespace megabyte::mempool {

class Mempool {
public:
    explicit Mempool(state::StateStore& state) : state_(state) {}

    // Returns "" on success, or a human-readable rejection reason.
    std::string add(const wallet::Transaction& tx) {
        if (!tx.verify()) return "invalid signature";

        std::lock_guard<std::mutex> lock(mutex_);
        uint64_t expectedNonce = state_.getNonce(tx.sender);
        if (tx.nonce != expectedNonce) return "bad nonce (expected " + std::to_string(expectedNonce) + ")";
        if (tx.type == "TRANSFER" && state_.getBalance(tx.sender) < tx.amount) return "insufficient balance";
        if (pendingSenders_.count(tx.sender)) return "sender already has a pending tx";

        pendingSenders_.insert(tx.sender);
        pending_.push_back(tx.toLine());
        return "";
    }

    // Pops up to maxCount pending tx lines for the miner to include.
    // Prototype limitation: if the block they end up in never gets
    // accepted (tip moved), these are gone â€” no re-queue. Fine at this
    // scale; a real mempool (Milestone 4) re-queues on reorg.
    std::vector<std::string> take(size_t maxCount) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> out;
        while (!pending_.empty() && out.size() < maxCount) {
            auto tx = wallet::Transaction::fromLine(pending_.front());
            pendingSenders_.erase(tx.sender);
            out.push_back(pending_.front());
            pending_.pop_front();
        }
        return out;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pending_.size();
    }

private:
    state::StateStore& state_;
    mutable std::mutex mutex_;
    std::deque<std::string> pending_;
    std::unordered_set<std::string> pendingSenders_;
};

} // namespace megabyte::mempool
