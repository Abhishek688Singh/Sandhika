#include "health_reminder/commands/command_executor.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <cstring>
#include <stdexcept>
#include <thread>

namespace health_reminder::commands {
namespace {

struct FdHelper {
    int fd {-1};
    ~FdHelper() {
        if (fd >= 0) {
            close(fd);
        }
    }
    void reset() {
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
    }
};

void set_nonblocking(int fd) {
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        (void)fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

void append_available(int fd, std::string& output) {
    if (fd < 0) return;
    std::array<char, 4096> buffer {};
    for (;;) {
        const ssize_t count = read(fd, buffer.data(), buffer.size());
        if (count > 0) {
            output.append(buffer.data(), static_cast<std::size_t>(count));
            continue;
        }
        break;
    }
}

}  // namespace

std::future<CommandExecutionResult> CommandExecutor::executeAsync(
    std::string command,
    std::chrono::milliseconds timeout) const {
    return std::async(std::launch::async, [this, command = std::move(command), timeout] {
        return execute(command, timeout);
    });
}

CommandExecutionResult CommandExecutor::execute(const std::string& command, std::chrono::milliseconds timeout) const {
    if (command.empty()) {
        throw std::invalid_argument("Command must not be empty");
    }
    if (timeout <= std::chrono::milliseconds::zero()) {
        throw std::invalid_argument("Command timeout must be positive");
    }

    int stdout_pipe[2] {};
    int stderr_pipe[2] {};
    if (pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0) {
        throw std::runtime_error("Failed to create command pipes");
    }

    FdHelper out_read{stdout_pipe[0]};
    FdHelper out_write{stdout_pipe[1]};
    FdHelper err_read{stderr_pipe[0]};
    FdHelper err_write{stderr_pipe[1]};

    const pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error("Failed to fork command process");
    }

    if (pid == 0) {
        out_read.reset();
        err_read.reset();
        dup2(out_write.fd, STDOUT_FILENO);
        dup2(err_write.fd, STDERR_FILENO);
        out_write.reset();
        err_write.reset();
        execl("/bin/sh", "sh", "-lc", command.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    out_write.reset();
    err_write.reset();
    set_nonblocking(out_read.fd);
    set_nonblocking(err_read.fd);

    CommandExecutionResult result;
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    int status = 0;
    bool child_done = false;

    while (!child_done) {
        append_available(out_read.fd, result.stdout_text);
        append_available(err_read.fd, result.stderr_text);

        const pid_t wait_result = waitpid(pid, &status, WNOHANG);
        if (wait_result == pid) {
            child_done = true;
            break;
        }

        if (std::chrono::steady_clock::now() >= deadline) {
            result.timed_out = true;
            kill(pid, SIGTERM);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            if (waitpid(pid, &status, WNOHANG) == 0) {
                kill(pid, SIGKILL);
            }
            waitpid(pid, &status, 0);
            child_done = true;
            break;
        }

        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(out_read.fd, &read_fds);
        FD_SET(err_read.fd, &read_fds);
        timeval wait_time {.tv_sec = 0, .tv_usec = 50000};
        const int max_fd = std::max(out_read.fd, err_read.fd) + 1;
        (void)select(max_fd, &read_fds, nullptr, nullptr, &wait_time);
    }

    append_available(out_read.fd, result.stdout_text);
    append_available(err_read.fd, result.stderr_text);

    if (result.timed_out) {
        result.exit_code = -1;
    } else if (WIFEXITED(status)) {
        result.exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        result.exit_code = 128 + WTERMSIG(status);
    }

    return result;
}

}  // namespace health_reminder::commands
