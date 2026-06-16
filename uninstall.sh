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
update-desktop-database ~/.local/share/applications/ || true

echo "Removing binary..."
rm -f ~/.local/bin/sandhika

echo "Removing cache..."
rm -rf ~/.cache/sandhika

echo "Optional: remove config and stats by running:"
echo "rm -rf ~/.config/sandhika"
echo "rm -rf ~/.local/share/sandhika"

echo "Done!"
