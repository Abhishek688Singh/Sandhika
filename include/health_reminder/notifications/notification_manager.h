#pragma once

#include <chrono>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace health_reminder::notifications {

enum class NotificationAction {
    Done,
    Snooze5m,
    Snooze10m,
    Skip
};

struct NotificationRequest {
    std::string reminder_id;
    std::string title;
    std::string message;
    bool sound {true};
    std::optional<std::string> icon;
    std::optional<std::string> command;
    std::chrono::seconds skip_unlock {0};
};

struct ActiveNotification {
    NotificationRequest request;
    std::chrono::steady_clock::time_point shown_at;
};

class NotificationManager {
public:
    using ActionCallback = std::function<void(std::string, NotificationAction)>;

    void setActionCallback(ActionCallback callback);
    void showNotification(NotificationRequest request);
    [[nodiscard]] bool activateAction(const std::string& reminder_id, NotificationAction action);
    [[nodiscard]] std::vector<ActiveNotification> activeNotifications() const;
    void closeNotification(const std::string& reminder_id);
    void closeAll();

private:
    [[nodiscard]] bool skipAllowedLocked(const ActiveNotification& notification) const;

    mutable std::mutex mutex_;
    std::vector<ActiveNotification> active_;
    ActionCallback callback_;
};

}  // namespace health_reminder::notifications
