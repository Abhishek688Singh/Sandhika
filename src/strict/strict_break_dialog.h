#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>

namespace health_reminder::strict {

class StrictBreakDialog : public QDialog {
    Q_OBJECT

public:
    explicit StrictBreakDialog(QWidget* parent = nullptr);
    ~StrictBreakDialog() override;

    void updateState(int remaining_seconds, bool skip_enabled);

signals:
    void skipRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    QLabel* time_label_;
    QPushButton* skip_button_;
};

}  // namespace health_reminder::strict
