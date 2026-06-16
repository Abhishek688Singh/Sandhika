#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace health_reminder::stats {

class StatsError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct ScreenTimeStats {
    int active_minutes {0};
};

struct ReminderStats {
    int shown {0};
    int completed {0};
    int skipped {0};
};

struct StreakStats {
    int eye_break_current {0};
    int eye_break_best {0};
};

struct DailyStats {
    std::chrono::year_month_day date;
    ScreenTimeStats screen_time;
    ReminderStats eye_break;
    ReminderStats water;
    StreakStats streaks;
};

struct AggregateStats {
    int days {0};
    ScreenTimeStats screen_time;
    ReminderStats eye_break;
    ReminderStats water;
    StreakStats streaks;
    int health_score {0};
};

class StatsManager {
public:
    explicit StatsManager(std::filesystem::path stats_directory);

    static std::filesystem::path defaultStatsDirectory();

    void addActiveScreenTime(std::chrono::minutes duration);

    void recordEyeBreakShown();
    void recordEyeBreakCompleted();
    void recordEyeBreakSkipped();

    void recordWaterShown();
    void recordWaterCompleted();
    void recordWaterSkipped();

    [[nodiscard]] DailyStats getTodayStats() const;
    [[nodiscard]] DailyStats getDailyStats(std::chrono::year_month_day date) const;
    [[nodiscard]] AggregateStats getWeeklyStats(std::chrono::year_month_day week_ending) const;
    [[nodiscard]] AggregateStats getMonthlyStats(std::chrono::year_month_day month) const;
    [[nodiscard]] std::vector<DailyStats> getLastDays(std::size_t days) const;

    void cleanupOldStats();

    [[nodiscard]] static int calculateHealthScore(const DailyStats& stats);

private:
    using Days = std::chrono::sys_days;

    static std::chrono::year_month_day currentDate();
    static std::string formatDate(std::chrono::year_month_day date);
    static std::chrono::year_month_day parseDate(std::string_view value);

    [[nodiscard]] std::filesystem::path pathForDate(std::chrono::year_month_day date) const;
    [[nodiscard]] DailyStats loadDay(std::chrono::year_month_day date) const;
    void saveDay(const DailyStats& stats) const;
    void updateToday(const std::function<void(DailyStats&)>& updater);

    std::filesystem::path stats_directory_;
    mutable std::mutex mutex_;
};

}  // namespace health_reminder::stats
