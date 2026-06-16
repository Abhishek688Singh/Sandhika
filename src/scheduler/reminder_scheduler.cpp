#include "sandhika/scheduler/reminder_scheduler.h"

#include <algorithm>
#include <ctime>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sandhika::scheduler {
namespace {

[[nodiscard]] bool is_valid_time(TimeOfDay time) {
    return time.hour >= 0 && time.hour <= 23 && time.minute >= 0 && time.minute <= 59;
}

[[nodiscard]] int weekday_to_tm(Weekday weekday) {
    switch (weekday) {
        case Weekday::Monday:
            return 1;
        case Weekday::Tuesday:
            return 2;
        case Weekday::Wednesday:
            return 3;
        case Weekday::Thursday:
            return 4;
        case Weekday::Friday:
            return 5;
        case Weekday::Saturday:
            return 6;
        case Weekday::Sunday:
            return 0;
    }
    return 1;
}

[[nodiscard]] bool is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

[[nodiscard]] int days_in_month(int year, int month) {
    static constexpr int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && is_leap_year(year)) {
        return 29;
    }
    return days[month - 1];
}

[[nodiscard]] std::tm local_time(std::chrono::system_clock::time_point time) {
    const std::time_t raw = std::chrono::system_clock::to_time_t(time);
    std::tm result {};
    localtime_r(&raw, &result);
    return result;
}

[[nodiscard]] std::chrono::system_clock::time_point make_local_time_point(
    int year,
    int month,
    int day,
    TimeOfDay at) {
    std::tm value {};
    value.tm_year = year - 1900;
    value.tm_mon = month - 1;
    value.tm_mday = day;
    value.tm_hour = at.hour;
    value.tm_min = at.minute;
    value.tm_sec = 0;
    value.tm_isdst = -1;
    return std::chrono::system_clock::from_time_t(std::mktime(&value));
}

[[nodiscard]] std::chrono::system_clock::time_point next_daily_time(
    std::chrono::system_clock::time_point from,
    TimeOfDay at) {
    std::tm local = local_time(from);
    auto candidate = make_local_time_point(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday, at);
    if (candidate <= from) {
        local.tm_mday += 1;
        local.tm_hour = at.hour;
        local.tm_min = at.minute;
        local.tm_sec = 0;
        local.tm_isdst = -1;
        candidate = std::chrono::system_clock::from_time_t(std::mktime(&local));
    }
    return candidate;
}

[[nodiscard]] std::chrono::system_clock::time_point next_weekly_time(
    std::chrono::system_clock::time_point from,
    Weekday weekday,
    TimeOfDay at) {
    std::tm local = local_time(from);
    const int current_weekday = local.tm_wday;
    const int target_weekday = weekday_to_tm(weekday);
    int days_ahead = (target_weekday - current_weekday + 7) % 7;

    auto candidate = make_local_time_point(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday, at);
    if (days_ahead > 0 || candidate <= from) {
        if (days_ahead == 0) days_ahead = 7;
        local.tm_mday += days_ahead;
        local.tm_hour = at.hour;
        local.tm_min = at.minute;
        local.tm_sec = 0;
        local.tm_isdst = -1;
        candidate = std::chrono::system_clock::from_time_t(std::mktime(&local));
    }
    return candidate;
}

[[nodiscard]] std::chrono::system_clock::time_point next_monthly_time(
    std::chrono::system_clock::time_point from,
    int day,
    TimeOfDay at) {
    std::tm local = local_time(from);
    int year = local.tm_year + 1900;
    int month = local.tm_mon + 1;

    for (int attempts = 0; attempts < 24; ++attempts) {
        const int target_day = std::min(day, days_in_month(year, month));
        auto candidate = make_local_time_point(year, month, target_day, at);
        if (candidate > from) {
            return candidate;
        }

        ++month;
        if (month > 12) {
            month = 1;
            ++year;
        }
    }

    throw std::logic_error("Unable to calculate next monthly reminder time");
}

}  // namespace

ReminderSchedule ReminderSchedule::intervalEvery(std::chrono::seconds interval) {
    ReminderSchedule schedule;
    schedule.type = ReminderScheduleType::Interval;
    schedule.interval = interval;
    return schedule;
}

ReminderSchedule ReminderSchedule::dailyAt(TimeOfDay at) {
    ReminderSchedule schedule;
    schedule.type = ReminderScheduleType::Daily;
    schedule.at = at;
    return schedule;
}

ReminderSchedule ReminderSchedule::weeklyAt(Weekday weekday, TimeOfDay at) {
    ReminderSchedule schedule;
    schedule.type = ReminderScheduleType::Weekly;
    schedule.weekday = weekday;
    schedule.at = at;
    return schedule;
}

ReminderSchedule ReminderSchedule::monthlyAt(int day, TimeOfDay at) {
    ReminderSchedule schedule;
    schedule.type = ReminderScheduleType::Monthly;
    schedule.day = day;
    schedule.at = at;
    return schedule;
}

ReminderScheduler::ReminderScheduler(ReminderCallbacks callbacks)
    : callbacks_(std::move(callbacks)) {}

ReminderScheduler::~ReminderScheduler() {
    stop();
}

void ReminderScheduler::start() {
    std::lock_guard lock(mutex_);
    if (running_) {
        return;
    }

    running_ = true;
    worker_ = std::jthread([this](std::stop_token stop_token) { run(stop_token); });
}

void ReminderScheduler::stop() {
    {
        std::lock_guard lock(mutex_);
        if (!running_) {
            return;
        }
        running_ = false;
    }

    worker_.request_stop();
    condition_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
}

bool ReminderScheduler::isRunning() const {
    std::lock_guard lock(mutex_);
    return running_;
}

void ReminderScheduler::setCallbacks(ReminderCallbacks callbacks) {
    std::lock_guard lock(mutex_);
    callbacks_ = std::move(callbacks);
}

void ReminderScheduler::addOrUpdateReminder(ReminderDefinition reminder) {
    validateReminder(reminder);

    const auto now = std::chrono::system_clock::now();
    const auto next_trigger = calculateNextTrigger(reminder, now);
    RuntimeReminder runtime {
        .definition = std::move(reminder),
        .next_trigger = next_trigger,
        .paused_until = std::nullopt,
        .pending = false,
    };

    std::lock_guard lock(mutex_);
    reminders_[runtime.definition.id] = std::move(runtime);
    condition_.notify_all();
}

bool ReminderScheduler::removeReminder(std::string_view id) {
    std::lock_guard lock(mutex_);
    const auto it = reminders_.find(id);
    if (it == reminders_.end()) {
        return false;
    }

    reminders_.erase(it);
    condition_.notify_all();
    return true;
}

void ReminderScheduler::clearReminders() {
    std::lock_guard lock(mutex_);
    reminders_.clear();
    condition_.notify_all();
}

bool ReminderScheduler::completeReminder(std::string_view id) {
    ReminderCallbacks callbacks;
    ReminderEvent event;
    {
        std::lock_guard lock(mutex_);
        auto it = reminders_.find(id);
        if (it == reminders_.end()) {
            return false;
        }

        it->second.pending = false;
        it->second.next_trigger = calculateNextTrigger(it->second.definition, std::chrono::system_clock::now());
        it->second.paused_until.reset();
        event = makeEvent(it->second);
        callbacks = callbacks_;
        condition_.notify_all();
    }

    if (callbacks.reminder_completed) {
        callbacks.reminder_completed(event);
    }
    return true;
}

bool ReminderScheduler::skipReminder(std::string_view id) {
    ReminderCallbacks callbacks;
    ReminderEvent event;
    {
        std::lock_guard lock(mutex_);
        auto it = reminders_.find(id);
        if (it == reminders_.end()) {
            return false;
        }

        it->second.pending = false;
        it->second.next_trigger = calculateNextTrigger(it->second.definition, std::chrono::system_clock::now());
        it->second.paused_until.reset();
        event = makeEvent(it->second);
        callbacks = callbacks_;
        condition_.notify_all();
    }

    if (callbacks.reminder_skipped) {
        callbacks.reminder_skipped(event);
    }
    return true;
}

bool ReminderScheduler::snoozeReminder(std::string_view id, std::chrono::seconds duration) {
    if (duration <= std::chrono::seconds::zero()) {
        throw std::invalid_argument("Snooze duration must be positive");
    }

    ReminderCallbacks callbacks;
    ReminderEvent event;
    {
        std::lock_guard lock(mutex_);
        auto it = reminders_.find(id);
        if (it == reminders_.end()) {
            return false;
        }

        it->second.pending = false;
        it->second.next_trigger = std::chrono::system_clock::now() + duration;
        it->second.paused_until.reset();
        event = makeEvent(it->second);
        callbacks = callbacks_;
        condition_.notify_all();
    }

    if (callbacks.reminder_snoozed) {
        callbacks.reminder_snoozed(event);
    }
    return true;
}

bool ReminderScheduler::pauseReminder(std::string_view id, std::chrono::seconds duration) {
    if (duration <= std::chrono::seconds::zero()) {
        throw std::invalid_argument("Pause duration must be positive");
    }

    ReminderCallbacks callbacks;
    ReminderEvent event;
    {
        std::lock_guard lock(mutex_);
        auto it = reminders_.find(id);
        if (it == reminders_.end()) {
            return false;
        }

        it->second.pending = false;
        it->second.paused_until = std::chrono::system_clock::now() + duration;
        event = makeEvent(it->second);
        callbacks = callbacks_;
        condition_.notify_all();
    }

    if (callbacks.reminder_paused) {
        callbacks.reminder_paused(event);
    }
    return true;
}

void ReminderScheduler::pauseAll(std::chrono::seconds duration) {
    if (duration <= std::chrono::seconds::zero()) {
        throw std::invalid_argument("Pause duration must be positive");
    }

    ReminderCallbacks callbacks;
    std::vector<ReminderEvent> events;
    {
        std::lock_guard lock(mutex_);
        const auto paused_until = std::chrono::system_clock::now() + duration;
        callbacks = callbacks_;
        for (auto& [_, reminder] : reminders_) {
            reminder.pending = false;
            reminder.paused_until = paused_until;
            events.push_back(makeEvent(reminder));
        }
        condition_.notify_all();
    }

    if (callbacks.reminder_paused) {
        for (const auto& event : events) {
            callbacks.reminder_paused(event);
        }
    }
}

void ReminderScheduler::resumeAll() {
    std::lock_guard lock(mutex_);
    const auto now = std::chrono::system_clock::now();
    for (auto& [_, reminder] : reminders_) {
        reminder.pending = false;
        reminder.paused_until.reset();
        if (reminder.next_trigger < now) {
            reminder.next_trigger = calculateNextTrigger(reminder.definition, now);
        }
    }
    condition_.notify_all();
}

std::size_t ReminderScheduler::reminderCount() const {
    std::lock_guard lock(mutex_);
    return reminders_.size();
}

std::optional<std::chrono::system_clock::time_point> ReminderScheduler::nextTriggerTime(std::string_view id) const {
    std::lock_guard lock(mutex_);
    const auto it = reminders_.find(id);
    if (it == reminders_.end()) {
        return std::nullopt;
    }
    return it->second.next_trigger;
}

void ReminderScheduler::validateReminder(const ReminderDefinition& reminder) {
    if (reminder.id.empty()) {
        throw std::invalid_argument("Reminder id must not be empty");
    }
    if (reminder.name.empty()) {
        throw std::invalid_argument("Reminder name must not be empty");
    }

    switch (reminder.schedule.type) {
        case ReminderScheduleType::Interval:
            if (reminder.schedule.interval <= std::chrono::seconds::zero()) {
                throw std::invalid_argument("Reminder interval must be positive");
            }
            break;
        case ReminderScheduleType::Daily:
        case ReminderScheduleType::Weekly:
            if (!is_valid_time(reminder.schedule.at)) {
                throw std::invalid_argument("Reminder time must be in range 00:00 to 23:59");
            }
            break;
        case ReminderScheduleType::Monthly:
            if (!is_valid_time(reminder.schedule.at)) {
                throw std::invalid_argument("Reminder time must be in range 00:00 to 23:59");
            }
            if (reminder.schedule.day < 1 || reminder.schedule.day > 31) {
                throw std::invalid_argument("Reminder monthly day must be in range 1 to 31");
            }
            break;
    }
}

std::chrono::system_clock::time_point ReminderScheduler::calculateNextTrigger(
    const ReminderDefinition& reminder,
    std::chrono::system_clock::time_point from) {
    switch (reminder.schedule.type) {
        case ReminderScheduleType::Interval:
            return from + reminder.schedule.interval;
        case ReminderScheduleType::Daily:
            return next_daily_time(from, reminder.schedule.at);
        case ReminderScheduleType::Weekly:
            return next_weekly_time(from, reminder.schedule.weekday, reminder.schedule.at);
        case ReminderScheduleType::Monthly:
            return next_monthly_time(from, reminder.schedule.day, reminder.schedule.at);
    }
    throw std::logic_error("Unsupported reminder schedule type");
}

ReminderEvent ReminderScheduler::makeEvent(const RuntimeReminder& reminder) {
    return ReminderEvent {
        .id = reminder.definition.id,
        .name = reminder.definition.name,
        .message = reminder.definition.message,
        .timestamp = std::chrono::system_clock::now(),
    };
}

void ReminderScheduler::run(std::stop_token stop_token) {
    std::unique_lock lock(mutex_);

    while (!stop_token.stop_requested()) {
        const auto now = std::chrono::system_clock::now();
        std::optional<std::chrono::system_clock::time_point> next_wake;
        std::vector<ReminderEvent> triggered_events;
        ReminderCallbacks callbacks = callbacks_;

        for (auto& [_, reminder] : reminders_) {
            if (!reminder.definition.enabled || reminder.pending) {
                continue;
            }

            if (reminder.paused_until.has_value()) {
                if (now < reminder.paused_until.value()) {
                    next_wake = next_wake.has_value() ? std::min(next_wake.value(), reminder.paused_until.value())
                                                      : reminder.paused_until.value();
                    continue;
                }
                reminder.paused_until.reset();
                if (reminder.next_trigger < now) {
                    reminder.next_trigger = calculateNextTrigger(reminder.definition, now);
                }
            }

            if (reminder.next_trigger <= now) {
                reminder.pending = true;
                triggered_events.push_back(makeEvent(reminder));
                continue;
            }

            next_wake = next_wake.has_value() ? std::min(next_wake.value(), reminder.next_trigger)
                                              : reminder.next_trigger;
        }

        if (!triggered_events.empty()) {
            lock.unlock();
            if (callbacks.reminder_triggered) {
                for (const auto& event : triggered_events) {
                    callbacks.reminder_triggered(event);
                }
            }
            lock.lock();
            continue;
        }

        if (next_wake.has_value()) {
            condition_.wait_until(lock, next_wake.value());
        } else {
            condition_.wait(lock);
        }
    }
}

}  // namespace sandhika::scheduler
