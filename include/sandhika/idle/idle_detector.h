#pragma once

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

namespace sandhika::idle {

struct IdleSnapshot {
    std::chrono::milliseconds idle_duration {0};
    std::chrono::minutes active_screen_time {0};
    bool idle {false};
};

class IdleProvider {
public:
    virtual ~IdleProvider() = default;
    [[nodiscard]] virtual std::optional<std::chrono::milliseconds> queryIdleDuration() = 0;
};

class XPrintIdleProvider final : public IdleProvider {
public:
    [[nodiscard]] std::optional<std::chrono::milliseconds> queryIdleDuration() override;
};

class ManualIdleProvider final : public IdleProvider {
public:
    void markActivity();
    [[nodiscard]] std::optional<std::chrono::milliseconds> queryIdleDuration() override;

private:
    std::chrono::steady_clock::time_point last_activity_ {std::chrono::steady_clock::now()};
    std::mutex mutex_;
};

class IdleDetector {
public:
    explicit IdleDetector(
        std::chrono::milliseconds idle_threshold = std::chrono::minutes(5),
        std::unique_ptr<IdleProvider> provider = nullptr);
    ~IdleDetector();

    IdleDetector(const IdleDetector&) = delete;
    IdleDetector& operator=(const IdleDetector&) = delete;

    void start();
    void stop();
    void notifyActivity();

    [[nodiscard]] IdleSnapshot snapshot() const;
    [[nodiscard]] bool isIdle() const;
    [[nodiscard]] std::chrono::milliseconds idleDuration() const;
    [[nodiscard]] std::chrono::minutes activeScreenTime() const;

private:
    void run(std::stop_token stop_token);
    [[nodiscard]] std::chrono::milliseconds queryIdleDurationLocked();

    mutable std::mutex mutex_;
    std::condition_variable_any condition_;
    std::unique_ptr<IdleProvider> provider_;
    ManualIdleProvider manual_provider_;
    std::jthread worker_;
    std::chrono::milliseconds idle_threshold_;
    std::chrono::milliseconds idle_duration_ {0};
    std::chrono::steady_clock::time_point last_sample_ {std::chrono::steady_clock::now()};
    std::chrono::seconds active_seconds_ {0};
    bool idle_ {false};
    bool running_ {false};
};

}  // namespace sandhika::idle
