#pragma once

#include <chrono>
#include <future>
#include <string>

namespace sandhika::commands {

struct CommandExecutionResult {
    int exit_code {-1};
    bool timed_out {false};
    std::string stdout_text;
    std::string stderr_text;
};

class CommandExecutor {
public:
    CommandExecutor() = default;

    [[nodiscard]] std::future<CommandExecutionResult> executeAsync(
        std::string command,
        std::chrono::milliseconds timeout = std::chrono::seconds(30)) const;

    [[nodiscard]] CommandExecutionResult execute(
        const std::string& command,
        std::chrono::milliseconds timeout = std::chrono::seconds(30)) const;
};

}  // namespace sandhika::commands
