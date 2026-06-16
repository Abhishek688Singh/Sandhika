#!/bin/bash
set -e

echo "Stopping service..."
systemctl --user stop sandhika.service || true
systemctl --user disable sandhika.service || true

echo "Removing systemd service..."
rm -f ~/.config/systemd/user/sandhika.service
systemctl --user daemon-reload

echo "Removing desktop entry..."
rm -f ~/.local/share/applications/sandhika.desktop
update-desktop-database ~/.local/share/applications/

echo "Removing binary..."
sudo rm -f /usr/local/bin/sandhika

echo "Done!"
