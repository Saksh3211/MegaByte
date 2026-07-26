#pragma once
// megabyte::node â€” Miner
// Now pulls real pending transactions from the mempool and pays itself a
// fixed coinbase reward (line format "COINBASE|<address>|<amount>",
// state::StateStore::applyBlock knows how to read it). Still polls for a
// fresh candidate each cycle rather than reacting to new-tip notifications
// â€” Roadmap Milestone 7 replaces that; "mine -threads N" maps directly to
// threadCount here either way.

#include <atomic>
#include <thread>
#include <vector>
#include "../blockchain/chain.hpp"
#include "../consensus/proof_of_work.hpp"
#include "../mempool/mempool.hpp"
#include "../state/account.hpp"
#include "status.hpp"

namespace megabyte::node {

constexpr uint64_t kCoinbaseReward = 50;
constexpr size_t kMaxTxPerBlock = 20;

class Miner {
public:
    Miner(blockchain::Chain& chain, state::StateStore& state, mempool::Mempool& mempool,
          NodeStatus& status, std::string minerAddress)
        : chain_(chain), state_(state), mempool_(mempool), status_(status),
          minerAddress_(std::move(minerAddress)) {}

    void start(int threadCount) {
        stop();
        stopFlag_ = false;
        status_.mining = true;
        status_.miningThreads = threadCount;
        for (int i = 0; i < threadCount; ++i) {
            workers_.emplace_back([this, i] { workerLoop(i); });
        }
        hashrateThread_ = std::thread([this] { hashrateLoop(); });
    }

    void stop() {
        stopFlag_ = true;
        for (auto& t : workers_) if (t.joinable()) t.join();
        workers_.clear();
        if (hashrateThread_.joinable()) hashrateThread_.join();
        status_.mining = false;
        status_.miningThreads = 0;
        status_.hashesLastSecond = 0;
    }

    ~Miner() { stop(); }

private:
    void workerLoop(int workerId) {
        while (!stopFlag_) {
            std::vector<std::string> txLines;
            txLines.push_back("COINBASE|" + minerAddress_ + "|" + std::to_string(kCoinbaseReward));
            for (auto& line : mempool_.take(kMaxTxPerBlock)) txLines.push_back(std::move(line));

            blockchain::Block candidate = chain_.buildCandidate(txLines);
            candidate.nonce = static_cast<uint64_t>(workerId) << 48; // spread search space

            while (!stopFlag_) {
                std::string hash = candidate.computeHash();
                hashesThisSecond_.fetch_add(1, std::memory_order_relaxed);
                if (consensus::meetsDifficulty(hash, candidate.difficulty)) {
                    if (chain_.tryAppendMinedBlock(candidate)) {
                        state_.applyBlock(candidate);
                        status_.height = chain_.height();
                        status_.difficulty = chain_.difficulty();
                    }
                    break; // fresh candidate next outer-loop iteration either way
                }
                candidate.nonce++;
            }
        }
    }

    void hashrateLoop() {
        using namespace std::chrono_literals;
        while (!stopFlag_) {
            std::this_thread::sleep_for(1s);
            status_.hashesLastSecond = hashesThisSecond_.exchange(0);
        }
    }

    blockchain::Chain& chain_;
    state::StateStore& state_;
    mempool::Mempool& mempool_;
    NodeStatus& status_;
    std::string minerAddress_;
    std::vector<std::thread> workers_;
    std::thread hashrateThread_;
    std::atomic<bool> stopFlag_{true};
    std::atomic<uint64_t> hashesThisSecond_{0};
};

} // namespace megabyte::node
