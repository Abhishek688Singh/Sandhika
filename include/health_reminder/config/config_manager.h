#pragma once

#include "health_reminder/config/config_types.h"

#include <filesystem>
#include <shared_mutex>
#include <stdexcept>
#include <vector>

namespace health_reminder::config {

class ConfigError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/**
 * Thread-safe YAML configuration loader and validator.
 *
 * Public API:
 * 1. Construct with explicit path or use defaultConfigPath().
 * 2. Call reload() to re-read the file after external changes.
 * 3. Read strongly-typed sections through getter methods.
 */
class ConfigManager {
public:
    explicit ConfigManager(std::filesystem::path config_path);

    static std::filesystem::path defaultConfigPath();

    void reload();

    [[nodiscard]] EyeBreakConfig getEyeBreakConfig() const;
    [[nodiscard]] WaterConfig getWaterConfig() const;
    [[nodiscard]] std::vector<CustomReminderConfig> getCustomReminders() const;
    [[nodiscard]] QuietHoursConfig getQuietHours() const;
    [[nodiscard]] WeekendConfig getWeekendConfig() const;
    [[nodiscard]] BatteryConfig getBatteryConfig() const;
    [[nodiscard]] FullscreenConfig getFullscreenConfig() const;

private:
    std::filesystem::path config_path_;

    EyeBreakConfig eye_break_config_;
    WaterConfig water_config_;
    std::vector<CustomReminderConfig> custom_reminders_;
    QuietHoursConfig quiet_hours_;
    WeekendConfig weekend_config_;
    BatteryConfig battery_config_;
    FullscreenConfig fullscreen_config_;

    mutable std::shared_mutex mutex_;
};

}  // namespace health_reminder::config
