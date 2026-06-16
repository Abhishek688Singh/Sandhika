#include "health_reminder/health/health_score_calculator.h"

#include <algorithm>

namespace health_reminder::health {
namespace {

[[nodiscard]] int compliance(int completed, int shown, int weight) {
    if (shown <= 0) {
        return weight;
    }
    return (completed * weight) / shown;
}

[[nodiscard]] std::string label_for(int value) {
    if (value >= 90) {
        return "Excellent";
    }
    if (value >= 75) {
        return "Good";
    }
    if (value >= 60) {
        return "Fair";
    }
    return "Needs Attention";
}

}  // namespace

HealthScore HealthScoreCalculator::calculate(const HealthScoreInput& input) const {
    int score = 0;
    score += compliance(input.eye_break_completed, input.eye_break_shown, 40);
    score += compliance(input.water_completed, input.water_shown, 25);

    if (input.active_screen_minutes <= 480) {
        score += 25;
    } else if (input.active_screen_minutes <= 600) {
        score += 15;
    } else if (input.active_screen_minutes <= 720) {
        score += 8;
    }

    score += std::min(input.break_streak, 10);
    score = std::clamp(score, 0, 100);
    return HealthScore {.value = score, .label = label_for(score)};
}

}  // namespace health_reminder::health
