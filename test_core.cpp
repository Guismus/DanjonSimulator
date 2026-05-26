#include "back/data/DataStore.hpp"
#include "back/systems/CombatSystem.hpp"
#include "back/core/entity.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

#define RUN_TEST(test_func) \
    do { \
        std::cout << "[RUN] " << #test_func << "..." << std::endl; \
        test_func(); \
        std::cout << "[SUCCESS] " << #test_func << std::endl; \
    } while (0)

void test_wound_stacking() {
    Entity fighter("Test Fighter");
    fighter.blood = 32.0f;
    fighter.physicalReserve = 100.0f;

    // Apply first wound
    fighter.applyWound(1, PhysicalDamageType::Tranchant);
    assert(fighter.wounds.size() == 1);
    assert(fighter.wounds[0].effectiveness == 1);

    // Apply second identical wound, they should combine: 2 Stade 1 -> Stade 2
    fighter.applyWound(1, PhysicalDamageType::Tranchant);
    assert(fighter.wounds.size() == 1);
    assert(fighter.wounds[0].effectiveness == 2);

    // Apply another Stade 2 wound, they should combine: Stade 2 + Stade 2 -> Stade 3
    fighter.applyWound(2, PhysicalDamageType::Tranchant);
    assert(fighter.wounds.size() == 1);
    assert(fighter.wounds[0].effectiveness == 3);

    // Apply a Stade 1 wound, should not combine, we should have Stade 3 and Stade 1 sorted descending
    fighter.applyWound(1, PhysicalDamageType::Tranchant);
    assert(fighter.wounds.size() == 2);
    assert(fighter.wounds[0].effectiveness == 3);
    assert(fighter.wounds[1].effectiveness == 1);
}

void test_death_rules() {
    // 1. Base case: healthy
    Entity fighter("Test Fighter");
    fighter.blood = 32.0f;
    fighter.physicalReserve = 100.0f;
    assert(!fighter.isDead());

    // 2. Stamina <= 0 is unconscious/KO, NOT dead
    fighter.physicalReserve = 0.0f;
    assert(!fighter.isDead());
    fighter.physicalReserve = 100.0f;

    // 3. Stade 5 wound -> dead
    fighter.applyWound(5, PhysicalDamageType::Neutre);
    assert(fighter.isDead());

    // Reset wounds
    fighter.wounds.clear();
    assert(!fighter.isDead());

    // 4. Blood <= 0 -> dead
    fighter.blood = 0.0f;
    assert(fighter.isDead());
    fighter.blood = 32.0f;
    assert(!fighter.isDead());

    // 5. Combined rule: Stade 4 (Surpuissance) + Stade 2 (Avantage) -> dead
    fighter.applyWound(4, PhysicalDamageType::Neutre);
    fighter.applyWound(2, PhysicalDamageType::Neutre);
    assert(fighter.isDead());

    // Reset wounds
    fighter.wounds.clear();

    // 6. Combined rule: Stade 4 (Surpuissance) + Stade 0 (Neutre) -> dead
    fighter.applyWound(4, PhysicalDamageType::Neutre);
    fighter.applyWound(0, PhysicalDamageType::Neutre);
    assert(fighter.isDead());

    // Reset wounds
    fighter.wounds.clear();

    // 7. Combined rule: Stade 4 (Surpuissance) + Stade -1 (Sous-Faveur) -> NOT dead
    fighter.applyWound(4, PhysicalDamageType::Neutre);
    fighter.applyWound(-1, PhysicalDamageType::Neutre);
    assert(!fighter.isDead());
}

void test_bleeding_rates() {
    Entity fighter("Test Fighter");
    
    // Contondant wounds do not bleed
    fighter.applyWound(2, PhysicalDamageType::Contondant);
    assert(fighter.getBleedingRate() == 0);
    assert(fighter.getBleedingState() == "Aucun");

    fighter.wounds.clear();

    // Tranchant wounds bleed:
    // Stade <= 0: rate = 1 (Faible)
    fighter.applyWound(0, PhysicalDamageType::Tranchant);
    assert(fighter.getBleedingRate() == 1);
    assert(fighter.getBleedingState() == "Bénin");

    // Stade 1 or 2: rate = 2 (Moyen)
    fighter.applyWound(2, PhysicalDamageType::Tranchant); // Note: this does not combine with Stade 0
    assert(fighter.getBleedingRate() == 2); // max rate is 2
    assert(fighter.getBleedingState() == "Violent");

    // Stade >= 3: rate = 3 (Grave)
    fighter.applyWound(3, PhysicalDamageType::Tranchant);
    assert(fighter.getBleedingRate() == 3);
    assert(fighter.getBleedingState() == "Grave");

    // Test applyBleeding severity reduction
    fighter.blood = 32.0f;
    fighter.applyBleeding(5);
    assert(std::abs(fighter.blood - 27.0f) < 0.001f);
}

void test_speed_free_actions() {
    // We mock/test calculateStatDifference using the loaded data.
    // Let's verify that when speeds are equal, stat difference is 0.
    float v1 = 12.0f;
    float v2 = 12.0f;
    int refLevel = 1;
    int speedDiff = CombatSystem::calculateStatDifference(v1, v2, refLevel);
    assert(speedDiff == 0);

    // Free actions calculation logic:
    int p1FreeActions = 2 + (speedDiff > 0 ? speedDiff : 0);
    int p2FreeActions = 2 + (speedDiff < 0 ? -speedDiff : 0);
    assert(p1FreeActions == 2);
    assert(p2FreeActions == 2);

    // Let's test with a difference. If v1 = 15.0, v2 = 10.0 at level 1:
    // We loaded diff_stats.json, let's test it.
    // Thresholds at Level 1 in diff_stats.json are: [0.5, 1.2, 2.0, 3.5, 5.0, 8.0, 12.0]
    // Abs difference is 5.0.
    // Let's see: 5.0 >= thresholds[4] (5.0), so effectiveness should be 5.
    int diffVal = CombatSystem::calculateStatDifference(15.0f, 10.0f, 1);
    assert(diffVal == 5);

    int act1 = 2 + (diffVal > 0 ? diffVal : 0);
    int act2 = 2 + (diffVal < 0 ? -diffVal : 0);
    assert(act1 == 7); // 2 + 5 = 7
    assert(act2 == 2);
}

void test_stamina_costs() {
    Entity attacker("Attacker");
    Entity defender("Defender");
    
    // Load a mock rank threshold D (400 max, unconscious 0)
    EnergyThresholds thresh;
    thresh.maxReserve = 400.0f;
    thresh.inconscient = 0.0f;
    attacker.physicalThresholds = thresh;
    attacker.maxPhysicalReserve = 400.0f;
    attacker.physicalReserve = 400.0f;
    
    defender.stade = 1;
    attacker.stade = 1;

    // Execute Pugilat (no weapon) cost = 7.5
    // multiplier = 1.0
    CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(std::abs(attacker.physicalReserve - (400.0f - 7.5f)) < 0.001f);

    // Equip light weapon (cost = 7.5)
    Weapon lightWeapon{"Light Sword", WeaponWeight::Leger, PhysicalDamageType::Tranchant, 100, 100, 10, 5};
    attacker.weapon = lightWeapon;
    CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(std::abs(attacker.physicalReserve - (392.5f - 7.5f)) < 0.001f);

    // Equip medium weapon (cost = 10.0)
    Weapon medWeapon{"Mace", WeaponWeight::Moyen, PhysicalDamageType::Contondant, 100, 100, 15, 8};
    attacker.weapon = medWeapon;
    CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(std::abs(attacker.physicalReserve - (385.0f - 10.0f)) < 0.001f);

    // Equip heavy weapon (cost = 12.5)
    Weapon heavyWeapon{"Greatsword", WeaponWeight::Lourd, PhysicalDamageType::Tranchant, 100, 100, 20, 10};
    attacker.weapon = heavyWeapon;
    // Overclock with multiplier 2.3f
    CombatSystem::executeAttack(attacker, defender, 2.3f);
    // Cost should be 12.5 * 2.3 = 28.75
    assert(std::abs(attacker.physicalReserve - (375.0f - 28.75f)) < 0.001f);
}

void test_armor_blocking() {
    Entity attacker("Attacker");
    Entity defender("Defender");
    
    attacker.stade = 1;
    defender.stade = 1;
    attacker.force = 10.0f;
    defender.resistance = 10.0f;

    // Equip mineral armor (blocks Tranchant, reduces Contondant 15%)
    Armor mineralArmor{"Mineral Armor", ArmorMaterial::Mineral, 100, 100, 10, 0};
    defender.armor = mineralArmor;

    // Attack with tranchant weapon (should be BLOCKED)
    Weapon sword{"Sword", WeaponWeight::Moyen, PhysicalDamageType::Tranchant, 100, 100, 15, 8};
    attacker.weapon = sword;
    
    int result = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(result == -99); // -99 represents blocked
    assert(defender.wounds.empty());

    // Attack with contondant weapon (should hit)
    Weapon hammer{"Hammer", WeaponWeight::Moyen, PhysicalDamageType::Contondant, 100, 100, 15, 8};
    attacker.weapon = hammer;
    result = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(result != -99);
}

void test_stat_penalties_rounding() {
    Entity fighter("Fighter");
    fighter.force = 10.25f;
    fighter.vitesse = 10.0f;
    
    // No wounds: stats should be normal
    assert(std::abs(fighter.getEffectiveForce() - 10.25f) < 0.001f);
    assert(std::abs(fighter.getEffectiveVitesse() - 10.0f) < 0.001f);

    // Apply a Stade 3 wound (-15% force & vitesse)
    fighter.applyWound(3, PhysicalDamageType::Contondant);
    // Effective force = 10.25 * 0.85 = 8.7125. Rounded up to 0.25: 8.75.
    assert(std::abs(fighter.getEffectiveForce() - 8.75f) < 0.001f);
    // Effective vitesse = 10.0 * 0.85 = 8.5. Rounded up: 8.5.
    assert(std::abs(fighter.getEffectiveVitesse() - 8.5f) < 0.001f);

    // Apply a Stade 4 wound (-30% force & vitesse)
    fighter.wounds.clear();
    fighter.applyWound(4, PhysicalDamageType::Contondant);
    // Effective force = 10.25 * 0.70 = 7.175. Rounded up to 0.25: 7.25.
    assert(std::abs(fighter.getEffectiveForce() - 7.25f) < 0.001f);
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
    assert(defender.physicalReserve == 90.0f); // 100 - 10

    // Execute attack: should be evaded (-98)
    int result = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(result == -98);
    assert(defender.activeDodges == 0);
    assert(defender.wounds.empty());

    // Test Parry: prepare parry
    CombatSystem::executeParry(defender, 1.0f);
    assert(defender.activeParries == 1);
    assert(defender.physicalReserve == 82.5f); // 90 - 7.5

    // Let's make attacker stronger so they deal positive damage
    attacker.force = 15.0f; // diff at level 1: 5.0 -> effectiveness 5 (Domination)
    Weapon lightWeapon{"Light Sword", WeaponWeight::Leger, PhysicalDamageType::Tranchant, 100, 100, 10, 5};
    attacker.weapon = lightWeapon;
    // Execute attack: should be parried (effectiveness 5 reduced by 10% -> 4)
    result = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(result == 4);
    assert(defender.activeParries == 0);
    assert(defender.wounds.size() == 1);
    assert(defender.wounds[0].effectiveness == 4);

    // Test Parry with metal shield vs Tranchant
    defender.wounds.clear();
    defender.passives.push_back("Bouclier en métal");
    CombatSystem::executeParry(defender, 1.0f);
    
    Weapon sword{"Sword", WeaponWeight::Moyen, PhysicalDamageType::Tranchant, 100, 100, 15, 8};
    attacker.weapon = sword;
    
    result = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(result == -97); // Parade bloquée
    assert(defender.activeParries == 0);
    assert(defender.wounds.empty());
}

void test_equipment_loading() {
    assert(DataStore::getInstance().loadWeapons("data/Equipement/Arme"));
    assert(DataStore::getInstance().loadArmors("data/Equipement/Armure"));

    auto weapons = DataStore::getInstance().getAvailableWeaponNames();
    assert(!weapons.empty());
    
    // Check espadon
    auto espadon = DataStore::getInstance().getWeaponTemplate("Espadon");
    assert(espadon.has_value());
    assert(espadon->name == "Espadon");
    assert(espadon->type == WeaponWeight::Lourd);
    assert(espadon->damageType == PhysicalDamageType::Tranchant);
    assert(espadon->durability == 150);

    auto armors = DataStore::getInstance().getAvailableArmorNames();
    assert(!armors.empty());
    auto maille = DataStore::getInstance().getArmorTemplate("cote de mailles en parbélienne");
    assert(maille.has_value());
    assert(maille->name == "cote de mailles en parbélienne");
    assert(maille->material == ArmorMaterial::Mineral);
    assert(maille->durability == 36);
}

void test_weapon_durability_and_multipliers() {
    Entity attacker("Attacker");
    Entity defender("Defender");
    attacker.stade = 1;
    defender.stade = 1;
    attacker.force = 10.0f;
    defender.resistance = 10.0f;
    attacker.maxPhysicalReserve = 100.0f;
    attacker.physicalReserve = 100.0f;

    // Test 1: Bare-hands (no weapon, not monster)
    // Damage multiplier = x0.95. raw effectiveness = 0. 0 * 0.95 = 0.
    // Stamina cost = 7.5
    int res = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(res == 0);
    assert(std::abs(attacker.physicalReserve - 92.5f) < 0.001f);

    // Test 2: Monster bare-hands
    // Damage multiplier = x1.0. raw effectiveness = 0. 0 * 1 = 0.
    // Stamina cost = 7.5
    attacker.isMonster = true;
    attacker.physicalReserve = 100.0f;
    res = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(res == 0);
    assert(std::abs(attacker.physicalReserve - 92.5f) < 0.001f);
    attacker.isMonster = false;

    // Test 3: Weapon Durability reduction
    // Weapon has res = 10. Attacker has force = 10.
    // refWeaponLevel = (10 < 10) ? 1 : 1 -> 1.
    // eff_weapon = calculateStatDifference(10, 10, 1) = 0.
    // durabilityLoss for eff_weapon=0 is 5.
    // Weapon starts at 10 durability. It should end at 5.
    Weapon weakWeapon{"Weak Sword", WeaponWeight::Leger, PhysicalDamageType::Tranchant, 10, 10, 10, 5};
    attacker.weapon = weakWeapon;
    attacker.physicalReserve = 100.0f;
    res = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(attacker.weapon->durability == 5);
    assert(std::abs(attacker.physicalReserve - 92.5f) < 0.001f);

    // Test 4: Weapon Breakage (durability falls to 0)
    // Next attack: eff_weapon = 0. durabilityLoss = 5.
    // Durability should fall to 0.
    res = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(attacker.weapon->durability == 0);

    // Test 5: Revert to bare-hands when broken
    // Now that weapon has 0 durability:
    // Stamina cost should be 7.5 (Pugilat/bare-hands) instead of 12.5 (if it was a lourd weapon)
    // Damage multiplier should be x0.95.
    Weapon brokenHeavy{"Broken Greatsword", WeaponWeight::Lourd, PhysicalDamageType::Tranchant, 0, 150, 20, 10};
    attacker.weapon = brokenHeavy;
    attacker.physicalReserve = 100.0f;
    // Attack: cost should be 7.5 because weapon is broken!
    res = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(std::abs(attacker.physicalReserve - 92.5f) < 0.001f); // 100 - 7.5 = 92.5
}

int main() {
    // 1. Initialiser le DataStore avec les fichiers JSON
    if (!DataStore::getInstance().loadSystemData("data/diff_stats.json")) {
        std::cerr << "Failed to load system data" << std::endl;
        return 1;
    }
    if (!DataStore::getInstance().loadEnergySystem("data/energy_system.json")) {
        std::cerr << "Failed to load energy system" << std::endl;
        return 1;
    }

    std::cout << "Starting unit tests..." << std::endl;

    RUN_TEST(test_wound_stacking);
    RUN_TEST(test_death_rules);
    RUN_TEST(test_bleeding_rates);
    RUN_TEST(test_speed_free_actions);
    RUN_TEST(test_stamina_costs);
    RUN_TEST(test_armor_blocking);
    RUN_TEST(test_stat_penalties_rounding);
    RUN_TEST(test_parry_dodge);
    RUN_TEST(test_equipment_loading);
    RUN_TEST(test_weapon_durability_and_multipliers);

    std::cout << "All core unit tests passed successfully!" << std::endl;
    return 0;
}
