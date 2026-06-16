#include "health_reminder/fullscreen/fullscreen_detector.h"

#include <array>
#include <cstdio>
#include <string>

namespace health_reminder::fullscreen {
namespace {

[[nodiscard]] std::optional<std::string> run_command(const char* command) {
    std::array<char, 256> buffer {};
    std::string output;
    FILE* pipe = popen(command, "r");
    if (pipe == nullptr) {
        return std::nullopt;
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }
    const int status = pclose(pipe);
    if (status != 0) {
        return std::nullopt;
    }
    return output;
}

[[nodiscard]] std::optional<std::string> active_window_id() {
    const auto output = run_command("xprop -root _NET_ACTIVE_WINDOW 2>/dev/null");
    if (!output.has_value()) {
        return std::nullopt;
    }
    const auto marker = output->find("window id #");
    if (marker == std::string::npos) {
        return std::nullopt;
    }
    auto id = output->substr(marker + 11);
    while (!id.empty() && (id.back() == '\n' || id.back() == ' ')) {
        id.pop_back();
    }
    if (id == "0x0" || id.empty()) {
        return std::nullopt;
    }
    return id;
}

[[nodiscard]] std::string shell_quote(const std::string& value) {
    std::string quoted = "'";
    for (char ch : value) {
        if (ch == '\'') {
            quoted += "'\\''";
        } else {
            quoted += ch;
        }
    }
    quoted += "'";
    return quoted;
}

}  // namespace

std::optional<FullscreenSnapshot> XPropFullscreenProvider::query() {
    const auto id = active_window_id();
    if (!id.has_value()) {
        return std::nullopt;
    }

    const std::string command = "xprop -id " + shell_quote(id.value()) +
                                " _NET_WM_STATE WM_CLASS 2>/dev/null";
    const auto output = run_command(command.c_str());
    if (!output.has_value()) {
        return std::nullopt;
    }

    FullscreenSnapshot snapshot;
    snapshot.fullscreen = output->find("_NET_WM_STATE_FULLSCREEN") != std::string::npos;
    const auto class_pos = output->find("WM_CLASS");
    if (class_pos != std::string::npos) {
        snapshot.window_class = output->substr(class_pos);
        while (!snapshot.window_class.empty() && snapshot.window_class.back() == '\n') {
            snapshot.window_class.pop_back();
        }
    }
    return snapshot;
}

FullscreenDetector::FullscreenDetector(std::unique_ptr<FullscreenProvider> provider)
    : provider_(std::move(provider)) {}

FullscreenDetector::~FullscreenDetector() {
    stop();
}

void FullscreenDetector::start() {
    std::lock_guard lock(mutex_);
    if (running_) {
        return;
    }
    running_ = true;
    worker_ = std::jthread([this](std::stop_token token) { run(token); });
}

void FullscreenDetector::stop() {
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

void FullscreenDetector::setCallback(Callback callback) {
    std::lock_guard lock(mutex_);
    callback_ = std::move(callback);
}

FullscreenSnapshot FullscreenDetector::snapshot() const {
    std::lock_guard lock(mutex_);
    return snapshot_;
}

bool FullscreenDetector::isFullscreen() const {
    std::lock_guard lock(mutex_);
    return snapshot_.fullscreen;
}

void FullscreenDetector::run(std::stop_token stop_token) {
    std::unique_lock lock(mutex_);
    while (!stop_token.stop_requested()) {
        FullscreenSnapshot next = queryProvider();
        Callback callback;
        bool changed = false;
        if (next.fullscreen != snapshot_.fullscreen || next.window_class != snapshot_.window_class) {
            snapshot_ = next;
            callback = callback_;
            changed = true;
        }

        lock.unlock();
        if (changed && callback) {
            callback(next);
        }
        lock.lock();
        condition_.wait_for(lock, std::chrono::seconds(1));
    }
}

FullscreenSnapshot FullscreenDetector::queryProvider() {
    if (provider_) {
        const auto snapshot = provider_->query();
        if (snapshot.has_value()) {
            return snapshot.value();
        }
    }

    auto snapshot = XPropFullscreenProvider {}.query();
    return snapshot.value_or(FullscreenSnapshot {});
}

}  // namespace health_reminder::fullscreen
