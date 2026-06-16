#include "sandhika/dashboard/dashboard_window.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QPieSeries>
#include <QDateTime>

namespace sandhika::dashboard {

class DashboardWindow::OverviewTab : public QWidget {
public:
    explicit OverviewTab(QWidget* parent = nullptr) : QWidget(parent) {
        auto* layout = new QVBoxLayout(this);
        score_label = new QLabel("Health Score: --", this);
        score_label->setStyleSheet("font-size: 32px; font-weight: bold; color: #a6e3a1;");
        score_label->setAlignment(Qt::AlignCenter);
        layout->addWidget(score_label);
        
        media_mode_label = new QLabel("Media Mode: --", this);
        media_mode_label->setStyleSheet("font-size: 20px; font-weight: bold; color: #f9e2af;");
        media_mode_label->setAlignment(Qt::AlignCenter);
        layout->addWidget(media_mode_label);

        details_label = new QLabel("Active Screen Time: --\nEye Breaks: --\nWater: --", this);
        details_label->setStyleSheet("font-size: 18px; color: #cdd6f4;");
        details_label->setAlignment(Qt::AlignCenter);
        layout->addWidget(details_label);
    }

    void update(const stats::DailyStats& stats, int score, bool media_mode) {
        score_label->setText(QString("Health Score: %1").arg(score));
        media_mode_label->setText(QString("Media Mode: %1").arg(media_mode ? "ON" : "OFF"));
        details_label->setText(QString("Active Screen Time: %1m\nEye Breaks: %2/%3\nWater: %4/%5")
            .arg(stats.screen_time.active_minutes)
            .arg(stats.eye_break.completed).arg(stats.eye_break.shown)
            .arg(stats.water.completed).arg(stats.water.shown));
    }

private:
    QLabel* score_label;
    QLabel* media_mode_label;
    QLabel* details_label;
};

class DashboardWindow::StatisticsTab : public QWidget {
public:
    explicit StatisticsTab(QWidget* parent = nullptr) : QWidget(parent) {
        layout = new QVBoxLayout(this);
        chart_view = new QChartView(this);
        chart_view->setRenderHint(QPainter::Antialiasing);
        layout->addWidget(chart_view);
    }

    void update(const std::vector<stats::DailyStats>& last_days) {
        auto* chart = new QChart();
        chart->setTitle("Last 30 Days Compliance");
        chart->setTheme(QChart::ChartThemeDark);

        auto* eye_series = new QLineSeries();
        eye_series->setName("Eye Breaks Completed");
        auto* water_series = new QLineSeries();
        water_series->setName("Water Reminders Completed");

        int max_val = 10;
        int i = 0;
        for (auto it = last_days.rbegin(); it != last_days.rend(); ++it) {
            eye_series->append(i, it->eye_break.completed);
            water_series->append(i, it->water.completed);
            max_val = std::max({max_val, it->eye_break.completed, it->water.completed});
            ++i;
        }

        chart->addSeries(eye_series);
        chart->addSeries(water_series);
        chart->createDefaultAxes();
        
        auto axes = chart->axes(Qt::Vertical);
        if (!axes.isEmpty()) {
            auto* axisY = qobject_cast<QValueAxis*>(axes.first());
            if (axisY) {
                axisY->setRange(0, max_val + 5);
            }
        }

        chart_view->setChart(chart);
    }

private:
    QVBoxLayout* layout;
    QChartView* chart_view;
};

class DashboardWindow::TrendsTab : public QWidget {
public:
    explicit TrendsTab(QWidget* parent = nullptr) : QWidget(parent) {
        layout = new QVBoxLayout(this);
        chart_view = new QChartView(this);
        chart_view->setRenderHint(QPainter::Antialiasing);
        layout->addWidget(chart_view);
    }

    void update(const std::vector<stats::DailyStats>& last_days) {
        auto* chart = new QChart();
        chart->setTitle("Screen Time Trend");
        chart->setTheme(QChart::ChartThemeDark);

        auto* series = new QBarSeries();
        auto* set = new QBarSet("Screen Time (minutes)");

        int max_val = 100;
        QStringList categories;
        for (auto it = last_days.rbegin(); it != last_days.rend(); ++it) {
            *set << it->screen_time.active_minutes;
            max_val = std::max(max_val, it->screen_time.active_minutes);
            categories << QString::number(static_cast<unsigned>(it->date.day()));
        }
        series->append(set);
        chart->addSeries(series);

        auto* axisX = new QBarCategoryAxis();
        axisX->append(categories);
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);

        auto* axisY = new QValueAxis();
        axisY->setRange(0, max_val + 60);
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);

        chart_view->setChart(chart);
    }

private:
    QVBoxLayout* layout;
    QChartView* chart_view;
};

class DashboardWindow::HealthScoreTab : public QWidget {
public:
    explicit HealthScoreTab(QWidget* parent = nullptr) : QWidget(parent) {
        layout = new QVBoxLayout(this);
        chart_view = new QChartView(this);
        chart_view->setRenderHint(QPainter::Antialiasing);
        layout->addWidget(chart_view);
    }

    void update(int score) {
        auto* chart = new QChart();
        chart->setTitle("Overall Health Score");
        chart->setTheme(QChart::ChartThemeDark);

        auto* series = new QPieSeries();
        series->append("Score", score);
        series->append("Remaining", 100 - score);
        
        auto slice = series->slices().at(0);
        slice->setColor(QColor("#a6e3a1"));
        slice->setLabelVisible(true);
        
        auto rem = series->slices().at(1);
        rem->setColor(QColor("#313244"));

        chart->addSeries(series);
        chart_view->setChart(chart);
    }

private:
    QVBoxLayout* layout;
    QChartView* chart_view;
};

DashboardWindow::DashboardWindow(stats::StatsManager* stats_manager, suppression::SuppressionManager* suppression_manager, QWidget* parent)
    : QWidget(parent), stats_manager_(stats_manager), suppression_manager_(suppression_manager) {
    setWindowTitle("Sandhika Dashboard");
    resize(800, 600);
    applyDarkTheme();
    setupUi();
    refreshData();
}

DashboardWindow::~DashboardWindow() = default;

void DashboardWindow::applyDarkTheme() {
    setStyleSheet("QWidget { background-color: #1e1e2e; color: #cdd6f4; }"
                  "QTabWidget::pane { border: 1px solid #313244; border-radius: 4px; }"
                  "QTabBar::tab { background: #313244; color: #cdd6f4; padding: 8px 16px; border: 1px solid #1e1e2e; }"
                  "QTabBar::tab:selected { background: #45475a; font-weight: bold; border-bottom-color: #a6e3a1; border-bottom-width: 2px; }");
}

void DashboardWindow::setupUi() {
    auto* main_layout = new QVBoxLayout(this);
    tabs_ = new QTabWidget(this);

    overview_tab_ = new OverviewTab(this);
    statistics_tab_ = new StatisticsTab(this);
    trends_tab_ = new TrendsTab(this);
    health_score_tab_ = new HealthScoreTab(this);

    tabs_->addTab(overview_tab_, "Overview");
    tabs_->addTab(statistics_tab_, "Statistics");
    tabs_->addTab(trends_tab_, "Trends");
    tabs_->addTab(health_score_tab_, "Health Score");

    main_layout->addWidget(tabs_);
}

void DashboardWindow::refreshData() {
    auto today = stats_manager_->getTodayStats();
    int score = stats::StatsManager::calculateHealthScore(today);
    auto last_30 = stats_manager_->getLastDays(30);

    overview_tab_->update(today, score, suppression_manager_->isMediaModeEnabled());
    statistics_tab_->update(last_30);
    trends_tab_->update(last_30);
    health_score_tab_->update(score);
}

}  // namespace sandhika::dashboard
