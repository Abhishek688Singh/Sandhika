#include "sandhika/config/config_manager.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <fstream>

namespace sandhika::config {
namespace {

std::string trim(std::string value) {
    const auto is_space = [](unsigned char ch) { return std::isspace(ch) != 0; };
    const auto first = std::find_if_not(value.begin(), value.end(), is_space);
    if (first == value.end()) {
        return {};
    }

    const auto last = std::find_if_not(value.rbegin(), value.rend(), is_space).base();
    return std::string(first, last);
}

std::string to_lower(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

[[nodiscard]] std::string parse_scalar_string(const YAML::Node& node, std::string_view field) {
    if (!node || !node.IsScalar()) {
        throw ConfigError("Expected string for field: " + std::string(field));
    }
    try {
        return node.as<std::string>();
    } catch (const YAML::Exception& ex) {
        throw ConfigError("Invalid string for field '" + std::string(field) + "': " + ex.what());
    }
}

template <typename T>
[[nodiscard]] T parse_scalar_as(const YAML::Node& node, std::string_view field) {
    if (!node || !node.IsScalar()) {
        throw ConfigError("Expected scalar for field: " + std::string(field));
    }
    try {
        return node.as<T>();
    } catch (const YAML::Exception& ex) {
        throw ConfigError("Invalid value for field '" + std::string(field) + "': " + ex.what());
    }
}

[[nodiscard]] bool is_digit(char ch) {
    return std::isdigit(static_cast<unsigned char>(ch)) != 0;
}

[[nodiscard]] std::chrono::seconds parse_duration(const YAML::Node& node, std::string_view field) {
    const std::string raw = trim(parse_scalar_string(node, field));
    if (raw.size() < 2) {
        throw ConfigError("Invalid duration for field: " + std::string(field));
    }

    const char unit = static_cast<char>(std::tolower(static_cast<unsigned char>(raw.back())));
    const std::string value_part = raw.substr(0, raw.size() - 1);
    std::size_t parsed_chars = 0;
    int value = 0;
    try {
        value = std::stoi(value_part, &parsed_chars);
    } catch (const std::exception&) {
        throw ConfigError("Invalid duration number for field: " + std::string(field));
    }

    if (parsed_chars != value_part.size() || value <= 0) {
        throw ConfigError("Duration must be a positive integer for field: " + std::string(field));
    }

    switch (unit) {
        case 's':
            return std::chrono::seconds(value);
        case 'm':
            return std::chrono::minutes(value);
        case 'h':
            return std::chrono::hours(value);
        default:
            throw ConfigError("Unsupported duration unit for field: " + std::string(field));
    }
}

[[nodiscard]] TimeOfDay parse_time_of_day(const YAML::Node& node, std::string_view field) {
    const std::string text = trim(parse_scalar_string(node, field));
    if (text.size() != 5 || text[2] != ':' || !is_digit(text[0]) || !is_digit(text[1]) ||
        !is_digit(text[3]) || !is_digit(text[4])) {
        throw ConfigError("Invalid time format for field: " + std::string(field));
    }

    const int hour = ((text[0] - '0') * 10) + (text[1] - '0');
    const int minute = ((text[3] - '0') * 10) + (text[4] - '0');

    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        throw ConfigError("Time out of range for field: " + std::string(field));
    }

    return TimeOfDay {hour, minute};
}

[[nodiscard]] Weekday parse_weekday(const YAML::Node& node, std::string_view field) {
    static const std::unordered_map<std::string, Weekday> map = {
        {"monday", Weekday::Monday},
        {"tuesday", Weekday::Tuesday},
        {"wednesday", Weekday::Wednesday},
        {"thursday", Weekday::Thursday},
        {"friday", Weekday::Friday},
        {"saturday", Weekday::Saturday},
        {"sunday", Weekday::Sunday},
    };

    const std::string value = to_lower(trim(parse_scalar_string(node, field)));
    const auto it = map.find(value);
    if (it == map.end()) {
        throw ConfigError("Unsupported weekday for field: " + std::string(field));
    }
    return it->second;
}

[[nodiscard]] EyeBreakConfig parse_eye_break(const YAML::Node& root) {
    EyeBreakConfig config;
    const YAML::Node node = root["eye_break"];
    if (!node) {
        return config;
    }
    if (node["enabled"]) {
        config.enabled = parse_scalar_as<bool>(node["enabled"], "eye_break.enabled");
    }
    if (node["interval"]) {
        config.interval = parse_duration(node["interval"], "eye_break.interval");
    }
    if (node["duration"]) {
        config.duration = parse_duration(node["duration"], "eye_break.duration");
    }
    if (node["strict_mode"]) {
        config.strict_mode = parse_scalar_as<bool>(node["strict_mode"], "eye_break.strict_mode");
    }
    if (node["skip_unlock"]) {
        config.skip_unlock = parse_duration(node["skip_unlock"], "eye_break.skip_unlock");
    }
    if (node["sound"]) {
        config.sound = parse_scalar_as<bool>(node["sound"], "eye_break.sound");
    }
    return config;
}

[[nodiscard]] WaterConfig parse_water(const YAML::Node& root) {
    WaterConfig config;
    const YAML::Node node = root["water"];
    if (!node) {
        return config;
    }
    if (node["enabled"]) {
        config.enabled = parse_scalar_as<bool>(node["enabled"], "water.enabled");
    }
    if (node["interval"]) {
        config.interval = parse_duration(node["interval"], "water.interval");
    }
    if (node["sound"]) {
        config.sound = parse_scalar_as<bool>(node["sound"], "water.sound");
    }
    return config;
}

[[nodiscard]] QuietHoursConfig parse_quiet_hours(const YAML::Node& root) {
    QuietHoursConfig config;
    const YAML::Node node = root["quiet_hours"];
    if (!node) {
        return config;
    }
    if (node["enabled"]) {
        config.enabled = parse_scalar_as<bool>(node["enabled"], "quiet_hours.enabled");
    }
    if (node["start"]) {
        config.start = parse_time_of_day(node["start"], "quiet_hours.start");
    }
    if (node["end"]) {
        config.end = parse_time_of_day(node["end"], "quiet_hours.end");
    }
    return config;
}

[[nodiscard]] WeekendConfig parse_weekend(const YAML::Node& root) {
    WeekendConfig config;
    const YAML::Node weekend = root["weekend"];
    if (!weekend) {
        return config;
    }

    const YAML::Node eye_break = weekend["eye_break"];
    if (eye_break && eye_break["enabled"]) {
        config.eye_break.enabled = parse_scalar_as<bool>(eye_break["enabled"], "weekend.eye_break.enabled");
    }

    const YAML::Node water = weekend["water"];
    if (water && water["enabled"]) {
        config.water.enabled = parse_scalar_as<bool>(water["enabled"], "weekend.water.enabled");
    }
    return config;
}

[[nodiscard]] BatteryConfig parse_battery(const YAML::Node& root) {
    BatteryConfig config;
    const YAML::Node battery = root["battery"];
    if (!battery) {
        return config;
    }
    if (battery["low_threshold"]) {
        const int threshold = parse_scalar_as<int>(battery["low_threshold"], "battery.low_threshold");
        if (threshold < 1 || threshold > 100) {
            throw ConfigError("battery.low_threshold must be in range [1, 100]");
        }
        config.low_threshold = threshold;
    }
    return config;
}

[[nodiscard]] MediaModeConfig parse_media_mode(const YAML::Node& root) {
    MediaModeConfig config;
    const YAML::Node mm = root["media_mode"];
    if (mm && mm["enabled"]) {
        config.enabled = parse_scalar_as<bool>(mm["enabled"], "media_mode.enabled");
    }
    return config;
}

[[nodiscard]] CustomReminderConfig parse_custom_reminder(const YAML::Node& node, std::size_t index) {
    if (!node || !node.IsMap()) {
        throw ConfigError("custom[" + std::to_string(index) + "] must be a map");
    }

    CustomReminderConfig reminder;
    reminder.name = parse_scalar_string(node["name"], "custom.name");
    reminder.message = parse_scalar_string(node["message"], "custom.message");
    if (node["sound"]) {
        reminder.sound = parse_scalar_as<bool>(node["sound"], "custom.sound");
    }
    if (node["icon"]) {
        reminder.icon = parse_scalar_string(node["icon"], "custom.icon");
    }
    if (node["command"]) {
        reminder.command = parse_scalar_string(node["command"], "custom.command");
    }

    const bool has_interval = static_cast<bool>(node["interval"]);
    const bool has_at = static_cast<bool>(node["at"]);
    const bool has_weekday = static_cast<bool>(node["weekday"]);
    const bool has_day = static_cast<bool>(node["day"]);

    if (has_interval) {
        if (has_at || has_weekday || has_day) {
            throw ConfigError("custom[" + std::to_string(index) + "] interval schedule cannot mix with at/weekday/day");
        }
        reminder.schedule_type = ReminderScheduleType::Interval;
        reminder.interval = parse_duration(node["interval"], "custom.interval");
        return reminder;
    }

    if (!has_at) {
        throw ConfigError("custom[" + std::to_string(index) + "] requires either interval or at");
    }

    reminder.at = parse_time_of_day(node["at"], "custom.at");
    if (has_weekday && has_day) {
        throw ConfigError("custom[" + std::to_string(index) + "] cannot define both weekday and day");
    }

    if (has_weekday) {
        reminder.schedule_type = ReminderScheduleType::Weekly;
        reminder.weekday = parse_weekday(node["weekday"], "custom.weekday");
        return reminder;
    }

    if (has_day) {
        reminder.schedule_type = ReminderScheduleType::Monthly;
        const int day = parse_scalar_as<int>(node["day"], "custom.day");
        if (day < 1 || day > 31) {
            throw ConfigError("custom[" + std::to_string(index) + "] day must be in range [1, 31]");
        }
        reminder.day = day;
        return reminder;
    }

    reminder.schedule_type = ReminderScheduleType::Daily;
    return reminder;
}

[[nodiscard]] std::vector<CustomReminderConfig> parse_custom_reminders(const YAML::Node& root) {
    std::vector<CustomReminderConfig> reminders;
    const YAML::Node custom = root["custom"];
    if (!custom) {
        return reminders;
    }
    if (!custom.IsSequence()) {
        throw ConfigError("custom must be a YAML sequence");
    }

    reminders.reserve(custom.size());
    for (std::size_t i = 0; i < custom.size(); ++i) {
        reminders.push_back(parse_custom_reminder(custom[i], i));
    }
    return reminders;
}

}  // namespace

ConfigManager::ConfigManager(std::filesystem::path config_path)
    : config_path_(std::move(config_path)) {
    reload();
}

std::filesystem::path ConfigManager::defaultConfigPath() {
    const char* home = std::getenv("HOME");
    if (home == nullptr || std::string_view(home).empty()) {
        throw ConfigError("HOME environment variable is not set");
    }

    return std::filesystem::path(home) / ".config" / "sandhika" / "config.yaml";
}

void ConfigManager::reload() {
    std::filesystem::path absolute_path;
    try {
        absolute_path = std::filesystem::absolute(config_path_);
        if (!std::filesystem::exists(absolute_path)) {
            std::error_code ec;
            std::filesystem::create_directories(absolute_path.parent_path(), ec);
            
            bool copied = false;
            if (std::filesystem::exists("config/config.example.yaml", ec)) {
                copied = std::filesystem::copy_file("config/config.example.yaml", absolute_path, std::filesystem::copy_options::skip_existing, ec);
            }
            
            if (!copied) {
                std::ofstream out(absolute_path);
                if (out) {
                    out << "eye_break:\n  enabled: true\n  interval: 20m\n  duration: 20s\n  strict_mode: false\n  skip_unlock: 60s\n  sound: true\n"
                        << "\nwater:\n  enabled: true\n  interval: 1h\n  sound: true\n"
                        << "\nquiet_hours:\n  enabled: false\n  start: \"23:00\"\n  end: \"07:00\"\n";
                }
            }
            
            if (!std::filesystem::exists(absolute_path, ec)) {
                std::unique_lock lock(mutex_);
                eye_break_config_ = EyeBreakConfig();
                water_config_ = WaterConfig();
                custom_reminders_.clear();
                quiet_hours_ = QuietHoursConfig();
                weekend_config_ = WeekendConfig();
                battery_config_ = BatteryConfig();
                return;
            }
        }
    } catch (const std::exception& ex) {
        // If anything fails during the fallback process, just load defaults
        std::unique_lock lock(mutex_);
        eye_break_config_ = EyeBreakConfig();
        water_config_ = WaterConfig();
        custom_reminders_.clear();
        quiet_hours_ = QuietHoursConfig();
        weekend_config_ = WeekendConfig();
        battery_config_ = BatteryConfig();
        return;
    }

    YAML::Node root;
    try {
        root = YAML::LoadFile(absolute_path.string());
    } catch (const YAML::Exception& ex) {
        throw ConfigError("Failed to parse config file '" + absolute_path.string() + "': " + ex.what());
    }

    EyeBreakConfig eye_break;
    WaterConfig water;
    std::vector<CustomReminderConfig> custom;
    QuietHoursConfig quiet_hours;
    WeekendConfig weekend;
    BatteryConfig battery;
    MediaModeConfig media_mode;
    try {
        eye_break = parse_eye_break(root);
        water = parse_water(root);
        custom = parse_custom_reminders(root);
        quiet_hours = parse_quiet_hours(root);
        weekend = parse_weekend(root);
        battery = parse_battery(root);
        media_mode = parse_media_mode(root);
    } catch (const YAML::Exception& ex) {
        throw ConfigError("Invalid YAML value in config file '" + absolute_path.string() + "': " + ex.what());
    }

    std::unique_lock lock(mutex_);
    eye_break_config_ = eye_break;
    water_config_ = water;
    custom_reminders_ = std::move(custom);
    quiet_hours_ = quiet_hours;
    weekend_config_ = weekend;
    battery_config_ = battery;
    media_mode_config_ = media_mode;
}

EyeBreakConfig ConfigManager::getEyeBreakConfig() const {
    std::shared_lock lock(mutex_);
    return eye_break_config_;
}

WaterConfig ConfigManager::getWaterConfig() const {
    std::shared_lock lock(mutex_);
    return water_config_;
}

std::vector<CustomReminderConfig> ConfigManager::getCustomReminders() const {
    std::shared_lock lock(mutex_);
    return custom_reminders_;
}

QuietHoursConfig ConfigManager::getQuietHours() const {
    std::shared_lock lock(mutex_);
    return quiet_hours_;
}

WeekendConfig ConfigManager::getWeekendConfig() const {
    std::shared_lock lock(mutex_);
    return weekend_config_;
}

BatteryConfig ConfigManager::getBatteryConfig() const {
    std::shared_lock lock(mutex_);
    return battery_config_;
}

MediaModeConfig ConfigManager::getMediaModeConfig() const {
    std::shared_lock lock(mutex_);
    return media_mode_config_;
}

void ConfigManager::setMediaModeEnabled(bool enabled) {
    {
        std::unique_lock lock(mutex_);
        media_mode_config_.enabled = enabled;
    }
    try {
        std::filesystem::path absolute_path = std::filesystem::absolute(config_path_);
        YAML::Node root;
        if (std::filesystem::exists(absolute_path)) {
            root = YAML::LoadFile(absolute_path.string());
        }
        if (!root["media_mode"]) {
            root["media_mode"] = YAML::Node(YAML::NodeType::Map);
        }
        root["media_mode"]["enabled"] = enabled;
        std::ofstream out(absolute_path);
        if (out) {
            out << root;
        }
    } catch (...) {}
}

}  // namespace sandhika::config
