# Sandhika

A Qt6-based desktop application for Linux to remind you to take breaks, drink water, and maintain healthy habits.

## Documentation

- [Specification](./SPEC.md)
- [Configuration Guide](./config/config.example.yaml)
- [GitHub Releases](../../releases)
  
## Features

### Eye Breaks

* Configurable reminders
* Strict fullscreen break mode
* Brightness dimming
* Skip lock timer
* Countdown animation

### Water Reminders

* Periodic hydration reminders
* Snooze support
* Statistics tracking

### Custom Reminders

Supports:

* Interval reminders
* Daily reminders
* Weekly reminders
* Monthly reminders

Examples:

```yaml
- name: "Take Medicine"
  interval: 4h

- name: "Lunch"
  at: "13:00"

- name: "Weekly Review"
  weekday: Sunday
  at: "18:00"

- name: "Pay Electricity Bill"
  day: 1
  at: "10:00"
```

### Dashboard

* Health Score
* Screen Time
* Eye Break Streaks
* Water Intake Trend
* 30 Day Charts

### System Tray

* Pause 30m
* Pause 1h
* Resume
* Reload Config
* Media Mode
* Open Dashboard

### Smart Suppression

* Idle detection
* Media mode
* Fullscreen movie suppression
* Gaming mode

---

## Screenshots

| Dashboard | Notification |
|-----------|-------------|
| ![](./screenshots/dashboard.png) | ![](./screenshots/notification.png) |

| Strict Break | Tray |
|-------------|------|
| ![](./screenshots/strict-break.png) | ![](./screenshots/tray.png) |

| Statistics | Trends |
|-----------|--------|
| ![](./screenshots/statistics.png) | ![](./screenshots/trends.png) |

---

## Installation

### Download AppImage

Download the latest release from:

```text
GitHub Releases
```

Then:

```bash
chmod +x Sandhika-x86_64.AppImage

./Sandhika-x86_64.AppImage
```

---

## Build from Source

Requirements:

```text
Qt6
Qt Charts
Qt Svg
yaml-cpp
cmake
g++
```

Build:

```bash
cmake -S . -B build

cmake --build build -j
```

---

## Running

```bash
sandhika start
```

Commands:

```bash
sandhika pause 1h

sandhika resume

sandhika reload

sandhika media on

sandhika media off

sandhika status
```

---

## Configuration

Config file:

```text
~/.config/sandhika/config.yaml
```

A default config is created automatically if it does not exist.

---

## Full Documentation

See:

```text
SPEC.md
```

The specification contains:

* Architecture
* Module Design
* Scheduler
* Notification System
* Strict Break Window
* Dashboard
* Statistics
* YAML Schema
* AppImage Packaging
* Installation
* Future Roadmap

---

## Project Structure

```text
include/
src/
tests/
resources/
config/
systemd/
scripts/
```

---

## Technology Stack

* C++20
* Qt6 Widgets
* Qt Charts
* yaml-cpp
* CMake
* Linux AppImage

---

## License

MIT License
