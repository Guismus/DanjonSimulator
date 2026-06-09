#pragma once

#include "../core/entity.hpp"
#include <array>

class CombatSystem {
public:
    static void setDiffStatsTable(const std::array<std::array<float, 7>, 32>& table);
    static int calculateStatDifference(float attackerStat, float defenderStat, int referenceLevel);
    // Execute attack and return the calculated effectiveness
    static int executeAttack(Entity& attacker, Entity& defender, float enduranceMultiplier = 1.0f,
                             DamageNature nature = DamageNature::Physique,
                             std::optional<DamageType> overrideType = std::nullopt,
                             WeaponDamageMultipliers multipliers = {});
    static void executeParry(Entity& character, float enduranceMultiplier = 1.0f);
    static void executeDodge(Entity& character, float enduranceMultiplier = 1.0f);
};
