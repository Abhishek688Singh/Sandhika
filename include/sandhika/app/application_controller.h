#pragma once

#include "sandhika/config/config_manager.h"
#include "sandhika/scheduler/reminder_scheduler.h"
#include "sandhika/idle/idle_detector.h"

#include "sandhika/brightness/brightness_controller.h"
#include "sandhika/commands/command_executor.h"
#include "sandhika/notifications/notification_manager.h"
#include "sandhika/strict/strict_break_window.h"
#include "sandhika/stats/stats_manager.h"
#include "sandhika/dashboard/dashboard_window.h"
#include "sandhika/tray/system_tray_manager.h"
#include "sandhika/suppression/suppression_manager.h"

#include <QObject>
#include <QTimer>
#include <memory>
#include <chrono>

namespace sandhika::app {

class ApplicationController : public QObject {
    Q_OBJECT

public:
    ApplicationController();
    ~ApplicationController() override;

    void startup();
    void shutdown();
    void pause(std::chrono::seconds duration);
    void resume();
    void reloadConfig();
    void showDashboard();
    void setMediaMode(bool active);
    void toggleMediaMode();

private slots:
    void handleReminders();

private:
    void setupConnections();
    void synchronizeReminders();
    
    // Callbacks from scheduler
    void onReminderTriggered(const scheduler::ReminderEvent& event);
    void onReminderCompleted(const scheduler::ReminderEvent& event);
    void onReminderSkipped(const scheduler::ReminderEvent& event);

    std::unique_ptr<config::ConfigManager> config_;
    std::unique_ptr<scheduler::ReminderScheduler> scheduler_;
    std::unique_ptr<idle::IdleDetector> idle_detector_;
    std::shared_ptr<brightness::BrightnessController> brightness_controller_;
    std::unique_ptr<commands::CommandExecutor> command_executor_;
    std::unique_ptr<notifications::NotificationManager> notifications_;
    std::unique_ptr<strict::StrictBreakWindow> strict_window_;
    std::unique_ptr<stats::StatsManager> stats_;
    std::unique_ptr<suppression::SuppressionManager> suppression_manager_;
    std::unique_ptr<dashboard::DashboardWindow> dashboard_;
    std::unique_ptr<tray::SystemTrayManager> tray_;
    
    QTimer* update_timer_;
};

}  // namespace sandhika::app
