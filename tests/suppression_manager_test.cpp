#include "sandhika/suppression/suppression_manager.h"
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace sandhika;

void assert_true(bool condition, const std::string& msg) {
    if (!condition) throw std::runtime_error(msg);
}

void test_suppression_manager() {
    auto config = std::make_shared<config::ConfigManager>(config::ConfigManager::defaultConfigPath());
    suppression::SuppressionManager manager(config.get());
    
    manager.setMediaMode(false);
    assert_true(!manager.isMediaModeEnabled(), "Media mode should be disabled initially");
    assert_true(!manager.shouldSuppress("eye_break"), "Should not suppress eye_break");
    assert_true(!manager.shouldSuppress("water"), "Should not suppress water");

    manager.setMediaMode(true);
    assert_true(manager.isMediaModeEnabled(), "Media mode should be enabled");
    assert_true(manager.shouldSuppress("eye_break"), "Should suppress eye_break");
    assert_true(manager.shouldSuppress("strict_break"), "Should suppress strict_break");
    assert_true(!manager.shouldSuppress("water"), "Should not suppress water");

    manager.toggleMediaMode();
    assert_true(!manager.isMediaModeEnabled(), "Media mode should be disabled after toggle");
}

int main() {
    try {
        test_suppression_manager();
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << "\n";
        return 1;
    }
    std::cout << "All suppression tests passed.\n";
    return 0;
}
