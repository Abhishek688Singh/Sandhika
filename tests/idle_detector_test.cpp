#include "health_reminder/idle/idle_detector.h"
#include <cassert>
#include <iostream>

using namespace health_reminder::idle;

class MockIdleProvider : public IdleProvider {
public:
    std::chrono::milliseconds current_idle {0};
    
    std::optional<std::chrono::milliseconds> queryIdleDuration() override {
        return current_idle;
    }
};

int main() {
    auto provider = std::make_unique<MockIdleProvider>();
    auto* p = provider.get();
    
    IdleDetector detector(std::chrono::milliseconds(500), std::move(provider));
    
    assert(!detector.isIdle());
    assert(detector.snapshot().active_screen_time == std::chrono::minutes(0));

    p->current_idle = std::chrono::milliseconds(1000);
    detector.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    
    assert(detector.isIdle());
    detector.notifyActivity();
    assert(!detector.isIdle());

    detector.stop();

    std::cout << "IdleDetector tests passed\n";
    return 0;
}
