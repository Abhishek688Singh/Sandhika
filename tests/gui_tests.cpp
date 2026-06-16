#include "sandhika/notifications/notification_manager.h"
#include "sandhika/strict/strict_break_window.h"
#include "sandhika/dashboard/dashboard_window.h"
#include "sandhika/stats/stats_manager.h"
#include "sandhika/config/config_manager.h"
#include "sandhika/suppression/suppression_manager.h"

#include <QApplication>
#include <QTimer>
#include <cassert>
#include <iostream>

using namespace sandhika;

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Test NotificationManager
    notifications::NotificationManager nm;
    nm.showNotification({.reminder_id = "test", .title = "Title", .message = "Msg"});
    assert(nm.activeNotifications().size() == 1);
    nm.closeAll();
    assert(nm.activeNotifications().empty());

    // Test Dashboard (Instantiate and delete)
    config::ConfigManager cm(config::ConfigManager::defaultConfigPath());
    suppression::SuppressionManager supp(&cm);
    stats::StatsManager sm("/tmp/stats");
    dashboard::DashboardWindow dw(&sm, &supp);
    dw.refreshData();
    dw.hide();

    // Test StrictBreakWindow
    strict::StrictBreakWindow sw(nullptr);
    sw.showBreak(std::chrono::seconds(1), std::chrono::seconds(0), 0);
    (void)sw.requestSkip();
    sw.close();

    std::cout << "GUI tests passed\n";
    return 0;
}
