#!/bin/bash
set -e

echo "Stopping service..."
systemctl --user stop health-reminder.service || true
systemctl --user disable health-reminder.service || true

echo "Removing systemd service..."
rm -f ~/.config/systemd/user/health-reminder.service
systemctl --user daemon-reload

echo "Removing desktop entry..."
rm -f ~/.local/share/applications/health-reminder.desktop
update-desktop-database ~/.local/share/applications/

echo "Removing binary..."
sudo rm -f /usr/local/bin/health-reminder

echo "Done!"
