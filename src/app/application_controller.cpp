#include "sandhika/app/application_controller.h"
#include "sandhika/notifications/notification_window.h"

#include <QApplication>
#include <QDebug>

namespace sandhika::app {

ApplicationController::ApplicationController()
    : config_(std::make_unique<config::ConfigManager>(config::ConfigManager::defaultConfigPath())),
      scheduler_(std::make_unique<scheduler::ReminderScheduler>()),
      idle_detector_(std::make_unique<idle::IdleDetector>()),
      brightness_controller_(std::make_shared<brightness::BrightnessController>()),
      command_executor_(std::make_unique<commands::CommandExecutor>()),
      notifications_(std::make_unique<notifications::NotificationManager>()),
      strict_window_(std::make_unique<strict::StrictBreakWindow>(brightness_controller_)),
      stats_(std::make_unique<stats::StatsManager>(stats::StatsManager::defaultStatsDirectory())),
      suppression_manager_(std::make_unique<suppression::SuppressionManager>(config_.get())),
      dashboard_(std::make_unique<dashboard::DashboardWindow>(stats_.get(), suppression_manager_.get())),
      tray_(std::make_unique<tray::SystemTrayManager>()),
      update_timer_(new QTimer(this)) {
      
    setupConnections();
}

ApplicationController::~ApplicationController() {
    shutdown();
}

void ApplicationController::startup() {
    synchronizeReminders();
    scheduler_->start();
    idle_detector_->start();
    
    update_timer_->start(60000); // 1 minute
    handleReminders(); // Update tray
}

void ApplicationController::shutdown() {
    scheduler_->stop();
    idle_detector_->stop();
    update_timer_->stop();
    strict_window_->close();
}

void ApplicationController::pause(std::chrono::seconds duration) {
    scheduler_->pauseAll(duration);
    tray_->showMessage("Paused", QString("Health reminders paused for %1 minutes.").arg(duration.count() / 60));
}

void ApplicationController::resume() {
    scheduler_->resumeAll();
    tray_->showMessage("Resumed", "Health reminders resumed.");
}

void ApplicationController::reloadConfig() {
    try {
        config_->reload();
        synchronizeReminders();
        tray_->showMessage("Config Reloaded", "Configuration updated successfully.");
    } catch (const std::exception& e) {
        tray_->showMessage("Config Error", e.what());
    }
}

void ApplicationController::setMediaMode(bool active) {
    suppression_manager_->setMediaMode(active);
    tray_->showMessage("Media Mode", active ? "Media mode enabled. Reminders suppressed." : "Media mode disabled.");
    dashboard_->refreshData();
}

void ApplicationController::toggleMediaMode() {
    setMediaMode(!suppression_manager_->isMediaModeEnabled());
}

void ApplicationController::showDashboard() {
    dashboard_->refreshData();
    dashboard_->show();
    dashboard_->raise();
    dashboard_->activateWindow();
}

void ApplicationController::setupConnections() {
    scheduler_->setCallbacks({
        .reminder_triggered = [this](const auto& e) { QMetaObject::invokeMethod(this, [this, e](){ onReminderTriggered(e); }); },
        .reminder_completed = [this](const auto& e) { QMetaObject::invokeMethod(this, [this, e](){ onReminderCompleted(e); }); },
        .reminder_skipped = [this](const auto& e) { QMetaObject::invokeMethod(this, [this, e](){ onReminderSkipped(e); }); }
    });

    notifications_->setActionCallback([this](std::string id, notifications::NotificationAction action) {
        if (action == notifications::NotificationAction::Done) {
            (void)scheduler_->completeReminder(id);
        } else if (action == notifications::NotificationAction::Snooze5m) {
            (void)scheduler_->snoozeReminder(id, std::chrono::minutes(5));
        } else if (action == notifications::NotificationAction::Snooze10m) {
            (void)scheduler_->snoozeReminder(id, std::chrono::minutes(10));
        } else if (action == notifications::NotificationAction::Skip) {
            (void)scheduler_->skipReminder(id);
        }
    });

    strict_window_->setFinishedCallback([this]() {
        (void)scheduler_->completeReminder("eye_break");
    });
    strict_window_->setSkippedCallback([this]() {
        (void)scheduler_->skipReminder("eye_break");
    });

    tray_->setPause30mCallback([this]() { pause(std::chrono::minutes(30)); });
    tray_->setPause1hCallback([this]() { pause(std::chrono::hours(1)); });
    tray_->setResumeCallback([this]() { resume(); });
    tray_->setOpenDashboardCallback([this]() { showDashboard(); });
    tray_->setToggleMediaModeCallback([this]() { toggleMediaMode(); });
    tray_->setReloadConfigCallback([this]() { reloadConfig(); });
    tray_->setQuitCallback([]() { qApp->quit(); });

    connect(update_timer_, &QTimer::timeout, this, &ApplicationController::handleReminders);
}

void ApplicationController::synchronizeReminders() {
    scheduler_->clearReminders();

    const auto eye_config = config_->getEyeBreakConfig();
    if (eye_config.enabled) {
        scheduler_->addOrUpdateReminder({
            .id = "eye_break",
            .name = "Eye Break",
            .message = "Look 20 feet away for 20 seconds.",
            .schedule = scheduler::ReminderSchedule::intervalEvery(eye_config.interval),
            .enabled = true
        });
    }

    const auto water_config = config_->getWaterConfig();
    if (water_config.enabled) {
        scheduler_->addOrUpdateReminder({
            .id = "water",
            .name = "Drink Water",
            .message = "Time to drink a glass of water.",
            .schedule = scheduler::ReminderSchedule::intervalEvery(water_config.interval),
            .enabled = true
        });
    }

    int i = 0;
    for (const auto& custom : config_->getCustomReminders()) {
        scheduler::ReminderSchedule sched;
        if (custom.schedule_type == config::ReminderScheduleType::Interval && custom.interval.has_value()) {
            sched = scheduler::ReminderSchedule::intervalEvery(custom.interval.value());
        } else if (custom.schedule_type == config::ReminderScheduleType::Daily && custom.at.has_value()) {
            sched = scheduler::ReminderSchedule::dailyAt({custom.at->hour, custom.at->minute});
        }
        
        scheduler_->addOrUpdateReminder({
            .id = "custom_" + std::to_string(i++),
            .name = custom.name,
            .message = custom.message,
            .schedule = sched,
            .enabled = true
        });
    }
}

void ApplicationController::onReminderTriggered(const scheduler::ReminderEvent& event) {
    if (suppression_manager_->shouldSuppress(event.id)) {
        (void)scheduler_->snoozeReminder(event.id, std::chrono::minutes(5));
        return;
    }
    if (idle_detector_->isIdle()) {
        (void)scheduler_->snoozeReminder(event.id, std::chrono::minutes(5));
        return;
    }

    if (event.id == "eye_break") {
        stats_->recordEyeBreakShown();
        auto conf = config_->getEyeBreakConfig();
        if (conf.strict_mode) {
            strict_window_->showBreak(conf.duration, conf.skip_unlock, 20);
        } else {
            notifications::NotificationRequest req {
                .reminder_id = event.id,
                .title = event.name,
                .message = event.message,
                .sound = conf.sound,
                .skip_unlock = conf.skip_unlock
            };
            notifications_->showNotification(req);
            auto* win = new notifications::NotificationWindow(notifications_->activeNotifications().back(), notifications_.get());
            win->show();
        }
    } else if (event.id == "water") {
        stats_->recordWaterShown();
        auto conf = config_->getWaterConfig();
        notifications::NotificationRequest req {
            .reminder_id = event.id,
            .title = event.name,
            .message = event.message,
            .sound = conf.sound,
            .skip_unlock = std::chrono::seconds(0)
        };
        notifications_->showNotification(req);
        auto* win = new notifications::NotificationWindow(notifications_->activeNotifications().back(), notifications_.get());
        win->show();
    } else {
        notifications::NotificationRequest req {
            .reminder_id = event.id,
            .title = event.name,
            .message = event.message,
            .sound = true,
            .skip_unlock = std::chrono::seconds(0)
        };
        notifications_->showNotification(req);
        auto* win = new notifications::NotificationWindow(notifications_->activeNotifications().back(), notifications_.get());
        win->show();
    }
}

void ApplicationController::onReminderCompleted(const scheduler::ReminderEvent& event) {
    if (event.id == "eye_break") stats_->recordEyeBreakCompleted();
    else if (event.id == "water") stats_->recordWaterCompleted();
    notifications_->closeNotification(event.id);
    handleReminders();
}

void ApplicationController::onReminderSkipped(const scheduler::ReminderEvent& event) {
    if (event.id == "eye_break") stats_->recordEyeBreakSkipped();
    else if (event.id == "water") stats_->recordWaterSkipped();
    notifications_->closeNotification(event.id);
    handleReminders();
}

void ApplicationController::handleReminders() {
    QString eye_break_str = "--";
    QString water_str = "--";

    auto next_eye = scheduler_->nextTriggerTime("eye_break");
    if (next_eye) {
        auto diff = std::chrono::duration_cast<std::chrono::minutes>(*next_eye - std::chrono::system_clock::now()).count();
        eye_break_str = QString::number(std::max(0L, diff)) + "m";
    }

    auto next_water = scheduler_->nextTriggerTime("water");
    if (next_water) {
        auto diff = std::chrono::duration_cast<std::chrono::minutes>(*next_water - std::chrono::system_clock::now()).count();
        water_str = QString::number(std::max(0L, diff)) + "m";
    }

    tray_->updateTimes(eye_break_str, water_str);
    stats_->addActiveScreenTime(std::chrono::minutes(1));
}

}  // namespace sandhika::app
