#!/bin/bash
set -e

echo "Building Health Reminder..."
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

echo "Installing binary..."
sudo cp build/health-reminder /usr/local/bin/
sudo chmod +x /usr/local/bin/health-reminder

echo "Installing desktop entry..."
mkdir -p ~/.local/share/applications
cp health-reminder.desktop ~/.local/share/applications/
update-desktop-database ~/.local/share/applications/

echo "Installing systemd service..."
mkdir -p ~/.config/systemd/user
cp health-reminder.service ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable health-reminder.service
systemctl --user restart health-reminder.service

echo "Done!"
