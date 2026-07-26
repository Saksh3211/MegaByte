#pragma once
// megabyte::p2p â€” peer client helpers, used by the TUI's join-net/init-node/
// set-peer/sync commands. Talks the same prototype text protocol as
// peer_server.hpp. Every function returns a bool/optional-style success
// flag rather than throwing â€” TUI commands report failure and keep going.

#include <optional>
#include <string>
#include <vector>
#include "../net/sockets_compat.hpp"
#include "../blockchain/block.hpp"

namespace megabyte::p2p {

struct PeerAddress {
    std::string host;
    int port;

    std::string toString() const { return host + ":" + std::to_string(port); }
};

inline std::optional<PeerAddress> parsePeerAddress(const std::string& s) {
    auto pos = s.find(':');
    if (pos == std::string::npos) return std::nullopt;
    PeerAddress addr;
    addr.host = s.substr(0, pos);
    try {
        addr.port = std::stoi(s.substr(pos + 1));
    } catch (...) {
        return std::nullopt;
    }
    return addr;
}

// Returns -1 on failure to connect/communicate.
inline long queryHeight(const PeerAddress& peer) {
    socket_t sock = net::connectTo(peer.host, peer.port);
    if (sock == MBC_INVALID_SOCKET) return -1;
    net::sendAll(sock, "HEIGHT\n");
    std::string reply = net::recvLine(sock);
    net::closeSocket(sock);
    try {
        return std::stol(reply);
    } catch (...) {
        return -1;
    }
}

// Returns an empty vector on failure. Caller distinguishes "peer had an
// empty/invalid chain" from "connection failed" by checking queryHeight
// first, same pattern the real SyncManager (Master Spec sequence diagram
// #5) uses.
inline std::vector<blockchain::Block> fetchChain(const PeerAddress& peer) {
    std::vector<blockchain::Block> blocks;
    socket_t sock = net::connectTo(peer.host, peer.port);
    if (sock == MBC_INVALID_SOCKET) return blocks;
    net::sendAll(sock, "GET_CHAIN\n");
    net::BufferedReader reader(sock);
    while (true) {
        std::string line = reader.readLine();
        if (line == "END" || (line.empty() && reader.eof())) break;
        blocks.push_back(blockchain::Block::fromLine(line));
    }
    net::closeSocket(sock);
    return blocks;
}

// Returns true if the peer acknowledged. Placeholder â€” no retry, no peer
// directory kept on either side (Roadmap Milestone 6 replaces this with
// real VERSION-time capability/identity exchange).
inline bool announceWallet(const PeerAddress& peer, const std::string& address) {
    socket_t sock = net::connectTo(peer.host, peer.port);
    if (sock == MBC_INVALID_SOCKET) return false;
    net::sendAll(sock, "ANNOUNCE " + address + "\n");
    std::string reply = net::recvLine(sock);
    net::closeSocket(sock);
    return reply == "OK";
}

inline bool broadcastTransaction(const PeerAddress& peer, const std::string& txLine) {
    socket_t sock = net::connectTo(peer.host, peer.port);
    if (sock == MBC_INVALID_SOCKET) return false;
    net::sendAll(sock, "TX " + txLine + "\n");
    std::string reply = net::recvLine(sock);
    net::closeSocket(sock);
    return reply == "OK";
}

} // namespace megabyte::p2p
