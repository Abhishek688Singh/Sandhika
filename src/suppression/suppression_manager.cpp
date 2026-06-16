#include "sandhika/suppression/suppression_manager.h"

namespace sandhika::suppression {

SuppressionManager::SuppressionManager(config::ConfigManager* config)
    : config_(config) {}

bool SuppressionManager::isMediaModeEnabled() const {
    return config_->getMediaModeConfig().enabled;
}

void SuppressionManager::setMediaMode(bool enabled) {
    config_->setMediaModeEnabled(enabled);
}

void SuppressionManager::toggleMediaMode() {
    setMediaMode(!isMediaModeEnabled());
}

bool SuppressionManager::shouldSuppress(const std::string& reminder_id) const {
    if (!isMediaModeEnabled()) {
        return false;
    }
    if (reminder_id == "eye_break" || reminder_id == "strict_break") {
        return true;
    }
    return false;
}

}  // namespace sandhika::suppression
