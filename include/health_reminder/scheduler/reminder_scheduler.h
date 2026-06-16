#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

namespace health_reminder::scheduler {

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

struct ReminderSchedule {
    ReminderScheduleType type {ReminderScheduleType::Interval};
    std::chrono::seconds interval {std::chrono::minutes(20)};
    TimeOfDay at {0, 0};
    Weekday weekday {Weekday::Monday};
    int day {1};

    [[nodiscard]] static ReminderSchedule intervalEvery(std::chrono::seconds interval);
    [[nodiscard]] static ReminderSchedule dailyAt(TimeOfDay at);
    [[nodiscard]] static ReminderSchedule weeklyAt(Weekday weekday, TimeOfDay at);
    [[nodiscard]] static ReminderSchedule monthlyAt(int day, TimeOfDay at);
};

struct ReminderDefinition {
    std::string id;
    std::string name;
    std::string message;
    ReminderSchedule schedule;
    bool enabled {true};
};

struct ReminderEvent {
    std::string id;
    std::string name;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
};

struct ReminderCallbacks {
    std::function<void(const ReminderEvent&)> reminder_triggered;
    std::function<void(const ReminderEvent&)> reminder_completed;
    std::function<void(const ReminderEvent&)> reminder_skipped;
    std::function<void(const ReminderEvent&)> reminder_snoozed;
    std::function<void(const ReminderEvent&)> reminder_paused;
};

class ReminderScheduler {
public:
    explicit ReminderScheduler(ReminderCallbacks callbacks = {});
    ~ReminderScheduler();

    ReminderScheduler(const ReminderScheduler&) = delete;
    ReminderScheduler& operator=(const ReminderScheduler&) = delete;

    void start();
    void stop();

    [[nodiscard]] bool isRunning() const;

    void setCallbacks(ReminderCallbacks callbacks);

    void addOrUpdateReminder(ReminderDefinition reminder);
    [[nodiscard]] bool removeReminder(std::string_view id);
    void clearReminders();

    [[nodiscard]] bool completeReminder(std::string_view id);
    [[nodiscard]] bool skipReminder(std::string_view id);
    [[nodiscard]] bool snoozeReminder(std::string_view id, std::chrono::seconds duration);
    [[nodiscard]] bool pauseReminder(std::string_view id, std::chrono::seconds duration);
    void pauseAll(std::chrono::seconds duration);
    void resumeAll();

    [[nodiscard]] std::size_t reminderCount() const;
    [[nodiscard]] std::optional<std::chrono::system_clock::time_point> nextTriggerTime(std::string_view id) const;

private:
    struct RuntimeReminder {
        ReminderDefinition definition;
        std::chrono::system_clock::time_point next_trigger;
        std::optional<std::chrono::system_clock::time_point> paused_until;
        bool pending {false};
    };

    static void validateReminder(const ReminderDefinition& reminder);
    static std::chrono::system_clock::time_point calculateNextTrigger(
        const ReminderDefinition& reminder,
        std::chrono::system_clock::time_point from);
    static ReminderEvent makeEvent(const RuntimeReminder& reminder);

    void run(std::stop_token stop_token);

    mutable std::mutex mutex_;
    std::condition_variable_any condition_;
    std::map<std::string, RuntimeReminder, std::less<>> reminders_;
    ReminderCallbacks callbacks_;
    std::jthread worker_;
    bool running_ {false};
};

}  // namespace health_reminder::scheduler
