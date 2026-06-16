#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace health_reminder::fullscreen {

struct FullscreenSnapshot {
    bool fullscreen {false};
    std::string window_class;
};

class FullscreenProvider {
public:
    virtual ~FullscreenProvider() = default;
    [[nodiscard]] virtual std::optional<FullscreenSnapshot> query() = 0;
};

class XPropFullscreenProvider final : public FullscreenProvider {
public:
    [[nodiscard]] std::optional<FullscreenSnapshot> query() override;
};

class FullscreenDetector {
public:
    using Callback = std::function<void(FullscreenSnapshot)>;

    explicit FullscreenDetector(std::unique_ptr<FullscreenProvider> provider = nullptr);
    ~FullscreenDetector();

    FullscreenDetector(const FullscreenDetector&) = delete;
    FullscreenDetector& operator=(const FullscreenDetector&) = delete;

    void start();
    void stop();
    void setCallback(Callback callback);

    [[nodiscard]] FullscreenSnapshot snapshot() const;
    [[nodiscard]] bool isFullscreen() const;

private:
    void run(std::stop_token stop_token);
    [[nodiscard]] FullscreenSnapshot queryProvider();

    mutable std::mutex mutex_;
    std::condition_variable_any condition_;
    std::unique_ptr<FullscreenProvider> provider_;
    Callback callback_;
    std::jthread worker_;
    FullscreenSnapshot snapshot_;
    bool running_ {false};
};

}  // namespace health_reminder::fullscreen
