#include "sandhika/idle/idle_detector.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace sandhika::idle {
namespace {

[[nodiscard]] std::optional<std::string> run_command(const char* command) {
    std::array<char, 128> buffer {};
    std::string output;
    FILE* pipe = popen(command, "r");
    if (pipe == nullptr) {
        return std::nullopt;
    }

    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }

    const int status = pclose(pipe);
    if (status != 0 || output.empty()) {
        return std::nullopt;
    }
    return output;
}

}  // namespace

std::optional<std::chrono::milliseconds> XPrintIdleProvider::queryIdleDuration() {
    const auto output = run_command("xprintidle 2>/dev/null");
    if (!output.has_value()) {
        return std::nullopt;
    }

    try {
        return std::chrono::milliseconds(std::stoll(output.value()));
    } catch (...) {
        return std::nullopt;
    }
}

void ManualIdleProvider::markActivity() {
    std::lock_guard lock(mutex_);
    last_activity_ = std::chrono::steady_clock::now();
}

std::optional<std::chrono::milliseconds> ManualIdleProvider::queryIdleDuration() {
    std::lock_guard lock(mutex_);
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - last_activity_);
}

IdleDetector::IdleDetector(std::chrono::milliseconds idle_threshold, std::unique_ptr<IdleProvider> provider)
    : provider_(std::move(provider)),
      idle_threshold_(idle_threshold) {
    if (idle_threshold_ <= std::chrono::milliseconds::zero()) {
        throw std::invalid_argument("Idle threshold must be positive");
    }
}

IdleDetector::~IdleDetector() {
    stop();
}

void IdleDetector::start() {
    std::lock_guard lock(mutex_);
    if (running_) {
        return;
    }
    running_ = true;
    last_sample_ = std::chrono::steady_clock::now();
    worker_ = std::jthread([this](std::stop_token token) { run(token); });
}

void IdleDetector::stop() {
    {
        std::lock_guard lock(mutex_);
        if (!running_) {
            return;
        }
        running_ = false;
    }
    worker_.request_stop();
    condition_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void IdleDetector::notifyActivity() {
    manual_provider_.markActivity();
    std::lock_guard lock(mutex_);
    idle_duration_ = std::chrono::milliseconds(0);
    idle_ = false;
    last_sample_ = std::chrono::steady_clock::now();
    condition_.notify_all();
}

IdleSnapshot IdleDetector::snapshot() const {
    std::lock_guard lock(mutex_);
    return IdleSnapshot {
        .idle_duration = idle_duration_,
        .active_screen_time = std::chrono::duration_cast<std::chrono::minutes>(active_seconds_),
        .idle = idle_,
    };
}

bool IdleDetector::isIdle() const {
    std::lock_guard lock(mutex_);
    return idle_;
}

std::chrono::milliseconds IdleDetector::idleDuration() const {
    std::lock_guard lock(mutex_);
    return idle_duration_;
}

std::chrono::minutes IdleDetector::activeScreenTime() const {
    std::lock_guard lock(mutex_);
    return std::chrono::duration_cast<std::chrono::minutes>(active_seconds_);
}

void IdleDetector::run(std::stop_token stop_token) {
    std::unique_lock lock(mutex_);
    while (!stop_token.stop_requested()) {
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_sample_);
        idle_duration_ = queryIdleDurationLocked();
        idle_ = idle_duration_ >= idle_threshold_;
        if (!idle_) {
            active_seconds_ += elapsed;
        }
        last_sample_ = now;
        condition_.wait_for(lock, std::chrono::seconds(1));
    }
}

std::chrono::milliseconds IdleDetector::queryIdleDurationLocked() {
    if (provider_) {
        const auto idle = provider_->queryIdleDuration();
        if (idle.has_value()) {
            return idle.value();
        }
    }

    const auto idle = XPrintIdleProvider {}.queryIdleDuration();
    if (idle.has_value()) {
        return idle.value();
    }
    return manual_provider_.queryIdleDuration().value_or(std::chrono::milliseconds(0));
}

}  // namespace sandhika::idle
