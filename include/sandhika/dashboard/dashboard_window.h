#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QLabel>
#include "sandhika/stats/stats_manager.h"
#include "sandhika/suppression/suppression_manager.h"

namespace sandhika::dashboard {

class DashboardWindow : public QWidget {
    Q_OBJECT

public:
    explicit DashboardWindow(stats::StatsManager* stats_manager, suppression::SuppressionManager* suppression_manager, QWidget* parent = nullptr);
    ~DashboardWindow() override;

    void refreshData();

private:
    void setupUi();
    void setupOverviewTab();
    void setupStatisticsTab();
    void setupTrendsTab();
    void setupHealthScoreTab();
    void applyDarkTheme();

    stats::StatsManager* stats_manager_;
    suppression::SuppressionManager* suppression_manager_;
    QTabWidget* tabs_;
    QLabel* media_mode_label_;

    // UI elements to update
    class OverviewTab;
    class StatisticsTab;
    class TrendsTab;
    class HealthScoreTab;

    OverviewTab* overview_tab_;
    StatisticsTab* statistics_tab_;
    TrendsTab* trends_tab_;
    HealthScoreTab* health_score_tab_;
};

}  // namespace sandhika::dashboard
