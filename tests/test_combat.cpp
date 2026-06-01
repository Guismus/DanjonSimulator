#include "../back/core/entity.hpp"
#include "../back/systems/CombatSystem.hpp"
#include "../back/data/DataStore.hpp"
#include <cassert>
#include <cmath>

void test_speed_free_actions() {
    float v1 = 12.0f;
    float v2 = 12.0f;
    int refLevel = 1;
    int speedDiff = CombatSystem::calculateStatDifference(v1, v2, refLevel);
    assert(speedDiff == 0);

    int p1FreeActions = 2 + (speedDiff > 0 ? speedDiff : 0);
    int p2FreeActions = 2 + (speedDiff < 0 ? -speedDiff : 0);
    assert(p1FreeActions == 2);
    assert(p2FreeActions == 2);

    int diffVal = CombatSystem::calculateStatDifference(15.0f, 10.0f, 1);
    assert(diffVal == 5);

    int act1 = 2 + (diffVal > 0 ? diffVal : 0);
    int act2 = 2 + (diffVal < 0 ? -diffVal : 0);
    assert(act1 == 7);
    assert(act2 == 2);
}

void test_stamina_costs() {
    Entity attacker("Attacker");
    Entity defender("Defender");
    
    EnergyThresholds thresh;
    thresh.maxReserve = 400.0f;
    thresh.inconscient = 0.0f;
    attacker.physicalThresholds = thresh;
    attacker.maxPhysicalReserve = 400.0f;
    attacker.physicalReserve = 400.0f;
    
    defender.stade = 1;
    attacker.stade = 1;

    // Pugilat (no weapon) cost = 7.5
    CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(std::abs(attacker.physicalReserve - (400.0f - 7.5f)) < 0.001f);

    // Light weapon (cost = 7.5)
    Weapon lightWeapon{"Light Sword", WeaponWeight::Leger, DamageType::Tranchant, 100, 100, 10, 5};
    attacker.weapon = lightWeapon;
    CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(std::abs(attacker.physicalReserve - (392.5f - 7.5f)) < 0.001f);

    // Medium weapon (cost = 10.0)
    Weapon medWeapon{"Mace", WeaponWeight::Moyen, DamageType::Contondant, 100, 100, 15, 8};
    attacker.weapon = medWeapon;
    CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(std::abs(attacker.physicalReserve - (385.0f - 10.0f)) < 0.001f);

    // Heavy weapon (cost = 12.5)
    Weapon heavyWeapon{"Greatsword", WeaponWeight::Lourd, DamageType::Tranchant, 100, 100, 20, 10};
    attacker.weapon = heavyWeapon;
    CombatSystem::executeAttack(attacker, defender, 2.3f);
    assert(std::abs(attacker.physicalReserve - (375.0f - 28.75f)) < 0.001f);
}

void test_parry_dodge() {
    Entity attacker("Attacker");
    Entity defender("Defender");
    
    attacker.stade = 1;
    defender.stade = 1;
    attacker.force = 10.0f;
    defender.resistance = 10.0f;
    defender.physicalReserve = 100.0f;

    // Test Dodge: prepare dodge
    CombatSystem::executeDodge(defender, 1.0f);
    assert(defender.activeDodges == 1);
    assert(defender.physicalReserve == 90.0f);

    // Execute attack: should be evaded (-98)
    int result = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(result == -98);
    assert(defender.activeDodges == 0);
    assert(defender.wounds.empty());

    // Test Parry: prepare parry
    CombatSystem::executeParry(defender, 1.0f);
    assert(defender.activeParries == 1);
    assert(defender.physicalReserve == 82.5f);

    attacker.force = 15.0f;
    Weapon lightWeapon{"Light Sword", WeaponWeight::Leger, DamageType::Tranchant, 100, 100, 10, 5};
    attacker.weapon = lightWeapon;
    result = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(result == 4);
    assert(defender.activeParries == 0);
    assert(defender.wounds.size() == 1);
    assert(defender.wounds[0].effectiveness == 4);

    // Test Parry with metal shield vs Tranchant
    defender.wounds.clear();
    defender.passives.push_back("Bouclier en métal");
    CombatSystem::executeParry(defender, 1.0f);
    
    Weapon sword{"Sword", WeaponWeight::Moyen, DamageType::Tranchant, 100, 100, 15, 8};
    attacker.weapon = sword;
    
    result = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(result == -97);
    assert(defender.activeParries == 0);
    assert(defender.wounds.empty());
}

void test_stat_penalties_rounding() {
    Entity fighter("Fighter");
    fighter.force = 10.25f;
    fighter.vitesse = 10.0f;
    
    assert(std::abs(fighter.getEffectiveForce() - 10.25f) < 0.001f);
    assert(std::abs(fighter.getEffectiveVitesse() - 10.0f) < 0.001f);

    // Stade 3 wound (-15%)
    fighter.applyWound(3, DamageType::Contondant);
    assert(std::abs(fighter.getEffectiveForce() - 8.75f) < 0.001f);
    assert(std::abs(fighter.getEffectiveVitesse() - 8.5f) < 0.001f);

    // Stade 4 wound (-30%)
    fighter.wounds.clear();
    fighter.applyWound(4, DamageType::Contondant);
    assert(std::abs(fighter.getEffectiveForce() - 7.25f) < 0.001f);
}
