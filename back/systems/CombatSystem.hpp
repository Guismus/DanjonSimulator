#pragma once

#include "../core/entity.hpp"
#include <array>

class CombatSystem {
public:
    static void setDiffStatsTable(const std::array<std::array<float, 7>, 32>& table);
    static int calculateStatDifference(float attackerStat, float defenderStat, int referenceLevel);
    // Execute attack and return the calculated effectiveness
    static int executeAttack(Entity& attacker, Entity& defender);
};
