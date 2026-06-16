#include "health_reminder/notifications/notification_manager.h"

#include <algorithm>
#include <stdexcept>

namespace health_reminder::notifications {

void NotificationManager::setActionCallback(ActionCallback callback) {
    std::lock_guard lock(mutex_);
    callback_ = std::move(callback);
}

void NotificationManager::showNotification(NotificationRequest request) {
    if (request.reminder_id.empty()) {
        throw std::invalid_argument("Notification reminder id must not be empty");
    }
    if (request.title.empty()) {
        throw std::invalid_argument("Notification title must not be empty");
    }

    std::lock_guard lock(mutex_);
    active_.erase(std::remove_if(active_.begin(), active_.end(), [&](const ActiveNotification& item) {
                      return item.request.reminder_id == request.reminder_id;
                  }),
                  active_.end());
    active_.push_back(ActiveNotification {
        .request = std::move(request),
        .shown_at = std::chrono::steady_clock::now(),
    });
}

bool NotificationManager::activateAction(const std::string& reminder_id, NotificationAction action) {
    ActionCallback callback;
    {
        std::lock_guard lock(mutex_);
        const auto it = std::find_if(active_.begin(), active_.end(), [&](const ActiveNotification& item) {
            return item.request.reminder_id == reminder_id;
        });
        if (it == active_.end()) {
            return false;
        }
        if (action == NotificationAction::Skip && !skipAllowedLocked(*it)) {
            return false;
        }
        callback = callback_;
        active_.erase(it);
    }

    if (callback) {
        callback(reminder_id, action);
    }
    return true;
}

std::vector<ActiveNotification> NotificationManager::activeNotifications() const {
    std::lock_guard lock(mutex_);
    return active_;
}

void NotificationManager::closeNotification(const std::string& reminder_id) {
    std::lock_guard lock(mutex_);
    active_.erase(std::remove_if(active_.begin(), active_.end(), [&](const ActiveNotification& item) {
                      return item.request.reminder_id == reminder_id;
                  }),
                  active_.end());
}

void NotificationManager::closeAll() {
    std::lock_guard lock(mutex_);
    active_.clear();
}

bool NotificationManager::skipAllowedLocked(const ActiveNotification& notification) const {
    return std::chrono::steady_clock::now() - notification.shown_at >= notification.request.skip_unlock;
}

}  // namespace health_reminder::notifications
