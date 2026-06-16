#include "health_reminder/tray/system_tray_manager.h"

#include <QIcon>
#include <QApplication>

namespace health_reminder::tray {

SystemTrayManager::SystemTrayManager(QObject* parent)
    : QObject(parent),
      tray_icon_(new QSystemTrayIcon(this)),
      tray_menu_(new QMenu()) {
    
    tray_icon_->setIcon(QIcon::fromTheme("health-reminder", QIcon(":/resources/icons/health-reminder.png")));
    tray_icon_->setToolTip("Health Reminder");
    
    setupMenu();
    tray_icon_->setContextMenu(tray_menu_);
    tray_icon_->show();
}

SystemTrayManager::~SystemTrayManager() {
    tray_icon_->hide();
}

void SystemTrayManager::setPause30mCallback(std::function<void()> cb) { pause_30m_cb_ = std::move(cb); }
void SystemTrayManager::setPause1hCallback(std::function<void()> cb) { pause_1h_cb_ = std::move(cb); }
void SystemTrayManager::setResumeCallback(std::function<void()> cb) { resume_cb_ = std::move(cb); }
void SystemTrayManager::setOpenDashboardCallback(std::function<void()> cb) { open_dashboard_cb_ = std::move(cb); }
void SystemTrayManager::setReloadConfigCallback(std::function<void()> cb) { reload_config_cb_ = std::move(cb); }
void SystemTrayManager::setQuitCallback(std::function<void()> cb) { quit_cb_ = std::move(cb); }

void SystemTrayManager::updateTimes(const QString& eye_break, const QString& water) {
    eye_break_action_->setText("Eye Break: " + eye_break);
    water_action_->setText("Water: " + water);
    tray_icon_->setToolTip(QString("Health Reminder\nEye Break: %1\nWater: %2").arg(eye_break, water));
}

void SystemTrayManager::showMessage(const QString& title, const QString& message) {
    tray_icon_->showMessage(title, message, QSystemTrayIcon::Information, 5000);
}

void SystemTrayManager::setupMenu() {
    eye_break_action_ = tray_menu_->addAction("Eye Break: --");
    eye_break_action_->setEnabled(false);
    
    water_action_ = tray_menu_->addAction("Water: --");
    water_action_->setEnabled(false);
    
    tray_menu_->addSeparator();
    
    auto* pause_30m = tray_menu_->addAction("Pause 30m");
    connect(pause_30m, &QAction::triggered, this, [this]() { if (pause_30m_cb_) pause_30m_cb_(); });
    
    auto* pause_1h = tray_menu_->addAction("Pause 1h");
    connect(pause_1h, &QAction::triggered, this, [this]() { if (pause_1h_cb_) pause_1h_cb_(); });
    
    auto* resume = tray_menu_->addAction("Resume");
    connect(resume, &QAction::triggered, this, [this]() { if (resume_cb_) resume_cb_(); });
    
    tray_menu_->addSeparator();
    
    auto* dashboard = tray_menu_->addAction("Open Dashboard");
    connect(dashboard, &QAction::triggered, this, [this]() { if (open_dashboard_cb_) open_dashboard_cb_(); });
    
    auto* reload = tray_menu_->addAction("Reload Config");
    connect(reload, &QAction::triggered, this, [this]() { if (reload_config_cb_) reload_config_cb_(); });
    
    tray_menu_->addSeparator();
    
    auto* quit = tray_menu_->addAction("Quit");
    connect(quit, &QAction::triggered, this, [this]() { if (quit_cb_) quit_cb_(); });
}

}  // namespace health_reminder::tray
