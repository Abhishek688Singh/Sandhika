#pragma once

#include <string>

namespace sandhika::health {

struct HealthScoreInput {
    int eye_break_shown {0};
    int eye_break_completed {0};
    int water_shown {0};
    int water_completed {0};
    int active_screen_minutes {0};
    int break_streak {0};
};

struct HealthScore {
    int value {0};
    std::string label;
};

class HealthScoreCalculator {
public:
    [[nodiscard]] HealthScore calculate(const HealthScoreInput& input) const;
};

}  // namespace sandhika::health
