# Health Reminder

A Qt6-based desktop application for Linux to remind you to take breaks, drink water, and maintain healthy habits.

## Features
- **Strict Break Window:** Fullscreen overlay that forces you to take eye breaks (dimmed brightness, disabled escape keys).
- **Dashboard:** Track your health score, screen time, and break compliance over the last 30 days using Qt Charts.
- **System Tray:** Quick access to pause, resume, and overview upcoming breaks.
- **Smart Idle & Fullscreen Detection:** Reminders are snoozed automatically if you are away or watching a movie/playing a game.

## Dependencies
- `qt6-base-dev`, `qt6-svg-dev`, `libqt6charts6-dev`
- `xprintidle` (for idle detection)
- `brightnessctl` (for dimming the screen during strict breaks)
- `yaml-cpp` (automatically fetched via CMake if not found)

## Installation

Run the install script:
```bash
./install.sh
```

## Running
```bash
health-reminder start
```
The application runs as a background daemon and exposes a CLI for interacting with it:
```bash
health-reminder pause 1h
health-reminder resume
health-reminder reload
```

## Configuration
Configuration is automatically created at `~/.config/health-reminder/config.yaml` if it is missing, by copying the default `config.example.yaml` or falling back to default values. The application will never abort due to a missing configuration file.

## License
MIT
