#!/bin/bash
set -e

echo "Building Sandhika..."
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

echo "Installing binary..."
mkdir -p ~/.local/bin
cp build/sandhika ~/.local/bin/
chmod +x ~/.local/bin/sandhika

echo "Installing desktop entry..."
mkdir -p ~/.local/share/applications
cp sandhika.desktop ~/.local/share/applications/
update-desktop-database ~/.local/share/applications/ || true

echo "Installing systemd service..."
mkdir -p ~/.config/systemd/user
cp sandhika.service ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable sandhika.service
systemctl --user restart sandhika.service

echo "Setting up config and data directories..."
mkdir -p ~/.config/sandhika
mkdir -p ~/.local/share/sandhika/stats
if [ ! -f ~/.config/sandhika/config.yaml ]; then
    cp config/config.example.yaml ~/.config/sandhika/config.yaml
fi
echo "Done!"
