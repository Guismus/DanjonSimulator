#include "CombatSystem.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <array>
#include <vector>

// Table de "Diff Stats" (Niveaux 1 à 32)
static std::array<std::array<float, 7>, 32> diffStatsTable;

void CombatSystem::setDiffStatsTable(const std::array<std::array<float, 7>, 32>& table) {
    diffStatsTable = table;
}

// Calculate diff stat effectiveness (-7 to +7)
int CombatSystem::calculateStatDifference(float attackerStat, float defenderStat, int referenceLevel) {
    float rawDiff = attackerStat - defenderStat;
    bool isNegative = (rawDiff < 0);
    float absDiff = std::abs(rawDiff);
    
    // Clamp referenceLevel between 1 and 32 to avoid out-of-bounds
    int clampedLevel = std::clamp(referenceLevel, 1, 32);
    const auto& thresholds = diffStatsTable[clampedLevel - 1];
    
    int effectiveness = 0;
    for (int i = 6; i >= 0; --i) {
        if (absDiff >= thresholds[i]) {
            effectiveness = i + 1;
            break;
        }
    }
    
    return isNegative ? -effectiveness : effectiveness;
}

void CombatSystem::executeAttack(Entity& attacker, Entity& defender) {
    // Determine which character has the lowest stat involved (Force vs Resistance)
    // and use their level as reference for the calculation
    int refLevel = (attacker.force < defender.resistance) ? attacker.stade : defender.stade;
    
    int effectiveness = calculateStatDifference(attacker.force, defender.resistance, refLevel);
    
    // According to the rules: Damage = cumulative diff stats. +5 accumulated = death.
    // If effectiveness is > 0, it means the attacker successfully deals damage.
    // Negative effectiveness could mean the attack is completely ineffective (0 damage).
    int damageToApply = std::max(0, effectiveness);
    
    defender.blood += damageToApply;
    
    // Death condition
    if (defender.blood >= 5.0f) {
        defender.blood = 5.0f; // Cap at 5 which means dead
    }
}
