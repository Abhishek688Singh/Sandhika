#include "sandhika/config/config_manager.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using sandhika::config::ConfigError;
using sandhika::config::ConfigManager;
using sandhika::config::ReminderScheduleType;

namespace {

class TempFile {
public:
    explicit TempFile(std::string content)
        : path_(make_path()) {
        write(content);
    }

    ~TempFile() {
        std::error_code ec;
        std::filesystem::remove(path_, ec);
    }

    [[nodiscard]] const std::filesystem::path& path() const { return path_; }

    void write(const std::string& content) const {
        std::ofstream out(path_, std::ios::trunc);
        if (!out) {
            throw std::runtime_error("Failed to create temp file: " + path_.string());
        }
        out << content;
        if (!out.good()) {
            throw std::runtime_error("Failed to write temp file: " + path_.string());
        }
    }

    void writeAtomically(const std::string& content) const {
        const auto temp_path = path_.string() + ".tmp";
        {
            std::ofstream out(temp_path, std::ios::trunc);
            if (!out) {
                throw std::runtime_error("Failed to create temp file: " + temp_path);
            }
            out << content;
            if (!out.good()) {
                throw std::runtime_error("Failed to write temp file: " + temp_path);
            }
        }

        std::error_code ec;
        std::filesystem::rename(temp_path, path_, ec);
        if (ec) {
            std::filesystem::remove(temp_path, ec);
            throw std::runtime_error("Failed to replace temp file: " + path_.string());
        }
    }

private:
    static std::filesystem::path make_path() {
        static std::atomic<unsigned int> counter {0};
        const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        return std::filesystem::temp_directory_path() /
               ("sandhika-config-test-" + std::to_string(now) + "-" +
                std::to_string(counter.fetch_add(1, std::memory_order_relaxed)) + ".yaml");
    }

    std::filesystem::path path_;
};

void assert_true(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename Callable>
void expect_config_error(Callable&& callable, const std::string& message) {
    bool thrown = false;
    try {
        callable();
    } catch (const ConfigError&) {
        thrown = true;
    }
    assert_true(thrown, message);
}

void test_parses_valid_config() {
    const std::string yaml = R"YAML(
eye_break:
  enabled: true
  interval: 20m
  duration: 20s
  strict_mode: false
  skip_unlock: 60s
  sound: true

water:
  enabled: true
  interval: 1h
  sound: true

quiet_hours:
  enabled: true
  start: "23:00"
  end: "07:00"

custom:
  - name: "Check Posture"
    interval: 30m
    message: "Sit straight"
    sound: true
    icon: posture
    command: "playerctl pause"
  - name: "Lunch"
    at: "13:00"
    message: "Take lunch"
  - name: "Weekly Review"
    weekday: Sunday
    at: "18:00"
    message: "Review goals"
  - name: "Pay Electricity Bill"
    day: 1
    at: "10:00"
    message: "Pay bill"

weekend:
  eye_break:
    enabled: false
  water:
    enabled: true

battery:
  low_threshold: 15

media_mode:
  enabled: true
)YAML";

    TempFile file(yaml);
    ConfigManager manager(file.path());

    const auto eye = manager.getEyeBreakConfig();
    assert_true(eye.enabled, "eye_break.enabled mismatch");
    assert_true(eye.interval == std::chrono::minutes(20), "eye_break.interval mismatch");
    assert_true(eye.duration == std::chrono::seconds(20), "eye_break.duration mismatch");
    assert_true(eye.skip_unlock == std::chrono::seconds(60), "eye_break.skip_unlock mismatch");

    const auto water = manager.getWaterConfig();
    assert_true(water.interval == std::chrono::hours(1), "water.interval mismatch");

    const auto quiet = manager.getQuietHours();
    assert_true(quiet.enabled, "quiet_hours.enabled mismatch");
    assert_true(quiet.start.hour == 23 && quiet.start.minute == 0, "quiet_hours.start mismatch");
    assert_true(quiet.end.hour == 7 && quiet.end.minute == 0, "quiet_hours.end mismatch");

    const auto custom = manager.getCustomReminders();
    assert_true(custom.size() == 4, "custom reminder count mismatch");
    assert_true(custom[0].schedule_type == ReminderScheduleType::Interval, "custom[0] type mismatch");
    assert_true(custom[1].schedule_type == ReminderScheduleType::Daily, "custom[1] type mismatch");
    assert_true(custom[2].schedule_type == ReminderScheduleType::Weekly, "custom[2] type mismatch");
    assert_true(custom[3].schedule_type == ReminderScheduleType::Monthly, "custom[3] type mismatch");
    assert_true(custom[0].command.has_value(), "custom[0].command expected");

    const auto weekend = manager.getWeekendConfig();
    assert_true(weekend.eye_break.enabled.has_value() && !weekend.eye_break.enabled.value(), "weekend.eye_break.enabled mismatch");
    assert_true(weekend.water.enabled.has_value() && weekend.water.enabled.value(), "weekend.water.enabled mismatch");

    const auto battery = manager.getBatteryConfig();
    assert_true(battery.low_threshold == 15, "battery.low_threshold mismatch");

    const auto media_mode = manager.getMediaModeConfig();
    assert_true(media_mode.enabled == true, "media_mode.enabled mismatch");
}

void test_rejects_invalid_duration() {
    const std::string yaml = R"YAML(
eye_break:
  interval: 20x
)YAML";

    TempFile file(yaml);
    expect_config_error([&file] { const ConfigManager manager(file.path()); },
                        "Expected invalid duration to throw ConfigError");
}

void test_rejects_invalid_custom_schedule() {
    const std::string yaml = R"YAML(
custom:
  - name: "Broken"
    message: "Broken schedule"
    interval: 30m
    at: "11:00"
)YAML";

    TempFile file(yaml);
    expect_config_error([&file] { const ConfigManager manager(file.path()); },
                        "Expected invalid custom schedule to throw ConfigError");
}

void test_rejects_invalid_battery_threshold() {
    const std::string yaml = R"YAML(
battery:
  low_threshold: 101
)YAML";

    TempFile file(yaml);
    expect_config_error([&file] { const ConfigManager manager(file.path()); },
                        "Expected invalid battery threshold to throw ConfigError");
}

void test_rejects_malformed_yaml() {
    TempFile file("eye_break: [\n");
    expect_config_error([&file] { const ConfigManager manager(file.path()); },
                        "Expected malformed YAML to throw ConfigError");
}

void test_handles_missing_file() {
    const auto missing_path = std::filesystem::temp_directory_path() / "sandhika-config-test-missing.yaml";
    std::error_code ec;
    std::filesystem::remove(missing_path, ec);
    ConfigManager manager(missing_path);
    const auto eye = manager.getEyeBreakConfig();
    assert_true(eye.enabled, "Expected default config to be loaded");
}

void test_rejects_invalid_booleans() {
    const std::string yaml = R"YAML(
eye_break:
  enabled: maybe
)YAML";

    TempFile file(yaml);
    expect_config_error([&file] { const ConfigManager manager(file.path()); },
                        "Expected invalid boolean to throw ConfigError");
}

void test_rejects_invalid_times() {
    const std::vector<std::string> invalid_times = {"1:2", "12:30:45", "abc", "24:00", "12:99"};
    for (const auto& time : invalid_times) {
        const std::string yaml = "quiet_hours:\n  enabled: true\n  start: \"" + time + "\"\n";
        TempFile file(yaml);
        expect_config_error([&file] { const ConfigManager manager(file.path()); },
                            "Expected invalid time to throw ConfigError: " + time);
    }
}

void test_rejects_invalid_weekday() {
    const std::string yaml = R"YAML(
custom:
  - name: "Broken"
    message: "Broken weekday"
    weekday: Funday
    at: "18:00"
)YAML";

    TempFile file(yaml);
    expect_config_error([&file] { const ConfigManager manager(file.path()); },
                        "Expected invalid weekday to throw ConfigError");
}

void test_rejects_invalid_monthly_day() {
    const std::string yaml = R"YAML(
custom:
  - name: "Broken"
    message: "Broken monthly day"
    day: 32
    at: "10:00"
)YAML";

    TempFile file(yaml);
    expect_config_error([&file] { const ConfigManager manager(file.path()); },
                        "Expected invalid monthly day to throw ConfigError");
}

void test_rejects_invalid_custom_reminder() {
    const std::string yaml = R"YAML(
custom:
  - message: "Missing name"
    interval: 30m
)YAML";

    TempFile file(yaml);
    expect_config_error([&file] { const ConfigManager manager(file.path()); },
                        "Expected invalid custom reminder to throw ConfigError");
}

void test_reload_after_failure_preserves_previous_config() {
    TempFile file("eye_break:\n  interval: 20m\n");
    ConfigManager manager(file.path());
    assert_true(manager.getEyeBreakConfig().interval == std::chrono::minutes(20),
                "Initial interval mismatch");

    file.write("eye_break:\n  interval: 20x\n");
    expect_config_error([&manager] { manager.reload(); }, "Expected failed reload to throw ConfigError");
    assert_true(manager.getEyeBreakConfig().interval == std::chrono::minutes(20),
                "Failed reload should preserve previous interval");

    file.write("eye_break:\n  interval: 10m\n");
    manager.reload();
    assert_true(manager.getEyeBreakConfig().interval == std::chrono::minutes(10),
                "Successful reload should update interval");
}

void test_concurrent_getters_and_reload() {
    TempFile file("eye_break:\n  interval: 20m\nwater:\n  interval: 1h\n");
    ConfigManager manager(file.path());
    std::atomic<bool> stop {false};
    std::vector<std::jthread> readers;

    for (int i = 0; i < 8; ++i) {
        readers.emplace_back([&manager, &stop] {
            while (!stop.load(std::memory_order_acquire)) {
                const auto eye = manager.getEyeBreakConfig();
                const auto water = manager.getWaterConfig();
                const auto custom = manager.getCustomReminders();
                (void)eye;
                (void)water;
                (void)custom;
            }
        });
    }

    for (int i = 0; i < 100; ++i) {
        const int minutes = (i % 2 == 0) ? 20 : 25;
        file.writeAtomically("eye_break:\n  interval: " + std::to_string(minutes) + "m\nwater:\n  interval: 1h\n");
        manager.reload();
    }

    stop.store(true, std::memory_order_release);
}

}  // namespace

int main() {
    try {
        test_parses_valid_config();
        test_rejects_invalid_duration();
        test_rejects_invalid_custom_schedule();
        test_rejects_invalid_battery_threshold();
        test_rejects_malformed_yaml();
        test_handles_missing_file();
        test_rejects_invalid_booleans();
        test_rejects_invalid_times();
        test_rejects_invalid_weekday();
        test_rejects_invalid_monthly_day();
        test_rejects_invalid_custom_reminder();
        test_reload_after_failure_preserves_previous_config();
        test_concurrent_getters_and_reload();
    } catch (const std::exception& ex) {
        std::cerr << "Test failure: " << ex.what() << '\n';
        return 1;
    }

    std::cout << "All ConfigManager tests passed\n";
    return 0;
}
