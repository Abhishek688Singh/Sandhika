#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace health_reminder::config {

enum class ReminderScheduleType {
    Interval,
    Daily,
    Weekly,
    Monthly
};

enum class Weekday {
    Monday,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday,
    Sunday
};

struct TimeOfDay {
    int hour {0};
    int minute {0};
};

struct EyeBreakConfig {
    bool enabled {true};
    std::chrono::seconds interval {std::chrono::minutes(20)};
    std::chrono::seconds duration {std::chrono::seconds(20)};
    bool strict_mode {false};
    std::chrono::seconds skip_unlock {std::chrono::seconds(60)};
    bool sound {true};
};

struct WaterConfig {
    bool enabled {true};
    std::chrono::seconds interval {std::chrono::hours(1)};
    bool sound {true};
};

struct QuietHoursConfig {
    bool enabled {false};
    TimeOfDay start {23, 0};
    TimeOfDay end {7, 0};
};

struct WeekendReminderOverride {
    std::optional<bool> enabled;
};

struct WeekendConfig {
    WeekendReminderOverride eye_break;
    WeekendReminderOverride water;
};

struct BatteryConfig {
    int low_threshold {15};
};

struct FullscreenConfig {
    std::vector<std::string> suppress_apps {
        "chrome", "chromium", "firefox", "vlc", "mpv", "steam"
    };
};

struct CustomReminderConfig {
    ReminderScheduleType schedule_type {ReminderScheduleType::Interval};
    std::string name;
    std::string message;
    bool sound {false};
    std::optional<std::string> icon;
    std::optional<std::string> command;

    std::optional<std::chrono::seconds> interval;
    std::optional<TimeOfDay> at;
    std::optional<Weekday> weekday;
    std::optional<int> day;
};

}  // namespace health_reminder::config
