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

int CombatSystem::executeAttack(Entity& attacker, Entity& defender, float enduranceMultiplier) {
    // Cost in endurance per attack (Pugilat by default = 7.5f)
    float cost = 7.5f; 
    
    if (attacker.weapon.has_value()) {
        if (attacker.weapon->weight == WeaponWeight::Leger) {
            cost = 7.5f;
        } else if (attacker.weapon->weight == WeaponWeight::Moyen) {
            cost = 10.0f;
        } else if (attacker.weapon->weight == WeaponWeight::Lourd) {
            cost = 12.5f;
        }
    }

    cost *= enduranceMultiplier;

    attacker.physicalReserve -= cost;
    if (attacker.physicalReserve < 0) {
        attacker.physicalReserve = 0;
    }

    // Determine which character has the lowest stat involved (Force vs Resistance)
    // and use their level as reference for the calculation
    int refLevel = (attacker.force < defender.resistance) ? attacker.stade : defender.stade;
    
    int effectiveness = calculateStatDifference(attacker.force, defender.resistance, refLevel);
    
    bool blocked = false;
    if (defender.armor.has_value() && defender.armor->durability > 0) {
        auto& armor = defender.armor.value();
        
        int refArmorLevel = (attacker.force < armor.res) ? attacker.stade : defender.stade;
        int eff_armor = calculateStatDifference(attacker.force, armor.res, refArmorLevel);
        
        int durabilityLoss = 0;
        if (eff_armor <= -5) {
            durabilityLoss = 0;
        } else if (eff_armor == -4) {
            durabilityLoss = 1;
        } else if (eff_armor == -3) {
            durabilityLoss = 2;
        } else if (eff_armor == -2) {
            durabilityLoss = 3;
        } else if (eff_armor == -1) {
            durabilityLoss = 4;
        } else if (eff_armor == 0) {
            durabilityLoss = 5;
        } else if (eff_armor == 1) {
            durabilityLoss = 6;
        } else if (eff_armor == 2) {
            durabilityLoss = 7;
        } else if (eff_armor == 3) {
            durabilityLoss = 8;
        } else if (eff_armor == 4) {
            durabilityLoss = 9;
        } else { // eff_armor >= 5 (Domination)
            durabilityLoss = armor.durability;
        }
        
        armor.durability = std::max(0, armor.durability - durabilityLoss);
        
        if (armor.durability > 0) {
            if (armor.material == ArmorMaterial::Peau) {
                if (effectiveness >= 0) {
                    effectiveness = static_cast<int>(effectiveness * 0.9f);
                }
            } else if (armor.material == ArmorMaterial::Mineral) {
                if (attacker.getActiveDamageType() == PhysicalDamageType::Contondant) {
                    if (effectiveness >= 0) {
                        effectiveness = static_cast<int>(effectiveness * 0.9f);
                    }
                } else {
                    blocked = true;
                }
            }
        }
    }
    
    if (blocked) {
        return -99;
    } else {
        defender.applyWound(effectiveness, attacker.getActiveDamageType());
        return effectiveness;
    }
}
