#include "sandhika/brightness/brightness_controller.h"

#include <array>
#include <cstdio>
#include <stdexcept>

namespace sandhika::brightness {
namespace {

CommandResult default_run_command(const std::string& command) {
    std::array<char, 128> buffer {};
    std::string output;
    FILE* pipe = popen((command + " 2>/dev/null").c_str(), "r");
    if (pipe == nullptr) {
        return CommandResult {.exit_code = 127, .output = {}};
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }
    return CommandResult {.exit_code = pclose(pipe), .output = output};
}

[[nodiscard]] int parse_positive_int(std::string value) {
    while (!value.empty() && (value.back() == '\n' || value.back() == ' ')) {
        value.pop_back();
    }
    return std::stoi(value);
}

}  // namespace

BrightnessController::BrightnessController(CommandRunner command_runner)
    : command_runner_(std::move(command_runner)) {}

BrightnessController::~BrightnessController() {
    restore();
}

std::optional<int> BrightnessController::getBrightness() {
    std::lock_guard lock(mutex_);
    const auto current = getRawValue("brightnessctl get");
    const auto max = getRawValue("brightnessctl max");
    if (!current.has_value() || !max.has_value() || max.value() <= 0) {
        return std::nullopt;
    }
    return std::clamp((current.value() * 100) / max.value(), 0, 100);
}

bool BrightnessController::setBrightness(int percent) {
    if (percent < 0 || percent > 100) {
        throw std::invalid_argument("Brightness percent must be in range [0, 100]");
    }
    std::lock_guard lock(mutex_);
    const auto result = run("brightnessctl set " + std::to_string(percent) + "%");
    return result.exit_code == 0;
}

bool BrightnessController::dimTo(int percent) {
    if (percent < 0 || percent > 100) {
        throw std::invalid_argument("Brightness percent must be in range [0, 100]");
    }
    std::lock_guard lock(mutex_);
    if (!original_brightness_.has_value()) {
        const auto current = getRawValue("brightnessctl get");
        const auto max = getRawValue("brightnessctl max");
        if (!current.has_value() || !max.has_value() || max.value() <= 0) {
            return false;
        }
        original_brightness_ = std::clamp((current.value() * 100) / max.value(), 0, 100);
    }

    const auto result = run("brightnessctl set " + std::to_string(percent) + "%");
    if (result.exit_code != 0) {
        original_brightness_.reset();
        return false;
    }
    return true;
}

void BrightnessController::restore() {
    std::lock_guard lock(mutex_);
    if (!original_brightness_.has_value()) {
        return;
    }
    (void)run("brightnessctl set " + std::to_string(original_brightness_.value()) + "%");
    original_brightness_.reset();
}

std::optional<int> BrightnessController::getRawValue(const std::string& command) {
    const auto result = run(command);
    if (result.exit_code != 0 || result.output.empty()) {
        return std::nullopt;
    }
    try {
        return parse_positive_int(result.output);
    } catch (...) {
        return std::nullopt;
    }
}

CommandResult BrightnessController::run(const std::string& command) const {
    if (command_runner_) {
        return command_runner_(command);
    }
    return default_run_command(command);
}

}  // namespace sandhika::brightness
