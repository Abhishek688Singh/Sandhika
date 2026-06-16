#include "sandhika/health/health_score_calculator.h"
#include <cassert>
#include <iostream>

using namespace sandhika::health;

int main() {
    HealthScoreCalculator calc;
    
    // Perfect score
    HealthScoreInput in1 {
        .eye_break_shown = 10,
        .eye_break_completed = 10,
        .water_shown = 5,
        .water_completed = 5,
        .active_screen_minutes = 400,
        .break_streak = 10
    };
    auto score1 = calc.calculate(in1);
    assert(score1.value == 100);
    assert(score1.label == "Excellent");

    // Bad score
    HealthScoreInput in2 {
        .eye_break_shown = 10,
        .eye_break_completed = 0,
        .water_shown = 5,
        .water_completed = 0,
        .active_screen_minutes = 800,
        .break_streak = 0
    };
    auto score2 = calc.calculate(in2);
    assert(score2.value == 0);
    assert(score2.label == "Needs Attention");

    std::cout << "HealthScoreCalculator tests passed\n";
    return 0;
}
