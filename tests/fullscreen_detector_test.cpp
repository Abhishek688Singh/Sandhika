#include "health_reminder/fullscreen/fullscreen_detector.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <atomic>
#include <chrono>

using namespace health_reminder::fullscreen;

class MockProvider : public FullscreenProvider {
public:
    std::optional<FullscreenSnapshot> query() override {
        FullscreenSnapshot snap;
        snap.fullscreen = fullscreen_;
        snap.window_class = class_;
        return snap;
    }
    
    bool fullscreen_ = false;
    std::string class_ = "";
};

void assert_true(bool condition, const std::string& msg) {
    if (!condition) throw std::runtime_error(msg);
}

void test_fullscreen_detector() {
    auto provider = std::make_unique<MockProvider>();
    auto* raw_provider = provider.get();
    
    FullscreenDetector detector(std::move(provider));
    detector.start();
    
    std::atomic<int> callback_count{0};
    detector.setCallback([&](const FullscreenSnapshot& snap) {
        callback_count++;
    });
    
    raw_provider->fullscreen_ = true;
    raw_provider->class_ = "firefox";
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    
    assert_true(callback_count > 0, "Callback should be fired");
    assert_true(detector.isFullscreen(), "Should be fullscreen");
    assert_true(detector.snapshot().window_class == "firefox", "Class should be firefox");
    
    detector.stop();
}

int main() {
    try {
        test_fullscreen_detector();
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << "\n";
        return 1;
    }
    std::cout << "All fullscreen tests passed.\n";
    return 0;
}
