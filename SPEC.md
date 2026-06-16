# Health Reminder Specification

This document is the canonical specification for Health Reminder, a production Linux desktop application that helps users take eye breaks, drink water, and maintain healthy long-session computer habits.

## Product Goals

- Run as a single long-lived desktop daemon named `health-reminder`.
- Provide reliable health reminders without disrupting games, videos, presentations, or idle time.
- Offer normal notifications for lightweight reminders and a strict fullscreen break mode when configured.
- Store useful local statistics without telemetry or external services.
- Remain maintainable through small modules, explicit ownership, RAII, and unit-tested core logic.

## Technology

- C++20
- Qt6 Widgets for desktop UI, tray, windows, and notification integration
- yaml-cpp for configuration and statistics files
- Modern CMake
- Linux desktop target with GNOME compatibility

## Architecture

`health-reminder` is a single daemon-style desktop process. It owns all modules through `ApplicationController`; modules communicate through explicit interfaces and callbacks.

```text
health-reminder
├── ConfigManager
├── ReminderScheduler
├── IdleDetector
├── NotificationManager
├── StrictBreakWindow
├── StatsManager
├── BrightnessController
├── FullscreenDetector
├── CommandExecutor
├── SystemTrayManager
├── Dashboard
├── HealthScoreCalculator
└── ApplicationController
```

### Module Responsibilities

- `ConfigManager`: Loads YAML configuration, validates schema, exposes strongly typed immutable config snapshots, and supports thread-safe reload.
- `ReminderScheduler`: Pure C++ scheduler for interval, daily, weekly, monthly, snoozed, and paused reminders. Uses `std::chrono`, `std::jthread`, and condition variables.
- `IdleDetector`: Tracks keyboard/mouse idle duration and active screen time. GNOME compatible, preferring XScreenSaver and falling back to `xprintidle` where available.
- `FullscreenDetector`: Detects fullscreen games, videos, browser playback, and presentations. Suppresses reminders while fullscreen is active.
- `BrightnessController`: Saves current brightness, dims to configured strict-mode brightness, and restores brightness. Uses `brightnessctl`; unsupported systems degrade to no-op.
- `CommandExecutor`: Runs configured commands asynchronously through `QProcess`, supports `~` expansion, stdout/stderr capture, and timeout handling.
- `NotificationManager`: Shows Qt desktop notifications for eye, water, and custom reminders with actions.
- `StrictBreakWindow`: Fullscreen Qt Widgets break overlay with countdown, input suppression, brightness dimming, and delayed skip availability.
- `StatsManager`: Stores one YAML stats file per day, keeps the last 30 days, computes daily/weekly/monthly aggregates and streaks.
- `HealthScoreCalculator`: Produces a 0-100 health score from break compliance, water compliance, completion rates, and screen time.
- `SystemTrayManager`: Provides tray status, next reminder countdowns, pause/resume actions, dashboard access, config reload, and quit.
- `Dashboard`: Qt Widgets dashboard with overview, statistics, trends, health score, and last-30-day graphs.
- `ApplicationController`: Wires modules together, owns lifetime, handles startup/shutdown, config reload, pause/resume, and event routing.

## Folder Structure

```text
config/                         Example configuration files
icons/                          Application and notification icons
include/health_reminder/        Public module headers
resources/                      Qt resources and static UI assets
scripts/                        Packaging/helper scripts
src/                            Module implementations and main executable
systemd/                        User service files
tests/                          Unit and integration tests
```

## Configuration

Default config path:

```text
~/.config/health-reminder/config.yaml
```

Durations use positive integer values followed by `s`, `m`, or `h`. Times use strict 24-hour `HH:MM` format.

### YAML Schema

```yaml
eye_break:
  enabled: true
  interval: 20m
  duration: 20s
  strict_mode: false
  skip_unlock: 60s
  sound: true

water:
  enabled: true
  interval: 1h
  sound: true

custom:
  - name: "Check Posture"
    interval: 30m
    message: "Sit straight"
    sound: true
    icon: posture
    command: "playerctl pause"

  - name: "Lunch"
    at: "13:00"
    message: "Take lunch"

  - name: "Weekly Review"
    weekday: Sunday
    at: "18:00"
    message: "Review goals"

  - name: "Pay Bill"
    day: 1
    at: "10:00"
    message: "Pay bill"

quiet_hours:
  enabled: true
  start: "23:00"
  end: "07:00"

weekend:
  eye_break:
    enabled: false
  water:
    enabled: true

battery:
  low_threshold: 15
```

### Custom Reminder Rules

- Interval reminders use `interval`.
- Daily reminders use `at`.
- Weekly reminders use `weekday` plus `at`.
- Monthly reminders use `day` plus `at`.
- `interval` cannot be mixed with `at`, `weekday`, or `day`.
- `weekday` and `day` cannot be used together.

### Custom Actions

Custom reminders may define a command:

```yaml
command: "playerctl pause"
command: "~/scripts/backup.sh"
command: "xdg-open https://example.com"
```

Commands run asynchronously, capture stdout/stderr, and enforce timeouts.

## Reminder Behavior

### Eye Break

- Default interval: every 20 minutes.
- Default duration: 20 seconds.
- Buttons: Done, Snooze 5m, Snooze 10m, Skip.
- If idle, reminders are skipped and screen time is not counted.
- If fullscreen is active, reminders are suppressed and resume afterward.
- If strict mode is disabled, eye breaks use normal notifications.
- If strict mode is enabled, eye breaks use `StrictBreakWindow`.

### Water Reminder

- Default interval: every 1 hour.
- Buttons: Done, Snooze 5m, Snooze 10m, Skip.
- Suppressed during idle and fullscreen states.

### Snooze and Pause

- Snooze applies to a single reminder instance and schedules it after the selected duration.
- Pause may apply to one reminder or all reminders.
- Paused reminders do not trigger until the pause expires or the user resumes.

## Strict Mode

Strict mode displays a fullscreen break overlay.

Requirements:

- Disable ESC dismissal.
- Disable Alt+F4 dismissal.
- Disable the close button.
- Show countdown.
- Animate countdown progress.
- Dim brightness to 20%.
- Restore original brightness on completion.
- Disable Skip until `eye_break.skip_unlock` has elapsed.
- Always restore brightness during normal close, app shutdown, or error cleanup.

## Idle Detection

Detect:

- Keyboard activity
- Mouse activity
- Idle duration
- Active screen time

Rules:

- Idle time does not count as screen time.
- Reminders are skipped while idle.
- Scheduler resumes normal behavior after activity returns.

## Fullscreen Detection

Detect fullscreen:

- Games
- VLC and other video players
- Browser videos
- Presentations

Rules:

- Suppress reminders while fullscreen is active.
- Resume scheduling when fullscreen exits.

## Statistics

Stats path:

```text
~/.local/share/health-reminder/stats/
```

Store one YAML file per day. Keep only the last 30 days.

Example:

```yaml
date: 2026-06-16

screen_time:
  active_minutes: 542

eye_break:
  shown: 25
  completed: 21
  skipped: 4

water:
  shown: 8
  completed: 6
  skipped: 2

streaks:
  eye_break_current: 12
  eye_break_best: 27
```

Computed views:

- Daily stats
- Weekly stats
- Monthly stats
- Eye break streaks
- Water streaks
- Health score inputs

## Dashboard

The dashboard is a Qt Widgets window with:

- Overview
- Statistics
- Trends
- Health Score
- Daily screen time graph
- Eye break trend graph
- Water trend graph
- Last 30 days view

Display:

- Today's screen time
- Eye break stats
- Water stats
- Streaks
- Health score from 0 to 100
- Health score label, such as Excellent, Good, Fair, or Needs Attention

## System Tray

Tray status shows:

- Next eye break countdown
- Next water reminder countdown

Menu:

- Pause 30m
- Pause 1h
- Resume
- Dashboard
- Reload Config
- Quit

## Installation

The project must provide:

- CMake build targets
- Installable executable
- Icons and Qt resources
- Example config file
- Optional user-level systemd service
- GNOME-compatible desktop integration

Build:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Engineering Rules

- No global mutable state.
- Use RAII everywhere.
- Prefer `std::chrono` for time.
- Prefer composition over inheritance.
- Separate headers and sources.
- Unit tests are required for each module.
- No placeholder implementations.
- No TODO implementations.
- No deprecated Qt APIs.
- Core logic should be testable without Qt where practical.
- Thread-safe modules must document their locking and callback behavior.
