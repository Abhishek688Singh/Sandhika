# Health Reminder

Linux desktop health reminder application (C++20, Qt6 Widgets, CMake) for long laptop usage.

## Current Implementation Status

This initial delivery implements the **ConfigManager** module with:

- YAML parsing using `yaml-cpp`
- Strongly typed config models
- Validation for durations, time-of-day, weekday, monthly day, and battery threshold
- Thread-safe reload/getters
- Unit tests for valid and invalid configurations

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Config File

Default path:

`~/.config/health-reminder/config.yaml`

See `config/config.example.yaml` for a full example.
