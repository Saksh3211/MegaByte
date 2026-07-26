// MegaByteNode â€” TUI prototype entry point.
//
// Normal mode (no args): interactive command shell.
//   init-node               connect to known peers, download the tallest
//                           valid chain (Master Spec sequence diagram #5)
//   set-peer <host:port>    remember a peer address
//   peers                   list known peers
//   join-net <host:port>    connect + remember a peer, start listening
//                           for inbound peers if not already
//   listen [port]           start accepting inbound peer connections
//                           (default 7777, Master Spec Â§10)
//   sync                    re-run init-node against known peers
//   mine -threads <n>       start mining with n worker threads
//   stop-mining             stop all mining threads
//   monitor                 open a second window with a live dashboard
//   status                  print current stats in this window
//   add <text>              synchronous single-block mine+append (no networking)
//   print | validate | height | difficulty <n> | help | exit
//
// --monitor <port> mode: not interactive â€” runs the dashboard loop that a
// window opened by `monitor` uses. You won't normally type this yourself.

#include <iostream>
#include <optional>
#include <sstream>
#include <vector>

#include "../blockchain/chain.hpp"
#include "../mempool/mempool.hpp"
#include "../p2p/peer_client.hpp"
#include "../p2p/peer_server.hpp"
#include "../state/account.hpp"
#include "../wallet/keystore.hpp"
#include "../wallet/transaction.hpp"
#include "miner.hpp"
#include "monitor.hpp"
#include "status.hpp"

#ifdef _WIN32
  #include <windows.h>
#endif

namespace {

std::string getExePath(const char* argv0) {
#ifdef _WIN32
    char buf[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (n > 0 && n < MAX_PATH) return std::string(buf, n);
#endif
    return argv0 ? argv0 : "megabyte_node";
}

} // namespace

int main(int argc, char** argv) {
    // --- --monitor sub-mode: this process instance IS the monitor window ---
    if (argc >= 3 && std::string(argv[1]) == "--monitor") {
        int statusPort = std::stoi(argv[2]);
        megabyte::node::runMonitorLoop(statusPort);
        return 0;
    }

    if (!megabyte::net::initSockets()) {
        std::cerr << "Failed to initialize networking.\n";
        return 1;
    }

    const std::string exePath = getExePath(argv[0]);
    int p2pPort = 7777;
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string(argv[i]) == "--port") p2pPort = std::stoi(argv[i + 1]);
    }
    const int kStatusPort = p2pPort + 1; // e.g. 7778 for the default 7777

    megabyte::blockchain::Chain chain;
    megabyte::state::StateStore state;
    megabyte::mempool::Mempool mempool(state);
    megabyte::node::NodeStatus status;
    status.height = chain.height();
    status.difficulty = chain.difficulty();

    // --- Wallet: create-or-load, every startup. PLACEHOLDER, see
    // wallet/keystore.hpp header for exactly what's missing vs. Master
    // Spec Â§13 (no encryption yet â€” do not use this for real funds). ---
    const std::string kWalletPath = "wallet.dat";
    megabyte::wallet::Wallet myWallet = megabyte::wallet::Wallet::exists(kWalletPath)
        ? megabyte::wallet::Wallet::load(kWalletPath)
        : megabyte::wallet::Wallet::generate();
    if (!megabyte::wallet::Wallet::exists(kWalletPath)) {
        myWallet.save(kWalletPath);
        std::cout << "No wallet found â€” created a new one.\n";
    } else {
        std::cout << "Loaded existing wallet.\n";
    }
    std::cout << "Wallet address: " << myWallet.address()
              << "  (WARNING: wallet.dat is unencrypted, prototype-only)\n\n";

    megabyte::node::StatusServer statusServer(status);
    statusServer.start(kStatusPort);

    megabyte::p2p::PeerServer p2pServer(chain, mempool);
    bool listening = false;

    megabyte::node::Miner miner(chain, state, mempool, status, myWallet.address());
    std::vector<megabyte::p2p::PeerAddress> knownPeers;

    auto refreshHeightStatus = [&] {
        status.height = chain.height();
        status.difficulty = chain.difficulty();
    };

    auto announceToAllPeers = [&] {
        if (knownPeers.empty()) return;
        std::cout << "Announcing wallet " << myWallet.address() << " to " << knownPeers.size() << " peer(s)...\n";
        for (const auto& peer : knownPeers) {
            bool ok = megabyte::p2p::announceWallet(peer, myWallet.address());
            std::cout << "  " << peer.toString() << (ok ? " acked\n" : " unreachable\n");
        }
    };

    auto doInitNode = [&] {
        announceToAllPeers();
        if (knownPeers.empty()) {
            std::cout << "No known peers. Use set-peer <host:port> or join-net <host:port> first.\n";
            return;
        }
        long bestHeight = -1;
        std::optional<megabyte::p2p::PeerAddress> bestPeer;
        for (const auto& peer : knownPeers) {
            long h = megabyte::p2p::queryHeight(peer);
            std::cout << "  " << peer.toString() << " -> height "
                      << (h < 0 ? std::string("unreachable") : std::to_string(h)) << "\n";
            if (h > bestHeight) { bestHeight = h; bestPeer = peer; }
        }
        if (!bestPeer || bestHeight < 0) {
            std::cout << "No reachable peers.\n";
            return;
        }
        if (static_cast<size_t>(bestHeight) <= chain.height()) {
            std::cout << "Local chain already at least as tall (" << chain.height() << "). Nothing to do.\n";
            return;
        }
        std::cout << "Fetching chain from " << bestPeer->toString() << "...\n";
        auto blocks = megabyte::p2p::fetchChain(*bestPeer);
        if (chain.replaceIfLongerAndValid(blocks)) {
            state.rebuildFromBlocks(blocks);
            refreshHeightStatus();
            std::cout << "Synced. New height: " << chain.height() << "\n";
        } else {
            std::cout << "Peer's chain was not longer/valid â€” kept local chain.\n";
        }
    };

    auto ensureListening = [&] {
        if (!listening) {
            listening = p2pServer.start(p2pPort);
            std::cout << (listening
                              ? "Listening for peers on port " + std::to_string(p2pPort) + "\n"
                              : "Failed to start P2P listener (port in use?)\n");
        }
    };

    std::cout << "MegaByte prototype node (TUI)\n";
    std::cout << "Genesis mined. Type 'help' for commands.\n\n";

    std::string line;
    while (true) {
        std::cout << "mbc> ";
        if (!std::getline(std::cin, line)) break;
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "exit" || cmd == "quit") {
            break;

        } else if (cmd == "help") {
            std::cout <<
                "  wallet                show this node's address/pubkey/username/balance\n"
                "  balance [addr]        show balance (defaults to own wallet)\n"
                "  send <addr> <amount>  sign + mempool + broadcast a transaction\n"
                "  register-username <n> claim a username for this wallet\n"
                "  whois <username>      resolve a username to an address (local state)\n"
                "  mempool               show pending transaction count\n"
                "  init-node             download tallest chain from known peers\n"
                "  set-peer <host:port>  remember a peer\n"
                "  peers                 list known peers\n"
                "  join-net <host:port>  connect + remember peer, start listening\n"
                "  listen [port]         start accepting inbound peers (default 7777)\n"
                "  sync                  re-run init-node\n"
                "  mine -threads <n>     start mining with n threads\n"
                "  stop-mining           stop mining\n"
                "  monitor               open live dashboard in a new window\n"
                "  status                print current stats here\n"
                "  add <text>            synchronous single-block mine+append\n"
                "  print | validate | height | difficulty <n> | exit\n";

        } else if (cmd == "set-peer") {
            std::string addrStr;
            iss >> addrStr;
            auto addr = megabyte::p2p::parsePeerAddress(addrStr);
            if (!addr) { std::cout << "Usage: set-peer <host:port>\n"; continue; }
            knownPeers.push_back(*addr);
            status.peerCount = static_cast<int>(knownPeers.size());
            std::cout << "Added peer " << addr->toString() << "\n";

        } else if (cmd == "peers") {
            if (knownPeers.empty()) std::cout << "(no known peers)\n";
            for (const auto& p : knownPeers) std::cout << "  " << p.toString() << "\n";

        } else if (cmd == "join-net") {
            std::string addrStr;
            iss >> addrStr;
            auto addr = megabyte::p2p::parsePeerAddress(addrStr);
            if (!addr) { std::cout << "Usage: join-net <host:port>\n"; continue; }
            long h = megabyte::p2p::queryHeight(*addr);
            if (h < 0) {
                std::cout << "Could not reach " << addr->toString() << "\n";
                continue;
            }
            knownPeers.push_back(*addr);
            status.peerCount = static_cast<int>(knownPeers.size());
            std::cout << "Connected. Peer height: " << h << "\n";
            ensureListening();
            megabyte::p2p::announceWallet(*addr, myWallet.address());
            std::cout << "Announced wallet " << myWallet.address() << " to " << addr->toString() << "\n";

        } else if (cmd == "listen") {
            int port = p2pPort;
            iss >> port;
            if (listening) {
                std::cout << "Already listening on port " << p2pServer.port() << "\n";
            } else {
                listening = p2pServer.start(port);
                std::cout << (listening ? "Listening on port " + std::to_string(port) + "\n"
                                         : "Failed to listen on port " + std::to_string(port) + "\n");
            }

        } else if (cmd == "init-node" || cmd == "sync") {
            doInitNode();

        } else if (cmd == "mine") {
            announceToAllPeers();
            std::string flag;
            int threads = 1;
            iss >> flag;
            if (flag == "-threads") iss >> threads;
            if (threads < 1) threads = 1;
            miner.start(threads);
            std::cout << "Mining started with " << threads << " thread(s). Try 'monitor' or 'status'.\n";

        } else if (cmd == "stop-mining") {
            miner.stop();
            std::cout << "Mining stopped.\n";

        } else if (cmd == "monitor") {
            bool ok = megabyte::node::launchMonitorWindow(exePath, kStatusPort);
            std::cout << (ok ? "Monitor window launched.\n"
                              : "Could not launch monitor window on this platform.\n");

        } else if (cmd == "status") {
            std::cout << status.snapshot() << "\n";

        } else if (cmd == "wallet") {
            std::cout << "Address:    " << myWallet.address() << "\n";
            std::cout << "Public key: " << myWallet.publicKeyHex() << "\n";
            std::string uname = state.usernameOf(myWallet.address());
            if (!uname.empty()) std::cout << "Username:   " << uname << "\n";
            std::cout << "Balance:    " << state.getBalance(myWallet.address()) << "\n";

        } else if (cmd == "balance") {
            std::string addr;
            iss >> addr;
            if (addr.empty()) addr = myWallet.address();
            std::cout << addr << " -> " << state.getBalance(addr) << "\n";

        } else if (cmd == "register-username") {
            std::string username;
            iss >> username;
            if (username.empty()) { std::cout << "Usage: register-username <name>\n"; continue; }
            uint64_t nonce = state.getNonce(myWallet.address());
            auto tx = megabyte::wallet::Transaction::createUsernameRegistration(myWallet, username, nonce);
            std::string err = mempool.add(tx);
            if (!err.empty()) {
                std::cout << "Rejected locally: " << err << "\n";
                continue;
            }
            std::cout << "Registration submitted to local mempool (will be mined into a block).\n";
            for (const auto& peer : knownPeers) {
                bool ok = megabyte::p2p::broadcastTransaction(peer, tx.toLine());
                std::cout << "  -> " << peer.toString() << (ok ? " accepted\n" : " rejected/unreachable\n");
            }

        } else if (cmd == "whois") {
            std::string username;
            iss >> username;
            auto addr = state.resolveUsername(username);
            std::cout << (addr ? (username + " -> " + *addr + "\n") : "No such username (locally known state only).\n");

        } else if (cmd == "mempool") {
            std::cout << mempool.size() << " pending transaction(s) in local mempool.\n";

        } else if (cmd == "send") {
            std::string toAddress;
            uint64_t amount = 0;
            iss >> toAddress >> amount;
            if (toAddress.empty() || amount == 0) {
                std::cout << "Usage: send <address> <amount>\n";
                continue;
            }
            uint64_t nonce = state.getNonce(myWallet.address());
            auto tx = megabyte::wallet::Transaction::createSigned(myWallet, toAddress, amount, nonce);
            std::string err = mempool.add(tx);
            if (!err.empty()) {
                std::cout << "Rejected locally: " << err << "\n";
                continue;
            }
            std::cout << "Signed tx: " << tx.sender.substr(0, 12) << "... -> "
                      << tx.receiver.substr(0, 12) << "... amount=" << amount
                      << "\nAdded to local mempool (will be mined into a block by any miner, including this node).\n";
            if (!knownPeers.empty()) {
                for (const auto& peer : knownPeers) {
                    bool ok = megabyte::p2p::broadcastTransaction(peer, tx.toLine());
                    std::cout << "  -> " << peer.toString() << (ok ? " accepted\n" : " rejected/unreachable\n");
                }
            }

        } else if (cmd == "add") {
            std::string rest;
            std::getline(iss, rest);
            if (!rest.empty()) rest = rest.substr(1);
            std::cout << "Mining single block synchronously...\n";
            auto b = chain.addTransactionAndMineSync(rest.empty() ? "(empty)" : rest);
            refreshHeightStatus();
            std::cout << "Mined block #" << b.height << " nonce=" << b.nonce << "\n";

        } else if (cmd == "print") {
            chain.print();

        } else if (cmd == "validate") {
            std::cout << (chain.isValid() ? "Chain is valid.\n" : "Chain is INVALID.\n");

        } else if (cmd == "height") {
            std::cout << "Height: " << chain.height() << "\n";

        } else if (cmd == "difficulty") {
            uint32_t d;
            if (iss >> d) {
                chain.setDifficulty(d);
                status.difficulty = d;
                std::cout << "Difficulty set to " << d << "\n";
            } else {
                std::cout << "Usage: difficulty <n>\n";
            }

        } else if (!cmd.empty()) {
            std::cout << "Unknown command. Type 'help'.\n";
        }
    }

    miner.stop();
    if (listening) p2pServer.stop();
    statusServer.stop();
    megabyte::net::cleanupSockets();
    std::cout << "Shutting down.\n";
    return 0;
}
