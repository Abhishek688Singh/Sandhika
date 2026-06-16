#include "sandhika/strict/strict_break_window.h"
#include "strict_break_dialog.h"

#include <QApplication>
#include <QKeyEvent>
#include <QMetaObject>
#include <QVBoxLayout>
#include <QScreen>
#include <QTimer>

namespace sandhika::strict {

StrictBreakDialog::StrictBreakDialog(QWidget* parent)
    : QDialog(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::MaximizeUsingFullscreenGeometryHint) {
    
    // Multi-monitor fullscreen
    auto screens = QGuiApplication::screens();
    QRect totalRect;
    for (auto screen : screens) {
        totalRect = totalRect.united(screen->geometry());
    }
    setGeometry(totalRect);
    
    setStyleSheet("QDialog { background-color: #11111b; }"
                  "QLabel { color: #cdd6f4; font-family: sans-serif; }"
                  "QPushButton { background-color: #313244; color: #cdd6f4; border: none; padding: 12px 24px; border-radius: 8px; font-size: 18px; font-weight: bold; }"
                  "QPushButton:hover { background-color: #45475a; }"
                  "QPushButton:disabled { background-color: #181825; color: #585b70; }");

    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    auto* title_label = new QLabel("Eye Break", this);
    title_label->setStyleSheet("font-size: 48px; font-weight: bold; color: #89b4fa;");
    title_label->setAlignment(Qt::AlignCenter);

    time_label_ = new QLabel("", this);
    time_label_->setStyleSheet("font-size: 120px; font-weight: bold;");
    time_label_->setAlignment(Qt::AlignCenter);

    skip_button_ = new QPushButton("Skip", this);
    skip_button_->setEnabled(false);
    skip_button_->setFixedSize(200, 60);
    connect(skip_button_, &QPushButton::clicked, this, &StrictBreakDialog::skipRequested);

    layout->addSpacing(100);
    layout->addWidget(title_label);
    layout->addSpacing(40);
    layout->addWidget(time_label_);
    layout->addSpacing(80);
    layout->addWidget(skip_button_, 0, Qt::AlignCenter);
    layout->addStretch();
}

StrictBreakDialog::~StrictBreakDialog() = default;

void StrictBreakDialog::updateState(int remaining_seconds, bool skip_enabled) {
    time_label_->setText(QString::number(remaining_seconds));
    skip_button_->setEnabled(skip_enabled);
}

void StrictBreakDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        event->ignore(); // Disable ESC
        return;
    }
    if (event->key() == Qt::Key_F4 && (event->modifiers() & Qt::AltModifier)) {
        event->ignore(); // Disable Alt+F4
        return;
    }
    QDialog::keyPressEvent(event);
}

void StrictBreakDialog::closeEvent(QCloseEvent* event) {
    event->ignore(); // Disable Close button
}

// -----------------------------------------------------------------------------

StrictBreakWindow::StrictBreakWindow(std::shared_ptr<brightness::BrightnessController> brightness_controller)
    : QObject(nullptr), brightness_controller_(std::move(brightness_controller)) {}

StrictBreakWindow::~StrictBreakWindow() {
    close();
}

void StrictBreakWindow::setFinishedCallback(FinishedCallback callback) {
    std::lock_guard lock(mutex_);
    finished_callback_ = std::move(callback);
}

void StrictBreakWindow::setSkippedCallback(SkippedCallback callback) {
    std::lock_guard lock(mutex_);
    skipped_callback_ = std::move(callback);
}

void StrictBreakWindow::showBreak(std::chrono::seconds duration, std::chrono::seconds skip_unlock, int dim_percent) {
    std::lock_guard lock(mutex_);
    if (state_.active) {
        return;
    }
    
    state_.active = true;
    state_.skip_enabled = (skip_unlock <= std::chrono::seconds::zero());
    state_.remaining = duration;

    if (brightness_controller_) {
        brightness_controller_->dimTo(dim_percent);
    }

    QMetaObject::invokeMethod(qApp, [this]() {
        if (!dialog_) {
            dialog_ = new StrictBreakDialog();
            connect(dialog_, &StrictBreakDialog::skipRequested, this, [this]() { requestSkip(); });
        }
        dialog_->updateState(state_.remaining.count(), state_.skip_enabled);
        dialog_->showFullScreen();
    }, Qt::QueuedConnection);

    worker_ = std::jthread([this, duration, skip_unlock](std::stop_token token) {
        run(token, duration, skip_unlock);
    });
}

bool StrictBreakWindow::requestSkip() {
    std::lock_guard lock(mutex_);
    if (!state_.active || !state_.skip_enabled) {
        return false;
    }
    
    worker_.request_stop();
    condition_.notify_all();
    finish(true);
    return true;
}

void StrictBreakWindow::close() {
    std::unique_lock lock(mutex_);
    if (!state_.active) {
        return;
    }
    worker_.request_stop();
    condition_.notify_all();
    lock.unlock();
    if (worker_.joinable()) {
        worker_.join();
    }
    QMetaObject::invokeMethod(qApp, [this]() {
        if (dialog_) {
            dialog_->hide();
            dialog_->deleteLater();
            dialog_ = nullptr;
        }
    });
}

StrictBreakState StrictBreakWindow::state() const {
    std::lock_guard lock(mutex_);
    return state_;
}

void StrictBreakWindow::run(std::stop_token stop_token, std::chrono::seconds duration, std::chrono::seconds skip_unlock) {
    const auto start = std::chrono::steady_clock::now();
    
    std::unique_lock lock(mutex_);
    while (!stop_token.stop_requested()) {
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start);
        
        state_.remaining = duration - elapsed;
        state_.skip_enabled = (elapsed >= skip_unlock);
        
        if (state_.remaining <= std::chrono::seconds::zero()) {
            state_.remaining = std::chrono::seconds::zero();
            break;
        }
        
        updateUI(state_.remaining.count(), state_.skip_enabled);
        condition_.wait_for(lock, std::chrono::seconds(1));
    }
    
    if (!stop_token.stop_requested()) {
        finish(false);
    }
}

void StrictBreakWindow::updateUI(int remaining, bool skip_enabled) {
    QMetaObject::invokeMethod(qApp, [this, remaining, skip_enabled]() {
        if (dialog_) {
            dialog_->updateState(remaining, skip_enabled);
        }
    });
}

void StrictBreakWindow::finish(bool skipped) {
    state_.active = false;
    
    if (brightness_controller_) {
        brightness_controller_->restore();
    }
    
    QMetaObject::invokeMethod(qApp, [this]() {
        if (dialog_) {
            dialog_->hide();
            dialog_->deleteLater();
            dialog_ = nullptr;
        }
    });
    
    auto fin_cb = finished_callback_;
    auto skip_cb = skipped_callback_;
    
    if (skipped && skip_cb) {
        skip_cb();
    } else if (!skipped && fin_cb) {
        fin_cb();
    }
}

}  // namespace sandhika::strict
