#include "CombatSystem.hpp"
#include "../data/DataStore.hpp"
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

void CombatSystem::executeParry(Entity& character, float enduranceMultiplier) {
    float cost = 7.5f * enduranceMultiplier;
    character.physicalReserve -= cost;
    if (character.physicalReserve < 0) {
        character.physicalReserve = 0;
    }
    character.activeParries++;
}

void CombatSystem::executeDodge(Entity& character, float enduranceMultiplier) {
    float cost = 10.0f * enduranceMultiplier;
    character.physicalReserve -= cost;
    if (character.physicalReserve < 0) {
        character.physicalReserve = 0;
    }
    character.activeDodges++;
}

int CombatSystem::executeAttack(Entity& attacker, Entity& defender, float enduranceMultiplier) {
    // Cost in endurance per attack (Pugilat by default = 7.5f)
    float cost = 7.5f; 
    float multiplier = 1.0f;
    
    if (attacker.isMonster) {
        multiplier = 1.0f;
        cost = 7.5f;
    } else if (attacker.weapon.has_value() && attacker.weapon->durability > 0) {
        if (attacker.weapon->type == WeaponWeight::Leger) {
            multiplier = DataStore::getInstance().getDmgMultLegere();
            cost = 7.5f;
        } else if (attacker.weapon->type == WeaponWeight::Moyen) {
            multiplier = DataStore::getInstance().getDmgMultMoyenne();
            cost = 10.0f;
        } else if (attacker.weapon->type == WeaponWeight::Lourd) {
            multiplier = DataStore::getInstance().getDmgMultLourde();
            cost = 12.5f;
        }
    } else {
        multiplier = DataStore::getInstance().getDmgMultMainsNu();
        cost = 7.5f;
    }

    cost *= enduranceMultiplier;

    attacker.physicalReserve -= cost;
    if (attacker.physicalReserve < 0) {
        attacker.physicalReserve = 0;
    }

    // Check Dodge first
    if (defender.activeDodges > 0) {
        defender.activeDodges--;
        return -98; // Special code for Evaded/Esquivé
    }

    float attForce = attacker.getEffectiveForce();
    float defRes = defender.getEffectiveResistance();

    // Weapon Durability reduction
    if (attacker.weapon.has_value() && attacker.weapon->durability > 0) {
        auto& weapon = attacker.weapon.value();
        int refWeaponLevel = (attForce < weapon.res) ? attacker.stade : defender.stade;
        int eff_weapon = calculateStatDifference(attForce, weapon.res, refWeaponLevel);
        
        int durabilityLoss = 0;
        if (eff_weapon <= -5) {
            durabilityLoss = 0;
        } else if (eff_weapon == -4) {
            durabilityLoss = 1;
        } else if (eff_weapon == -3) {
            durabilityLoss = 2;
        } else if (eff_weapon == -2) {
            durabilityLoss = 3;
        } else if (eff_weapon == -1) {
            durabilityLoss = 4;
        } else if (eff_weapon == 0) {
            durabilityLoss = 5;
        } else if (eff_weapon == 1) {
            durabilityLoss = 6;
        } else if (eff_weapon == 2) {
            durabilityLoss = 7;
        } else if (eff_weapon == 3) {
            durabilityLoss = 8;
        } else if (eff_weapon == 4) {
            durabilityLoss = 9;
        } else { // eff_weapon >= 5
            durabilityLoss = weapon.durability;
        }
        weapon.durability = std::max(0, weapon.durability - durabilityLoss);
    }

    // Determine which character has the lowest stat involved (Force vs Resistance)
    // and use their level as reference for the calculation
    int refLevel = (attForce < defRes) ? attacker.stade : defender.stade;
    
    int effectiveness = calculateStatDifference(attForce, defRes, refLevel);

    // Apply weapon damage multiplier
    if (effectiveness >= 0) {
        effectiveness = static_cast<int>(effectiveness * multiplier);
    }
    
    // Check Parry
    bool parried = false;
    bool parryBlocked = false;
    if (defender.activeParries > 0) {
        defender.activeParries--;
        parried = true;
        if (defender.hasPassive("Bouclier en métal") && attacker.getActiveDamageType() == PhysicalDamageType::Tranchant) {
            parryBlocked = true;
        }
    }

    if (parryBlocked) {
        return -97; // Special code for Parry Blocked (bouclier en métal vs tranchant)
    }

    if (parried) {
        if (effectiveness >= 0) {
            effectiveness = static_cast<int>(effectiveness * 0.9f); // 10% reduction
        }
    }
    
    bool blocked = false;
    if (defender.armor.has_value() && defender.armor->durability > 0) {
        auto& armor = defender.armor.value();
        
        int refArmorLevel = (attForce < armor.res) ? attacker.stade : defender.stade;
        int eff_armor = calculateStatDifference(attForce, armor.res, refArmorLevel);
        
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
                    effectiveness = static_cast<int>(effectiveness * 0.85f); // 15% reduction
                }
            } else if (armor.material == ArmorMaterial::Mineral) {
                if (attacker.getActiveDamageType() == PhysicalDamageType::Contondant) {
                    if (effectiveness >= 0) {
                        effectiveness = static_cast<int>(effectiveness * 0.85f); // 15% reduction
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
