#include "health_reminder/notifications/notification_manager.h"
#include "health_reminder/strict/strict_break_window.h"
#include "health_reminder/dashboard/dashboard_window.h"
#include "health_reminder/stats/stats_manager.h"

#include <QApplication>
#include <QTimer>
#include <cassert>
#include <iostream>

using namespace health_reminder;

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Test NotificationManager
    notifications::NotificationManager nm;
    nm.showNotification({.reminder_id = "test", .title = "Title", .message = "Msg"});
    assert(nm.activeNotifications().size() == 1);
    nm.closeAll();
    assert(nm.activeNotifications().empty());

    // Test Dashboard (Instantiate and delete)
    stats::StatsManager sm("/tmp/stats");
    dashboard::DashboardWindow dw(&sm);
    dw.refreshData();
    dw.hide();

    // Test StrictBreakWindow
    strict::StrictBreakWindow sw(nullptr);
    sw.showBreak(std::chrono::seconds(1), std::chrono::seconds(0), 0);
    sw.requestSkip();
    sw.close();

    std::cout << "GUI tests passed\n";
    return 0;
}
