#include "sandhika/stats/stats_manager.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace sandhika::stats {
namespace {

template <typename T>
[[nodiscard]] T yaml_as_or(const YAML::Node& node, T fallback, std::string_view field) {
    if (!node) {
        return fallback;
    }

    try {
        return node.as<T>();
    } catch (const YAML::Exception& ex) {
        throw StatsError("Invalid stats field '" + std::string(field) + "': " + ex.what());
    }
}

[[nodiscard]] bool is_digit(char ch) {
    return std::isdigit(static_cast<unsigned char>(ch)) != 0;
}

[[nodiscard]] bool is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

[[nodiscard]] int days_in_month(int year, unsigned int month) {
    static constexpr int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && is_leap_year(year)) {
        return 29;
    }
    return days[month - 1];
}

[[nodiscard]] int clamp_score(int value) {
    return std::clamp(value, 0, 100);
}

[[nodiscard]] int compliance_score(const ReminderStats& stats, int weight) {
    if (stats.shown <= 0) {
        return weight;
    }
    return (stats.completed * weight) / stats.shown;
}

}  // namespace

StatsManager::StatsManager(std::filesystem::path stats_directory)
    : stats_directory_(std::move(stats_directory)) {
    try {
        std::filesystem::create_directories(stats_directory_);
    } catch (const std::filesystem::filesystem_error& ex) {
        throw StatsError("Failed to create stats directory '" + stats_directory_.string() + "': " + ex.what());
    }
    cleanupOldStats();
}

std::filesystem::path StatsManager::defaultStatsDirectory() {
    const char* home = std::getenv("HOME");
    if (home == nullptr || std::string_view(home).empty()) {
        throw StatsError("HOME environment variable is not set");
    }

    return std::filesystem::path(home) / ".local" / "share" / "sandhika" / "stats";
}

void StatsManager::addActiveScreenTime(std::chrono::minutes duration) {
    if (duration < std::chrono::minutes::zero()) {
        throw std::invalid_argument("Screen time duration must not be negative");
    }
    updateToday([duration](DailyStats& stats) {
        stats.screen_time.active_minutes += static_cast<int>(duration.count());
    });
}

void StatsManager::recordEyeBreakShown() {
    updateToday([](DailyStats& stats) { ++stats.eye_break.shown; });
}

void StatsManager::recordEyeBreakCompleted() {
    updateToday([](DailyStats& stats) {
        ++stats.eye_break.completed;
        ++stats.streaks.eye_break_current;
        stats.streaks.eye_break_best = std::max(stats.streaks.eye_break_best, stats.streaks.eye_break_current);
    });
}

void StatsManager::recordEyeBreakSkipped() {
    updateToday([](DailyStats& stats) {
        ++stats.eye_break.skipped;
        stats.streaks.eye_break_current = 0;
    });
}

void StatsManager::recordWaterShown() {
    updateToday([](DailyStats& stats) { ++stats.water.shown; });
}

void StatsManager::recordWaterCompleted() {
    updateToday([](DailyStats& stats) { ++stats.water.completed; });
}

void StatsManager::recordWaterSkipped() {
    updateToday([](DailyStats& stats) { ++stats.water.skipped; });
}

DailyStats StatsManager::getTodayStats() const {
    return getDailyStats(currentDate());
}

DailyStats StatsManager::getDailyStats(std::chrono::year_month_day date) const {
    std::lock_guard lock(mutex_);
    return loadDay(date);
}

AggregateStats StatsManager::getWeeklyStats(std::chrono::year_month_day week_ending) const {
    std::lock_guard lock(mutex_);
    AggregateStats aggregate;
    const Days end {week_ending};
    for (int offset = 6; offset >= 0; --offset) {
        const auto date = std::chrono::year_month_day {end - std::chrono::days(offset)};
        const auto daily = loadDay(date);
        ++aggregate.days;
        aggregate.screen_time.active_minutes += daily.screen_time.active_minutes;
        aggregate.eye_break.shown += daily.eye_break.shown;
        aggregate.eye_break.completed += daily.eye_break.completed;
        aggregate.eye_break.skipped += daily.eye_break.skipped;
        aggregate.water.shown += daily.water.shown;
        aggregate.water.completed += daily.water.completed;
        aggregate.water.skipped += daily.water.skipped;
        aggregate.streaks.eye_break_current = daily.streaks.eye_break_current;
        aggregate.streaks.eye_break_best = std::max(aggregate.streaks.eye_break_best, daily.streaks.eye_break_best);
    }
    aggregate.health_score = calculateHealthScore(DailyStats {
        .date = week_ending,
        .screen_time = aggregate.screen_time,
        .eye_break = aggregate.eye_break,
        .water = aggregate.water,
        .streaks = aggregate.streaks,
    });
    return aggregate;
}

AggregateStats StatsManager::getMonthlyStats(std::chrono::year_month_day month) const {
    std::lock_guard lock(mutex_);
    AggregateStats aggregate;
    const int year = static_cast<int>(month.year());
    const unsigned int month_number = static_cast<unsigned int>(month.month());
    const int day_count = days_in_month(year, month_number);

    for (int day = 1; day <= day_count; ++day) {
        const auto daily = loadDay(std::chrono::year_month_day {
            std::chrono::year {year},
            std::chrono::month {month_number},
            std::chrono::day {static_cast<unsigned int>(day)},
        });
        ++aggregate.days;
        aggregate.screen_time.active_minutes += daily.screen_time.active_minutes;
        aggregate.eye_break.shown += daily.eye_break.shown;
        aggregate.eye_break.completed += daily.eye_break.completed;
        aggregate.eye_break.skipped += daily.eye_break.skipped;
        aggregate.water.shown += daily.water.shown;
        aggregate.water.completed += daily.water.completed;
        aggregate.water.skipped += daily.water.skipped;
        aggregate.streaks.eye_break_current = daily.streaks.eye_break_current;
        aggregate.streaks.eye_break_best = std::max(aggregate.streaks.eye_break_best, daily.streaks.eye_break_best);
    }
    aggregate.health_score = calculateHealthScore(DailyStats {
        .date = month,
        .screen_time = aggregate.screen_time,
        .eye_break = aggregate.eye_break,
        .water = aggregate.water,
        .streaks = aggregate.streaks,
    });
    return aggregate;
}

std::vector<DailyStats> StatsManager::getLastDays(std::size_t days) const {
    std::lock_guard lock(mutex_);
    std::vector<DailyStats> result;
    result.reserve(days);

    const Days today {currentDate()};
    for (std::size_t offset = 0; offset < days; ++offset) {
        result.push_back(loadDay(std::chrono::year_month_day {today - std::chrono::days(offset)}));
    }
    return result;
}

void StatsManager::cleanupOldStats() {
    std::lock_guard lock(mutex_);
    const Days oldest_allowed = Days {currentDate()} - std::chrono::days(29);

    try {
        if (!std::filesystem::exists(stats_directory_)) {
            return;
        }

        for (const auto& entry : std::filesystem::directory_iterator(stats_directory_)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".yaml") {
                continue;
            }

            const auto date = parseDate(entry.path().stem().string());
            if (Days {date} < oldest_allowed) {
                std::filesystem::remove(entry.path());
            }
        }
    } catch (const StatsError&) {
        throw;
    } catch (const std::filesystem::filesystem_error& ex) {
        throw StatsError("Failed to clean stats directory '" + stats_directory_.string() + "': " + ex.what());
    }
}

int StatsManager::calculateHealthScore(const DailyStats& stats) {
    int score = 0;
    score += compliance_score(stats.eye_break, 40);
    score += compliance_score(stats.water, 25);

    if (stats.screen_time.active_minutes <= 480) {
        score += 25;
    } else if (stats.screen_time.active_minutes <= 600) {
        score += 15;
    } else if (stats.screen_time.active_minutes <= 720) {
        score += 8;
    }

    score += std::min(stats.streaks.eye_break_current, 10);
    return clamp_score(score);
}

std::chrono::year_month_day StatsManager::currentDate() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t raw = std::chrono::system_clock::to_time_t(now);
    std::tm local {};
    localtime_r(&raw, &local);
    return std::chrono::year_month_day {
        std::chrono::year {local.tm_year + 1900},
        std::chrono::month {static_cast<unsigned int>(local.tm_mon + 1)},
        std::chrono::day {static_cast<unsigned int>(local.tm_mday)}
    };
}

std::string StatsManager::formatDate(std::chrono::year_month_day date) {
    std::ostringstream out;
    out << std::setfill('0') << std::setw(4) << static_cast<int>(date.year()) << '-'
        << std::setw(2) << static_cast<unsigned int>(date.month()) << '-'
        << std::setw(2) << static_cast<unsigned int>(date.day());
    return out.str();
}

std::chrono::year_month_day StatsManager::parseDate(std::string_view value) {
    if (value.size() != 10 || value[4] != '-' || value[7] != '-') {
        throw StatsError("Invalid stats date: " + std::string(value));
    }
    for (const int index : {0, 1, 2, 3, 5, 6, 8, 9}) {
        if (!is_digit(value[static_cast<std::size_t>(index)])) {
            throw StatsError("Invalid stats date: " + std::string(value));
        }
    }

    const int year = std::stoi(std::string(value.substr(0, 4)));
    const unsigned int month = static_cast<unsigned int>(std::stoi(std::string(value.substr(5, 2))));
    const unsigned int day = static_cast<unsigned int>(std::stoi(std::string(value.substr(8, 2))));
    std::chrono::year_month_day date {std::chrono::year {year}, std::chrono::month {month}, std::chrono::day {day}};
    if (!date.ok()) {
        throw StatsError("Invalid stats date: " + std::string(value));
    }
    return date;
}

std::filesystem::path StatsManager::pathForDate(std::chrono::year_month_day date) const {
    return stats_directory_ / (formatDate(date) + ".yaml");
}

DailyStats StatsManager::loadDay(std::chrono::year_month_day date) const {
    DailyStats stats;
    stats.date = date;

    const auto path = pathForDate(date);
    try {
        if (!std::filesystem::exists(path)) {
            return stats;
        }
    } catch (const std::filesystem::filesystem_error& ex) {
        throw StatsError("Failed to inspect stats file '" + path.string() + "': " + ex.what());
    }

    YAML::Node root;
    try {
        root = YAML::LoadFile(path.string());
    } catch (const YAML::Exception& ex) {
        throw StatsError("Failed to parse stats file '" + path.string() + "': " + ex.what());
    }

    if (root["date"]) {
        const auto parsed_date = parseDate(yaml_as_or<std::string>(root["date"], formatDate(date), "date"));
        if (parsed_date != date) {
            throw StatsError("Stats file date does not match filename: " + path.string());
        }
    }

    stats.screen_time.active_minutes =
        yaml_as_or<int>(root["screen_time"]["active_minutes"], 0, "screen_time.active_minutes");
    stats.eye_break.shown = yaml_as_or<int>(root["eye_break"]["shown"], 0, "eye_break.shown");
    stats.eye_break.completed = yaml_as_or<int>(root["eye_break"]["completed"], 0, "eye_break.completed");
    stats.eye_break.skipped = yaml_as_or<int>(root["eye_break"]["skipped"], 0, "eye_break.skipped");
    stats.water.shown = yaml_as_or<int>(root["water"]["shown"], 0, "water.shown");
    stats.water.completed = yaml_as_or<int>(root["water"]["completed"], 0, "water.completed");
    stats.water.skipped = yaml_as_or<int>(root["water"]["skipped"], 0, "water.skipped");
    stats.streaks.eye_break_current =
        yaml_as_or<int>(root["streaks"]["eye_break_current"], 0, "streaks.eye_break_current");
    stats.streaks.eye_break_best =
        yaml_as_or<int>(root["streaks"]["eye_break_best"], 0, "streaks.eye_break_best");

    return stats;
}

void StatsManager::saveDay(const DailyStats& stats) const {
    const auto path = pathForDate(stats.date);
    try {
        std::filesystem::create_directories(stats_directory_);
    } catch (const std::filesystem::filesystem_error& ex) {
        throw StatsError("Failed to create stats directory '" + stats_directory_.string() + "': " + ex.what());
    }

    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "date" << YAML::Value << formatDate(stats.date);
    out << YAML::Key << "screen_time" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "active_minutes" << YAML::Value << stats.screen_time.active_minutes;
    out << YAML::EndMap;
    out << YAML::Key << "eye_break" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "shown" << YAML::Value << stats.eye_break.shown;
    out << YAML::Key << "completed" << YAML::Value << stats.eye_break.completed;
    out << YAML::Key << "skipped" << YAML::Value << stats.eye_break.skipped;
    out << YAML::EndMap;
    out << YAML::Key << "water" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "shown" << YAML::Value << stats.water.shown;
    out << YAML::Key << "completed" << YAML::Value << stats.water.completed;
    out << YAML::Key << "skipped" << YAML::Value << stats.water.skipped;
    out << YAML::EndMap;
    out << YAML::Key << "streaks" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "eye_break_current" << YAML::Value << stats.streaks.eye_break_current;
    out << YAML::Key << "eye_break_best" << YAML::Value << stats.streaks.eye_break_best;
    out << YAML::EndMap;
    out << YAML::EndMap;

    if (!out.good()) {
        throw StatsError("Failed to serialize stats file '" + path.string() + "'");
    }

    const auto tmp_path = path.string() + ".tmp";
    {
        std::ofstream file(tmp_path, std::ios::trunc);
        if (!file) {
            throw StatsError("Failed to open temporary stats file for writing: " + tmp_path);
        }
        file << out.c_str() << '\n';
        if (!file.good()) {
            throw StatsError("Failed to write temporary stats file: " + tmp_path);
        }
    }
    std::error_code ec;
    std::filesystem::rename(tmp_path, path, ec);
    if (ec) {
        throw StatsError("Failed to rename temporary stats file to: " + path.string());
    }
}

void StatsManager::updateToday(const std::function<void(DailyStats&)>& updater) {
    std::lock_guard lock(mutex_);
    DailyStats stats = loadDay(currentDate());
    updater(stats);
    saveDay(stats);
}

}  // namespace sandhika::stats
