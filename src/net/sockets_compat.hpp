#pragma once
// megabyte::net â€” thin cross-platform socket shim so p2p/ code doesn't
// need #ifdefs scattered through it. Windows uses Winsock2, POSIX uses
// BSD sockets. This is a prototype convenience layer, not a real
// abstraction â€” Roadmap Milestone 6 replaces this with Boost.Asio.

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "Ws2_32.lib")
  using socket_t = SOCKET;
  #define MBC_INVALID_SOCKET INVALID_SOCKET
  #define MBC_CLOSESOCKET closesocket
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h>
  using socket_t = int;
  #define MBC_INVALID_SOCKET (-1)
  #define MBC_CLOSESOCKET close
#endif

#include <string>
#include <cstring>

namespace megabyte::net {

inline bool initSockets() {
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
    return true;
#endif
}

inline void cleanupSockets() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// Connect to host:port. Returns MBC_INVALID_SOCKET on failure.
inline socket_t connectTo(const std::string& host, int port) {
    struct addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo* res = nullptr;

    std::string portStr = std::to_string(port);
    if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res) != 0) {
        return MBC_INVALID_SOCKET;
    }

    socket_t sock = MBC_INVALID_SOCKET;
    for (auto* p = res; p != nullptr; p = p->ai_next) {
        sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sock == MBC_INVALID_SOCKET) continue;
        if (connect(sock, p->ai_addr, static_cast<int>(p->ai_addrlen)) == 0) break;
        MBC_CLOSESOCKET(sock);
        sock = MBC_INVALID_SOCKET;
    }
    freeaddrinfo(res);
    return sock;
}

// Minimal listening socket, backlog fixed at 16 (prototype scope).
inline socket_t listenOn(int port) {
    socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == MBC_INVALID_SOCKET) return MBC_INVALID_SOCKET;

    int opt = 1;
#ifdef _WIN32
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) != 0) {
        MBC_CLOSESOCKET(sock);
        return MBC_INVALID_SOCKET;
    }
    if (listen(sock, 16) != 0) {
        MBC_CLOSESOCKET(sock);
        return MBC_INVALID_SOCKET;
    }
    return sock;
}

// Send an entire std::string, retrying on partial sends.
inline bool sendAll(socket_t sock, const std::string& data) {
    size_t sent = 0;
    while (sent < data.size()) {
        int n = send(sock, data.data() + sent, static_cast<int>(data.size() - sent), 0);
        if (n <= 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

// Read until '\n' or the peer closes the connection. Simple line protocol,
// fine for the prototype's text-based messages.
inline std::string recvLine(socket_t sock) {
    std::string out;
    char c;
    while (true) {
        int n = recv(sock, &c, 1, 0);
        if (n <= 0) break;
        if (c == '\n') break;
        out.push_back(c);
    }
    return out;
}

// Byte-at-a-time recvLine (below) is fine for short single-line replies
// (HEIGHT, ANNOUNCE, TX) but is far too slow for GET_CHAIN's many-line
// response â€” found during testing (syncing a few thousand blocks took
// long enough to look hung). BufferedReader reads in chunks instead.
class BufferedReader {
public:
    explicit BufferedReader(socket_t sock) : sock_(sock) {}

    // Returns "" (with atEof_ set) once the connection closes with no more data.
    std::string readLine() {
        while (true) {
            auto nl = buf_.find('\n');
            if (nl != std::string::npos) {
                std::string line = buf_.substr(0, nl);
                buf_.erase(0, nl + 1);
                return line;
            }
            if (atEof_) {
                std::string rest = buf_;
                buf_.clear();
                return rest;
            }
            char chunk[4096];
            int n = recv(sock_, chunk, sizeof(chunk), 0);
            if (n <= 0) { atEof_ = true; continue; }
            buf_.append(chunk, static_cast<size_t>(n));
        }
    }

    bool eof() const { return atEof_ && buf_.empty(); }

private:
    socket_t sock_;
    std::string buf_;
    bool atEof_ = false;
};

inline void closeSocket(socket_t sock) {
    if (sock != MBC_INVALID_SOCKET) MBC_CLOSESOCKET(sock);
}

// True if `sock` has a pending connection/data within timeoutMs. Used so
// accept-loop threads can wake up periodically and check a stop flag,
// instead of blocking in accept() forever â€” closing a socket from another
// thread does NOT reliably unblock a thread parked in accept() on it
// (this bit the prototype during testing), so polling is the robust fix.
inline bool waitReadable(socket_t sock, int timeoutMs) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
    struct timeval tv{};
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
#ifdef _WIN32
    int n = select(0, &fds, nullptr, nullptr, &tv);
#else
    int n = select(static_cast<int>(sock) + 1, &fds, nullptr, nullptr, &tv);
#endif
    return n > 0 && FD_ISSET(sock, &fds);
}

} // namespace megabyte::net
