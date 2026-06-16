#pragma once

#include "health_reminder/brightness/brightness_controller.h"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

namespace health_reminder::strict {

struct StrictBreakState {
    bool active {false};
    bool skip_enabled {false};
    std::chrono::seconds remaining {0};
};

class StrictBreakWindow {
public:
    using FinishedCallback = std::function<void()>;
    using SkippedCallback = std::function<void()>;

    explicit StrictBreakWindow(std::shared_ptr<brightness::BrightnessController> brightness_controller);
    ~StrictBreakWindow();

    StrictBreakWindow(const StrictBreakWindow&) = delete;
    StrictBreakWindow& operator=(const StrictBreakWindow&) = delete;

    void setFinishedCallback(FinishedCallback callback);
    void setSkippedCallback(SkippedCallback callback);
    void showBreak(std::chrono::seconds duration, std::chrono::seconds skip_unlock, int dim_percent = 20);
    [[nodiscard]] bool requestSkip();
    void close();

    [[nodiscard]] StrictBreakState state() const;

private:
    void run(std::stop_token stop_token, std::chrono::seconds duration, std::chrono::seconds skip_unlock);
    void finish(bool skipped);

    std::shared_ptr<brightness::BrightnessController> brightness_controller_;
    mutable std::mutex mutex_;
    std::condition_variable_any condition_;
    std::jthread worker_;
    FinishedCallback finished_callback_;
    SkippedCallback skipped_callback_;
    StrictBreakState state_;
};

}  // namespace health_reminder::strict
