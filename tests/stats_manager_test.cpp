#include "sandhika/stats/stats_manager.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using sandhika::stats::DailyStats;
using sandhika::stats::StatsError;
using sandhika::stats::StatsManager;

namespace {

class TempDir {
public:
    TempDir()
        : path_(make_path()) {
        std::filesystem::create_directories(path_);
    }

    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    [[nodiscard]] const std::filesystem::path& path() const { return path_; }

private:
    static std::filesystem::path make_path() {
        static std::atomic<unsigned int> counter {0};
        const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
        return std::filesystem::temp_directory_path() /
               ("sandhika-stats-test-" + std::to_string(now) + "-" +
                std::to_string(counter.fetch_add(1, std::memory_order_relaxed)));
    }

    std::filesystem::path path_;
};

void assert_true(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::string format_date(std::chrono::year_month_day date) {
    std::ostringstream out;
    out << std::setfill('0') << std::setw(4) << static_cast<int>(date.year()) << '-'
        << std::setw(2) << static_cast<unsigned int>(date.month()) << '-'
        << std::setw(2) << static_cast<unsigned int>(date.day());
    return out.str();
}

std::chrono::year_month_day today() {
    return std::chrono::year_month_day {std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())};
}

void write_stats_file(
    const std::filesystem::path& directory,
    std::chrono::year_month_day date,
    int active_minutes,
    int eye_shown,
    int eye_completed,
    int eye_skipped,
    int water_shown,
    int water_completed,
    int water_skipped) {
    const auto path = directory / (format_date(date) + ".yaml");
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        throw std::runtime_error("Failed to write stats test file");
    }

    out << "date: " << format_date(date) << '\n';
    out << "screen_time:\n";
    out << "  active_minutes: " << active_minutes << '\n';
    out << "eye_break:\n";
    out << "  shown: " << eye_shown << '\n';
    out << "  completed: " << eye_completed << '\n';
    out << "  skipped: " << eye_skipped << '\n';
    out << "water:\n";
    out << "  shown: " << water_shown << '\n';
    out << "  completed: " << water_completed << '\n';
    out << "  skipped: " << water_skipped << '\n';
    out << "streaks:\n";
    out << "  eye_break_current: " << eye_completed << '\n';
    out << "  eye_break_best: " << eye_completed << '\n';
}

void test_records_today_stats() {
    TempDir dir;
    StatsManager manager(dir.path());

    manager.addActiveScreenTime(std::chrono::minutes(45));
    manager.recordEyeBreakShown();
    manager.recordEyeBreakCompleted();
    manager.recordWaterShown();
    manager.recordWaterSkipped();

    const auto stats = manager.getTodayStats();
    assert_true(stats.screen_time.active_minutes == 45, "screen time mismatch");
    assert_true(stats.eye_break.shown == 1, "eye shown mismatch");
    assert_true(stats.eye_break.completed == 1, "eye completed mismatch");
    assert_true(stats.streaks.eye_break_current == 1, "current streak mismatch");
    assert_true(stats.streaks.eye_break_best == 1, "best streak mismatch");
    assert_true(stats.water.shown == 1, "water shown mismatch");
    assert_true(stats.water.skipped == 1, "water skipped mismatch");
}

void test_weekly_and_monthly_aggregates() {
    TempDir dir;
    const auto base = std::chrono::sys_days {today()};
    const auto day1 = std::chrono::year_month_day {base - std::chrono::days(1)};
    const auto day2 = std::chrono::year_month_day {base};

    write_stats_file(dir.path(), day1, 100, 5, 4, 1, 3, 2, 1);
    write_stats_file(dir.path(), day2, 120, 6, 5, 1, 4, 3, 1);

    StatsManager manager(dir.path());
    const auto weekly = manager.getWeeklyStats(day2);
    assert_true(weekly.screen_time.active_minutes == 220, "weekly screen time mismatch");
    assert_true(weekly.eye_break.shown == 11, "weekly eye shown mismatch");
    assert_true(weekly.eye_break.completed == 9, "weekly eye completed mismatch");
    assert_true(weekly.water.completed == 5, "weekly water completed mismatch");
    assert_true(weekly.health_score > 0, "weekly health score expected");

    const auto monthly = manager.getMonthlyStats(day2);
    assert_true(monthly.screen_time.active_minutes >= 220, "monthly screen time mismatch");
    assert_true(monthly.eye_break.completed >= 9, "monthly eye completed mismatch");
}

void test_cleanup_keeps_last_30_days() {
    TempDir dir;
    const auto current = std::chrono::sys_days {today()};
    const auto old_day = std::chrono::year_month_day {current - std::chrono::days(40)};
    const auto recent_day = std::chrono::year_month_day {current - std::chrono::days(2)};

    write_stats_file(dir.path(), old_day, 10, 1, 1, 0, 1, 1, 0);
    write_stats_file(dir.path(), recent_day, 10, 1, 1, 0, 1, 1, 0);

    StatsManager manager(dir.path());
    manager.cleanupOldStats();

    assert_true(!std::filesystem::exists(dir.path() / (format_date(old_day) + ".yaml")),
                "old stats file should be removed");
    assert_true(std::filesystem::exists(dir.path() / (format_date(recent_day) + ".yaml")),
                "recent stats file should remain");
}

void test_malformed_yaml_throws_stats_error() {
    TempDir dir;
    const auto date = today();
    std::ofstream out(dir.path() / (format_date(date) + ".yaml"), std::ios::trunc);
    out << "screen_time: [\n";
    out.close();

    StatsManager manager(dir.path());
    bool thrown = false;
    try {
        (void)manager.getDailyStats(date);
    } catch (const StatsError&) {
        thrown = true;
    }
    assert_true(thrown, "Expected malformed stats YAML to throw StatsError");
}

void test_health_score() {
    DailyStats stats;
    stats.date = today();
    stats.screen_time.active_minutes = 480;
    stats.eye_break.shown = 10;
    stats.eye_break.completed = 9;
    stats.water.shown = 8;
    stats.water.completed = 8;
    stats.streaks.eye_break_current = 5;

    const int score = StatsManager::calculateHealthScore(stats);
    assert_true(score == 91, "health score mismatch");
}

}  // namespace

int main() {
    try {
        test_records_today_stats();
        test_weekly_and_monthly_aggregates();
        test_cleanup_keeps_last_30_days();
        test_malformed_yaml_throws_stats_error();
        test_health_score();
    } catch (const std::exception& ex) {
        std::cerr << "Test failure: " << ex.what() << '\n';
        return 1;
    }

    std::cout << "All StatsManager tests passed\n";
    return 0;
}
