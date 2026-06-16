#!/bin/bash
set -e

echo "Building Sandhika..."
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

echo "Installing binary..."
sudo cp build/sandhika /usr/local/bin/
sudo chmod +x /usr/local/bin/sandhika

echo "Installing desktop entry..."
mkdir -p ~/.local/share/applications
cp sandhika.desktop ~/.local/share/applications/
update-desktop-database ~/.local/share/applications/

echo "Installing systemd service..."
mkdir -p ~/.config/systemd/user
cp sandhika.service ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable sandhika.service
systemctl --user restart sandhika.service

echo "Done!"
