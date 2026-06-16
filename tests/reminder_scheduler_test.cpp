#include "sandhika/scheduler/reminder_scheduler.h"

#include <chrono>
#include <condition_variable>
#include <ctime>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>

using sandhika::scheduler::ReminderCallbacks;
using sandhika::scheduler::ReminderDefinition;
using sandhika::scheduler::ReminderEvent;
using sandhika::scheduler::ReminderSchedule;
using sandhika::scheduler::ReminderScheduler;
using sandhika::scheduler::TimeOfDay;
using sandhika::scheduler::Weekday;

namespace {

struct CallbackState {
    std::mutex mutex;
    std::condition_variable condition;
    int triggered {0};
    int completed {0};
    int skipped {0};
    int snoozed {0};
    int paused {0};
};

void assert_true(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

ReminderDefinition make_interval_reminder(std::chrono::seconds interval) {
    return ReminderDefinition {
        .id = "eye",
        .name = "Eye Break",
        .message = "Rest your eyes",
        .schedule = ReminderSchedule::intervalEvery(interval),
        .enabled = true,
    };
}

ReminderCallbacks make_callbacks(CallbackState& state) {
    ReminderCallbacks callbacks;
    callbacks.reminder_triggered = [&state](const ReminderEvent&) {
        std::lock_guard lock(state.mutex);
        ++state.triggered;
        state.condition.notify_all();
    };
    callbacks.reminder_completed = [&state](const ReminderEvent&) {
        std::lock_guard lock(state.mutex);
        ++state.completed;
        state.condition.notify_all();
    };
    callbacks.reminder_skipped = [&state](const ReminderEvent&) {
        std::lock_guard lock(state.mutex);
        ++state.skipped;
        state.condition.notify_all();
    };
    callbacks.reminder_snoozed = [&state](const ReminderEvent&) {
        std::lock_guard lock(state.mutex);
        ++state.snoozed;
        state.condition.notify_all();
    };
    callbacks.reminder_paused = [&state](const ReminderEvent&) {
        std::lock_guard lock(state.mutex);
        ++state.paused;
        state.condition.notify_all();
    };
    return callbacks;
}

bool wait_for_count(CallbackState& state, int CallbackState::*member, int expected, std::chrono::milliseconds timeout) {
    std::unique_lock lock(state.mutex);
    return state.condition.wait_for(lock, timeout, [&state, member, expected] {
        return state.*member >= expected;
    });
}

TimeOfDay one_minute_from_now() {
    const auto now = std::chrono::system_clock::now() + std::chrono::minutes(1);
    const std::time_t raw = std::chrono::system_clock::to_time_t(now);
    std::tm local {};
    localtime_r(&raw, &local);
    return TimeOfDay {.hour = local.tm_hour, .minute = local.tm_min};
}

void test_interval_trigger_and_complete() {
    CallbackState state;
    ReminderScheduler scheduler(make_callbacks(state));
    scheduler.addOrUpdateReminder(make_interval_reminder(std::chrono::seconds(1)));
    scheduler.start();

    assert_true(wait_for_count(state, &CallbackState::triggered, 1, std::chrono::seconds(3)),
                "Expected interval reminder to trigger");
    assert_true(scheduler.completeReminder("eye"), "Expected completeReminder to succeed");
    assert_true(wait_for_count(state, &CallbackState::completed, 1, std::chrono::seconds(1)),
                "Expected completion callback");
    scheduler.stop();
}

void test_snooze_reschedules_reminder() {
    CallbackState state;
    ReminderScheduler scheduler(make_callbacks(state));
    scheduler.addOrUpdateReminder(make_interval_reminder(std::chrono::seconds(1)));
    scheduler.start();

    assert_true(wait_for_count(state, &CallbackState::triggered, 1, std::chrono::seconds(3)),
                "Expected reminder before snooze");
    assert_true(scheduler.snoozeReminder("eye", std::chrono::seconds(1)), "Expected snoozeReminder to succeed");
    assert_true(wait_for_count(state, &CallbackState::snoozed, 1, std::chrono::seconds(1)),
                "Expected snooze callback");
    assert_true(wait_for_count(state, &CallbackState::triggered, 2, std::chrono::seconds(3)),
                "Expected reminder after snooze");
    scheduler.stop();
}

void test_pause_delays_reminder() {
    CallbackState state;
    ReminderScheduler scheduler(make_callbacks(state));
    scheduler.addOrUpdateReminder(make_interval_reminder(std::chrono::seconds(1)));
    assert_true(scheduler.pauseReminder("eye", std::chrono::seconds(2)), "Expected pauseReminder to succeed");
    scheduler.start();

    assert_true(wait_for_count(state, &CallbackState::paused, 1, std::chrono::seconds(1)),
                "Expected pause callback");
    assert_true(!wait_for_count(state, &CallbackState::triggered, 1, std::chrono::milliseconds(1200)),
                "Reminder should not trigger while paused");
    assert_true(wait_for_count(state, &CallbackState::triggered, 1, std::chrono::seconds(4)),
                "Expected reminder after pause expires");
    scheduler.stop();
}

void test_calendar_schedules_have_future_trigger_times() {
    ReminderScheduler scheduler;
    const auto now = std::chrono::system_clock::now();
    const auto at = one_minute_from_now();

    scheduler.addOrUpdateReminder(ReminderDefinition {
        .id = "daily",
        .name = "Daily",
        .message = "Daily reminder",
        .schedule = ReminderSchedule::dailyAt(at),
        .enabled = true,
    });
    scheduler.addOrUpdateReminder(ReminderDefinition {
        .id = "weekly",
        .name = "Weekly",
        .message = "Weekly reminder",
        .schedule = ReminderSchedule::weeklyAt(Weekday::Sunday, at),
        .enabled = true,
    });
    scheduler.addOrUpdateReminder(ReminderDefinition {
        .id = "monthly",
        .name = "Monthly",
        .message = "Monthly reminder",
        .schedule = ReminderSchedule::monthlyAt(31, at),
        .enabled = true,
    });

    const auto daily = scheduler.nextTriggerTime("daily");
    const auto weekly = scheduler.nextTriggerTime("weekly");
    const auto monthly = scheduler.nextTriggerTime("monthly");
    assert_true(daily.has_value() && daily.value() > now, "Expected future daily trigger");
    assert_true(weekly.has_value() && weekly.value() > now, "Expected future weekly trigger");
    assert_true(monthly.has_value() && monthly.value() > now, "Expected future monthly trigger");
}

void test_validation_rejects_invalid_reminders() {
    ReminderScheduler scheduler;

    bool empty_id_thrown = false;
    try {
        scheduler.addOrUpdateReminder(ReminderDefinition {
            .id = "",
            .name = "Invalid",
            .message = "Invalid",
            .schedule = ReminderSchedule::intervalEvery(std::chrono::seconds(1)),
            .enabled = true,
        });
    } catch (const std::invalid_argument&) {
        empty_id_thrown = true;
    }
    assert_true(empty_id_thrown, "Expected empty id to be rejected");

    bool invalid_interval_thrown = false;
    try {
        scheduler.addOrUpdateReminder(make_interval_reminder(std::chrono::seconds(0)));
    } catch (const std::invalid_argument&) {
        invalid_interval_thrown = true;
    }
    assert_true(invalid_interval_thrown, "Expected invalid interval to be rejected");

    bool invalid_monthly_day_thrown = false;
    try {
        scheduler.addOrUpdateReminder(ReminderDefinition {
            .id = "monthly",
            .name = "Monthly",
            .message = "Invalid",
            .schedule = ReminderSchedule::monthlyAt(32, TimeOfDay {.hour = 10, .minute = 0}),
            .enabled = true,
        });
    } catch (const std::invalid_argument&) {
        invalid_monthly_day_thrown = true;
    }
    assert_true(invalid_monthly_day_thrown, "Expected invalid monthly day to be rejected");
}

}  // namespace

int main() {
    try {
        test_interval_trigger_and_complete();
        test_snooze_reschedules_reminder();
        test_pause_delays_reminder();
        test_calendar_schedules_have_future_trigger_times();
        test_validation_rejects_invalid_reminders();
    } catch (const std::exception& ex) {
        std::cerr << "Test failure: " << ex.what() << '\n';
        return 1;
    }

    std::cout << "All ReminderScheduler tests passed\n";
    return 0;
}
