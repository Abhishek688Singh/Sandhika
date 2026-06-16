#include "sandhika/notifications/notification_window.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QProcess>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

namespace sandhika::notifications {

NotificationWindow::NotificationWindow(const ActiveNotification& active,
                                       NotificationManager* manager,
                                       QWidget* parent)
    : QDialog(parent, Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Tool),
      active_(active),
      manager_(manager),
      skip_timer_(new QTimer(this)),
      skip_button_(nullptr) {
    setupUi();
    playSound();
    executeAction();

    if (active_.request.skip_unlock > std::chrono::seconds::zero()) {
        skip_timer_->start(1000);
        connect(skip_timer_, &QTimer::timeout, this, &NotificationWindow::updateSkipButton);
        updateSkipButton();
    } else {
        skip_button_->setEnabled(true);
        skip_button_->setText("Skip");
    }

    // Fade-in animation
    auto* opacity = new QGraphicsOpacityEffect(this);
    this->setGraphicsEffect(opacity);
    auto* anim = new QPropertyAnimation(opacity, "opacity", this);
    anim->setDuration(500);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

NotificationWindow::~NotificationWindow() = default;

void NotificationWindow::setupUi() {
    this->setStyleSheet("QDialog { background-color: #1e1e2e; border-radius: 12px; border: 1px solid #313244; }"
                        "QLabel { color: #cdd6f4; }"
                        "QPushButton { background-color: #45475a; color: #cdd6f4; border: none; padding: 8px 16px; border-radius: 6px; font-weight: bold; }"
                        "QPushButton:hover { background-color: #585b70; }"
                        "QPushButton:pressed { background-color: #313244; }"
                        "QPushButton:disabled { background-color: #313244; color: #6c7086; }");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(16);

    auto* title_label = new QLabel(QString::fromStdString(active_.request.title), this);
    title_label->setStyleSheet("font-size: 18px; font-weight: bold; color: #89b4fa;");
    layout->addWidget(title_label);

    auto* message_label = new QLabel(QString::fromStdString(active_.request.message), this);
    message_label->setStyleSheet("font-size: 14px;");
    message_label->setWordWrap(true);
    layout->addWidget(message_label);

    auto* buttons_layout = new QHBoxLayout();
    buttons_layout->setSpacing(8);

    auto* done_btn = new QPushButton("Done", this);
    done_btn->setStyleSheet("background-color: #a6e3a1; color: #11111b;");
    connect(done_btn, &QPushButton::clicked, this, &NotificationWindow::onDone);

    auto* snooze5_btn = new QPushButton("Snooze 5m", this);
    connect(snooze5_btn, &QPushButton::clicked, this, &NotificationWindow::onSnooze5m);

    auto* snooze10_btn = new QPushButton("Snooze 10m", this);
    connect(snooze10_btn, &QPushButton::clicked, this, &NotificationWindow::onSnooze10m);

    skip_button_ = new QPushButton("Skip", this);
    skip_button_->setEnabled(false);
    skip_button_->setStyleSheet("background-color: #f38ba8; color: #11111b;");
    connect(skip_button_, &QPushButton::clicked, this, &NotificationWindow::onSkip);

    buttons_layout->addWidget(done_btn);
    buttons_layout->addWidget(snooze5_btn);
    buttons_layout->addWidget(snooze10_btn);
    buttons_layout->addWidget(skip_button_);

    layout->addLayout(buttons_layout);
}

void NotificationWindow::updateSkipButton() {
    const auto elapsed = std::chrono::steady_clock::now() - active_.shown_at;
    const auto remaining = active_.request.skip_unlock - elapsed;
    if (remaining <= std::chrono::seconds::zero()) {
        skip_button_->setEnabled(true);
        skip_button_->setText("Skip");
        skip_timer_->stop();
    } else {
        const auto secs = std::chrono::duration_cast<std::chrono::seconds>(remaining).count();
        skip_button_->setText(QString("Skip (%1s)").arg(secs));
    }
}

void NotificationWindow::playSound() {
    if (!active_.request.sound) return;
    QApplication::beep();
}

void NotificationWindow::executeAction() {
    if (active_.request.command) {
        QProcess::startDetached(QString::fromStdString(*active_.request.command), {});
    }
}

void NotificationWindow::onDone() {
    if (manager_->activateAction(active_.request.reminder_id, NotificationAction::Done)) {
        accept();
    }
}

void NotificationWindow::onSnooze5m() {
    if (manager_->activateAction(active_.request.reminder_id, NotificationAction::Snooze5m)) {
        accept();
    }
}

void NotificationWindow::onSnooze10m() {
    if (manager_->activateAction(active_.request.reminder_id, NotificationAction::Snooze10m)) {
        accept();
    }
}

void NotificationWindow::onSkip() {
    if (manager_->activateAction(active_.request.reminder_id, NotificationAction::Skip)) {
        accept();
    }
}

}  // namespace sandhika::notifications
