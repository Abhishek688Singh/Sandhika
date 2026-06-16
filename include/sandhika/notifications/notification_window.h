#pragma once

#include "sandhika/notifications/notification_manager.h"

#include <QDialog>
#include <QTimer>
#include <QLabel>
#include <QPushButton>

namespace sandhika::notifications {

class NotificationWindow : public QDialog {
    Q_OBJECT

public:
    explicit NotificationWindow(const ActiveNotification& active,
                                NotificationManager* manager,
                                QWidget* parent = nullptr);
    ~NotificationWindow() override;

private slots:
    void updateSkipButton();
    void onDone();
    void onSnooze5m();
    void onSnooze10m();
    void onSkip();

private:
    void setupUi();
    void playSound();
    void executeAction();

    ActiveNotification active_;
    NotificationManager* manager_;
    QTimer* skip_timer_;
    QPushButton* skip_button_;
};

}  // namespace sandhika::notifications
