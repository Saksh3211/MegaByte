#pragma once
// megabyte::p2p â€” PeerServer
// Prototype scope: a plain-text, unencrypted request/response protocol â€”
// NOT the real message catalog from Master Spec Â§10 (VERSION/VERACK/Noise
// handshake/etc â€” that's Roadmap Milestone 6). Supports:
//   "HEIGHT"            -> current chain height
//   "GET_CHAIN"         -> every block as one wire-line per line, then "END"
//   "ANNOUNCE <address>" -> placeholder wallet-presence announcement, logged
//                           only (no peer/wallet directory kept yet)
//   "TX <line>"         -> submits to the local mempool (signature, nonce,
//                           balance checked against local state)
// One connection handled at a time per accepted socket, then closed.

#include <atomic>
#include <iostream>
#include <thread>
#include "../net/sockets_compat.hpp"
#include "../blockchain/chain.hpp"
#include "../mempool/mempool.hpp"
#include "../wallet/transaction.hpp"

namespace megabyte::p2p {

class PeerServer {
public:
    PeerServer(blockchain::Chain& chain, mempool::Mempool& mempool)
        : chain_(chain), mempool_(mempool) {}

    bool start(int port) {
        listenSocket_ = net::listenOn(port);
        if (listenSocket_ == MBC_INVALID_SOCKET) return false;
        running_ = true;
        port_ = port;
        thread_ = std::thread([this] { acceptLoop(); });
        return true;
    }

    void stop() {
        running_ = false;
        net::closeSocket(listenSocket_);
        if (thread_.joinable()) thread_.join();
    }

    int port() const { return port_; }

private:
    void acceptLoop() {
        while (running_) {
            if (!net::waitReadable(listenSocket_, 200)) continue;
            socket_t client = accept(listenSocket_, nullptr, nullptr);
            if (client == MBC_INVALID_SOCKET) {
                if (!running_) break;
                continue;
            }
            std::thread(&PeerServer::handleClient, this, client).detach();
        }
    }

    void handleClient(socket_t client) {
        std::string request = net::recvLine(client);
        if (request == "HEIGHT") {
            std::string reply = std::to_string(chain_.height()) + "\n";
            net::sendAll(client, reply);
        } else if (request == "GET_CHAIN") {
            std::string payload;
            for (const auto& line : chain_.dumpLines()) payload += line + "\n";
            payload += "END\n";
            net::sendAll(client, payload);
        } else if (request.rfind("ANNOUNCE ", 0) == 0) {
            std::string address = request.substr(9);
            std::cout << "\n[peer] wallet announced: " << address << "\nmbc> " << std::flush;
            net::sendAll(client, "OK\n");
        } else if (request.rfind("TX ", 0) == 0) {
            auto tx = wallet::Transaction::fromLine(request.substr(3));
            std::string err = mempool_.add(tx);
            std::cout << "\n[peer] tx received " << tx.sender.substr(0, 12) << "... -> "
                      << tx.receiver.substr(0, 12) << "... amount=" << tx.amount
                      << (err.empty() ? " (accepted into mempool)" : (" (rejected: " + err + ")"))
                      << "\nmbc> " << std::flush;
            net::sendAll(client, err.empty() ? "OK\n" : "REJECTED\n");
        }
        net::closeSocket(client);
    }

    blockchain::Chain& chain_;
    mempool::Mempool& mempool_;
    socket_t listenSocket_ = MBC_INVALID_SOCKET;
    std::thread thread_;
    std::atomic<bool> running_{false};
    int port_ = 0;
};

} // namespace megabyte::p2p
