#pragma once

#include "../core/entity.hpp"
#include <array>

class CombatSystem {
public:
    static void setDiffStatsTable(const std::array<std::array<float, 7>, 32>& table);
    static int calculateStatDifference(float attackerStat, float defenderStat, int referenceLevel);
    // Remove unused placeholder methods for now to prevent linker issues, 
    // or we can just keep executeAttack
    static void executeAttack(Entity& attacker, Entity& defender);
};
