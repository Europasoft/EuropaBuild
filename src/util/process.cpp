// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <array>
#include <cstring>
#include <iostream>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#else
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "process.hpp"

namespace Util {

#define READ 0
#define WRITE 1

namespace {}

Result process(const std::vector<std::string> & cmd, const char * cwd) {
#ifdef _WIN32
    // Windows implementation using CreateProcess
    std::string out{}, err{};
    
    // Create command line string
    std::string cmdline;
    for (size_t i = 0; i < cmd.size(); ++i) {
        if (i > 0) cmdline += " ";
        cmdline += cmd[i];
    }
    
    // Create pipes for stdout and stderr
    HANDLE hOutRead, hOutWrite, hErrRead, hErrWrite;
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
    
    if (!CreatePipe(&hOutRead, &hOutWrite, &sa, 0) ||
        !CreatePipe(&hErrRead, &hErrWrite, &sa, 0)) {
        return Result{1, "", "Failed to create pipes"};
    }
    
    // Set up process startup info
    STARTUPINFOA si = {sizeof(STARTUPINFOA)};
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hOutWrite;
    si.hStdError = hErrWrite;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    
    PROCESS_INFORMATION pi = {};
    
    // Create the process
    BOOL success = CreateProcessA(
        NULL,                    // Application name
        const_cast<char*>(cmdline.c_str()), // Command line
        NULL,                    // Process security attributes
        NULL,                    // Thread security attributes
        TRUE,                    // Inherit handles
        0,                       // Creation flags
        NULL,                    // Environment
        cwd,                     // Current directory
        &si,                     // Startup info
        &pi                      // Process information
    );
    
    // Close write handles in parent
    CloseHandle(hOutWrite);
    CloseHandle(hErrWrite);
    
    if (!success) {
        CloseHandle(hOutRead);
        CloseHandle(hErrRead);
        return Result{1, "", "Failed to create process"};
    }
    
    // Read output
    char buffer[4096];
    DWORD bytesRead;
    
    while (ReadFile(hOutRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        out += buffer;
    }
    
    while (ReadFile(hErrRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        err += buffer;
    }
    
    // Wait for process to complete
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    
    // Cleanup
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hOutRead);
    CloseHandle(hErrRead);
    
    return Result{static_cast<int8_t>(exitCode), out, err};
    
#else
    // Unix/Linux implementation (original code)
    std::string out{}, err{};
    int out_pipes[2];
    int err_pipes[2];
    if (pipe(out_pipes) != 0) {
        throw std::runtime_error{"failed to create stdout pipes"};
    }
    if (pipe(err_pipes) != 0) {
        throw std::runtime_error{"failed to create stderr pipes"};
    }

    pid_t pid = fork();
    if (pid == 0) {
        dup2(out_pipes[WRITE], STDOUT_FILENO);
        dup2(err_pipes[WRITE], STDERR_FILENO);
        close(out_pipes[READ]);
        close(out_pipes[WRITE]);
        close(err_pipes[READ]);
        close(err_pipes[WRITE]);

        char * c_cmd[cmd.size() + 1];
        for (unsigned i = 0; i < cmd.size(); ++i) {
            c_cmd[i] = strdup(cmd[i].c_str());
        }
        c_cmd[cmd.size()] = nullptr;

        if (cwd != nullptr) {
            chdir(cwd);
        }
        execvp(c_cmd[0], c_cmd);
        std::cerr << "Program failed to execute: " << strerror(errno) << std::endl;
        _exit(127);
    }

    close(out_pipes[WRITE]);
    close(err_pipes[WRITE]);

    std::array<char, 16384> buffer{};
    int status;
    ssize_t count = 0;

    std::array<pollfd, 2> fds;
    fds[0] = {out_pipes[READ], POLLIN, 0};
    fds[1] = {err_pipes[READ], POLLIN, 0};

    while (true) {
        int rt = poll(fds.data(), fds.size(), 5 * 1000);

        if (rt < 0) {
            std::cerr << "Error: " << strerror(errno) << std::endl;
        } else if (rt == 0) {
            kill(pid, SIGKILL);
            for (auto const & f : fds) {
                close(f.fd);
            }
            throw std::runtime_error{"timeout of 5 seconds elapsed"};
        }

        if (fds[0].revents & POLLIN) {
            count = read(out_pipes[READ], buffer.data(), buffer.size());
            out.append(buffer.begin(), buffer.begin() + count);
        }
        if (fds[1].revents & POLLIN) {
            count = read(err_pipes[READ], buffer.data(), buffer.size());
            err.append(buffer.begin(), buffer.begin() + count);
        }

        if (fds[0].revents & POLLHUP and fds[1].revents & POLLHUP) {
            break;
        }
    }

    for (auto const & f : fds) {
        close(f.fd);
    }

    while (waitpid(pid, &status, 0) == -1)
        ;

    if (status > 255) {
        status %= 255;
    }

    if (status > 128) {
        status -= 128;
        status *= -1;
    }

    return Result{status, out, err};
#endif
};

} // namespace Util
