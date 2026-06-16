#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QString>
#include <functional>

namespace health_reminder::tray {

class SystemTrayManager : public QObject {
    Q_OBJECT

public:
    explicit SystemTrayManager(QObject* parent = nullptr);
    ~SystemTrayManager() override;

    void setPause30mCallback(std::function<void()> cb);
    void setPause1hCallback(std::function<void()> cb);
    void setResumeCallback(std::function<void()> cb);
    void setOpenDashboardCallback(std::function<void()> cb);
    void setReloadConfigCallback(std::function<void()> cb);
    void setQuitCallback(std::function<void()> cb);

    void updateTimes(const QString& eye_break, const QString& water);
    void showMessage(const QString& title, const QString& message);

private:
    void setupMenu();

    QSystemTrayIcon* tray_icon_;
    QMenu* tray_menu_;
    
    QAction* eye_break_action_;
    QAction* water_action_;
    
    std::function<void()> pause_30m_cb_;
    std::function<void()> pause_1h_cb_;
    std::function<void()> resume_cb_;
    std::function<void()> open_dashboard_cb_;
    std::function<void()> reload_config_cb_;
    std::function<void()> quit_cb_;
};

}  // namespace health_reminder::tray
