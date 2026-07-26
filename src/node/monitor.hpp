#pragma once
// megabyte::node — monitor window launcher + display loop.
//
// launchMonitorWindow(): spawns a *new* OS console window running
// `<this exe> --monitor <statusPort>`. On Windows this uses CreateProcess
// with CREATE_NEW_CONSOLE, which is the standard way to open a second
// console from a console app — I can't run a Windows executable in this
// sandbox, so this path is written to the documented Win32 API but not
// executed by me; test it on your machine and tell me what happens.
// The POSIX fallback (for reference/Linux dev use) tries common terminal
// emulators via fork/exec.
//
// runMonitorLoop(): what that second window actually runs — polls the
// status server every 500ms and redraws a small live dashboard.

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include "../net/sockets_compat.hpp"

#ifdef _WIN32
  #include <windows.h>
#else
  #include <unistd.h>
  #include <sys/wait.h>
#endif

namespace megabyte::node {

inline bool launchMonitorWindow(const std::string& exePath, int statusPort) {
    std::string args = " --monitor " + std::to_string(statusPort);

#ifdef _WIN32
    std::string cmdLine = "\"" + exePath + "\"" + args;
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    BOOL ok = CreateProcessA(
        nullptr,
        cmdLine.data(),      // mutable buffer required by CreateProcessA
        nullptr, nullptr,
        FALSE,
        CREATE_NEW_CONSOLE,  // <-- opens a genuinely separate console window
        nullptr, nullptr,
        &si, &pi);

    if (ok) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }
    return false;
#else
    // Linux/macOS dev-machine fallback: try a few common terminal emulators.
    pid_t pid = fork();
    if (pid == 0) {
        std::string full = exePath + args;
        execlp("x-terminal-emulator", "x-terminal-emulator", "-e", full.c_str(), (char*)nullptr);
        execlp("gnome-terminal", "gnome-terminal", "--", "bash", "-c", full.c_str(), (char*)nullptr);
        execlp("xterm", "xterm", "-e", full.c_str(), (char*)nullptr);
        _exit(127); // none found
    }
    return pid > 0;
#endif
}

inline void runMonitorLoop(int statusPort) {
    using namespace std::chrono_literals;
    std::cout << "MegaByte monitor — polling status on port " << statusPort << "\n";
    std::cout << "(Ctrl+C to close this window)\n\n";

    while (true) {
        socket_t sock = net::connectTo("127.0.0.1", statusPort);
        std::string line = "(node not reachable)";
        if (sock != MBC_INVALID_SOCKET) {
            line = net::recvLine(sock);
            net::closeSocket(sock);
        }

#ifdef _WIN32
        system("cls");
#else
        std::cout << "\033[2J\033[H"; // clear + home, for Linux dev use
#endif
        std::cout << "=== MegaByte Node Monitor ===\n\n";
        // status.snapshot() format: key=value pairs space-separated
        std::istringstream iss(line);
        std::string token;
        while (iss >> token) {
            auto eq = token.find('=');
            if (eq == std::string::npos) continue;
            std::cout << "  " << token.substr(0, eq) << ": " << token.substr(eq + 1) << "\n";
        }
        std::cout << "\n(refreshing every 500ms)\n";

        std::this_thread::sleep_for(500ms);
    }
}

} // namespace megabyte::node
