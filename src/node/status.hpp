#pragma once
// megabyte::node — NodeStatus + StatusServer
// Shared atomic counters the miner/p2p threads update, plus a tiny local
// TCP server so a separate "monitor" process/window can poll live stats
// without touching shared memory directly (keeps the two processes
// decoupled — same reason a real explorer talks to the node over RPC
// instead of linking against its internals, Master Spec §12).

#include <atomic>
#include <sstream>
#include <string>
#include <thread>
#include "../net/sockets_compat.hpp"

namespace megabyte::node {

struct NodeStatus {
    std::atomic<size_t> height{0};
    std::atomic<uint32_t> difficulty{0};
    std::atomic<int> miningThreads{0};
    std::atomic<uint64_t> hashesLastSecond{0};
    std::atomic<int> peerCount{0};
    std::atomic<bool> mining{false};

    std::string snapshot() const {
        std::ostringstream oss;
        oss << "height=" << height.load()
            << " difficulty=" << difficulty.load()
            << " mining=" << (mining.load() ? "true" : "false")
            << " threads=" << miningThreads.load()
            << " hashrate=" << hashesLastSecond.load() << "H/s"
            << " peers=" << peerCount.load();
        return oss.str();
    }
};

class StatusServer {
public:
    explicit StatusServer(NodeStatus& status) : status_(status) {}

    bool start(int port) {
        listenSocket_ = net::listenOn(port);
        if (listenSocket_ == MBC_INVALID_SOCKET) return false;
        running_ = true;
        thread_ = std::thread([this] { acceptLoop(); });
        return true;
    }

    void stop() {
        running_ = false;
        net::closeSocket(listenSocket_);
        if (thread_.joinable()) thread_.join();
    }

private:
    void acceptLoop() {
        while (running_) {
            if (!net::waitReadable(listenSocket_, 200)) continue;
            socket_t client = accept(listenSocket_, nullptr, nullptr);
            if (client == MBC_INVALID_SOCKET) {
                if (!running_) break;
                continue;
            }
            net::sendAll(client, status_.snapshot() + "\n");
            net::closeSocket(client);
        }
    }

    NodeStatus& status_;
    socket_t listenSocket_ = MBC_INVALID_SOCKET;
    std::thread thread_;
    std::atomic<bool> running_{false};
};

} // namespace megabyte::node
