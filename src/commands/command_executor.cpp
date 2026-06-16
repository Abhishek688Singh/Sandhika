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

void set_nonblocking(int fd) {
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        (void)fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

void append_available(int fd, std::string& output) {
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

    const pid_t pid = fork();
    if (pid < 0) {
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        close(stderr_pipe[0]);
        close(stderr_pipe[1]);
        throw std::runtime_error("Failed to fork command process");
    }

    if (pid == 0) {
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);
        execl("/bin/sh", "sh", "-lc", command.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    close(stdout_pipe[1]);
    close(stderr_pipe[1]);
    set_nonblocking(stdout_pipe[0]);
    set_nonblocking(stderr_pipe[0]);

    CommandExecutionResult result;
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    int status = 0;
    bool child_done = false;

    while (!child_done) {
        append_available(stdout_pipe[0], result.stdout_text);
        append_available(stderr_pipe[0], result.stderr_text);

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
        FD_SET(stdout_pipe[0], &read_fds);
        FD_SET(stderr_pipe[0], &read_fds);
        timeval wait_time {.tv_sec = 0, .tv_usec = 50000};
        const int max_fd = std::max(stdout_pipe[0], stderr_pipe[0]) + 1;
        (void)select(max_fd, &read_fds, nullptr, nullptr, &wait_time);
    }

    append_available(stdout_pipe[0], result.stdout_text);
    append_available(stderr_pipe[0], result.stderr_text);
    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

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
