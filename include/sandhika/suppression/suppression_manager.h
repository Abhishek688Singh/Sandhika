#pragma once

#include "sandhika/config/config_manager.h"
#include <string>
#include <memory>

namespace sandhika::suppression {

class SuppressionManager {
public:
    explicit SuppressionManager(config::ConfigManager* config);

    bool isMediaModeEnabled() const;
    void setMediaMode(bool enabled);
    void toggleMediaMode();

    bool shouldSuppress(const std::string& reminder_id) const;

private:
    config::ConfigManager* config_;
};

}  // namespace sandhika::suppression
