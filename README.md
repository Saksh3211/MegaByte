# MegaByte Prototype (TUI + P2P + wallet placeholder)

A real, compiling proof-of-concept: SHA-512d hashing → block structure →
multi-threaded proof-of-work mining → an in-memory chain → basic TCP
peer-to-peer sync → a placeholder Ed25519 wallet that can sign and
broadcast transactions. Every command below was actually run and verified
during development (two real local node processes talking over TCP,
mining, syncing, and exchanging a signed transaction).

## Folder layout
```
megabyte-prototype/
├── CMakeLists.txt
├── vcpkg.json
├── setup.ps1
└── src/
    ├── crypto/       hash.hpp              (SHA-512d)
    ├── blockchain/   block.hpp, chain.hpp  (thread-safe chain, wire lines)
    ├── consensus/    proof_of_work.hpp
    ├── net/          sockets_compat.hpp    (Winsock/BSD sockets shim)
    ├── p2p/          peer_server.hpp, peer_client.hpp
    ├── wallet/       keystore.hpp, transaction.hpp   (PLACEHOLDER — see below)
    └── node/         main.cpp, miner.hpp, status.hpp, monitor.hpp
```

## First-time setup (Windows 11 + VS Code)
1. Install [Git](https://git-scm.com/download/win), [CMake](https://cmake.org/download/),
   and the "Desktop development with C++" workload (Visual Studio 2022 Build Tools),
   or Clang via VS Code's C/C++ extension.
2. Open this folder in VS Code, open the integrated terminal (make sure it's PowerShell).
3. Run:
   ```powershell
   .\setup.ps1
   ```
4. It'll build and launch `megabyte_node.exe` automatically.

## Commands
```
wallet                show this node's address/pubkey (created automatically on first run)
send <addr> <amount>  sign + broadcast a placeholder transaction
init-node             connect to known peers, download the tallest valid chain
set-peer <host:port>  remember a peer without connecting yet
peers                 list known peers
join-net <host:port>  connect + remember a peer, start listening for inbound peers
listen [port]         start accepting inbound peer connections (default 7777)
sync                  re-run init-node
mine -threads <n>     start mining with n worker threads
stop-mining           stop all mining threads
monitor               open a live dashboard in a SEPARATE window
status                print current stats in this window
add <text>            synchronous single-block mine+append (no networking)
print | validate | height | difficulty <n> | help | exit
```

Run with `megabyte_node.exe --port 7779` to pick a different P2P port —
useful if you're running two nodes on the same machine to test against
each other (each also gets its own status port, `p2pPort + 1`).

## Try it with two nodes (two VS Code terminals, or two folders)
**Terminal 1** (in a folder, e.g. `nodeA\`):
```
.\megabyte_node.exe
mbc> listen 7777
mbc> mine -threads 4
mbc> monitor
```
`monitor` pops open a second window with a live-refreshing dashboard
(height, hashrate, thread count, peer count) — that's `node/monitor.hpp`
polling `node/status.hpp`'s local TCP status server every 500ms.

**Terminal 2** (in a different folder, e.g. `nodeB\`, so it gets its own `wallet.dat`):
```
.\megabyte_node.exe --port 7779
mbc> join-net 127.0.0.1:7777
mbc> init-node
mbc> send mbc1<some address> 42
```
`join-net` connects, remembers the peer, starts listening for inbound
connections, and immediately announces this node's wallet address to the
peer. `init-node` asks all known peers for their height, picks the
tallest, and downloads/validates their chain. `send` signs a transaction
with this node's private key and broadcasts it — node A's window will
print `[peer] tx received ... (signature OK, not yet mempooled)`.

## What "wallet" and "send" actually do right now — read this before relying on either
This is a **placeholder**, not the real design (Master Spec §13 is the
real one — BIP-39 mnemonic, HD derivation, Argon2id + AES-256-GCM at
rest). Today:
- One Ed25519 keypair is generated on first run and saved to `wallet.dat`
  **in plain text, unencrypted**. Anyone who can read that file has the
  funds. Don't put anything of value behind it.
- `send` builds and signs a real transaction and broadcasts it to known
  peers, who verify the signature — but there's no mempool yet, so it is
  **never included in a mined block**. It's proof that signing/verification
  works end to end, nothing more.
- There's no balance or account state at all yet (that's Roadmap
  Milestone 2 — `state/` — genuinely the next thing worth building).

## What's still missing (and where it's specified / when it's due)
| Missing piece | Where it's designed | Roadmap milestone |
|---|---|---|
| Account/state module (balances, nonces enforced) | Master Spec §5 | Milestone 2 |
| Mempool (transactions actually get mined) | Master Spec §7 | Milestone 4 |
| Difficulty adjustment (currently fixed) | Master Spec §2 (LWMA) | Milestone 5 |
| Real fork-choice (currently "longest chain", not cumulative work) | Master Spec §2 | Milestone 5 |
| Encrypted wallet, mnemonic, HD keys | Master Spec §13 | Milestone 8 |
| Real wire protocol + Noise transport encryption | Master Spec §10 | Milestone 6 |
| Persistent storage (chain is lost on exit) | Master Spec §6 | Milestone 3 |

## Rebuilding after you edit source
```powershell
cmake --build build --config Release
.\build\Release\megabyte_node.exe
```
