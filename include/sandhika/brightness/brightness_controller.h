#pragma once

#include <functional>
#include <mutex>
#include <optional>
#include <string>

namespace sandhika::brightness {

struct CommandResult {
    int exit_code {0};
    std::string output;
};

class BrightnessController {
public:
    using CommandRunner = std::function<CommandResult(const std::string&)>;

    explicit BrightnessController(CommandRunner command_runner = {});
    ~BrightnessController();

    BrightnessController(const BrightnessController&) = delete;
    BrightnessController& operator=(const BrightnessController&) = delete;

    [[nodiscard]] std::optional<int> getBrightness();
    [[nodiscard]] bool setBrightness(int percent);
    [[nodiscard]] bool dimTo(int percent);
    void restore();

private:
    [[nodiscard]] std::optional<int> getRawValue(const std::string& command);
    [[nodiscard]] CommandResult run(const std::string& command) const;

    CommandRunner command_runner_;
    std::optional<int> original_brightness_;
    std::mutex mutex_;
};

}  // namespace sandhika::brightness
