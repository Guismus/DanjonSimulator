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
    Weapon lightWeapon{"Light Sword", WeaponType::Tranchant, WeaponWeight::Leger, 100, 0};
    attacker.weapon = lightWeapon;
    CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(std::abs(attacker.physicalReserve - (392.5f - 7.5f)) < 0.001f);

    // Equip medium weapon (cost = 10.0)
    Weapon medWeapon{"Mace", WeaponType::Contondant, WeaponWeight::Moyen, 100, 0};
    attacker.weapon = medWeapon;
    CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(std::abs(attacker.physicalReserve - (385.0f - 10.0f)) < 0.001f);

    // Equip heavy weapon (cost = 12.5)
    Weapon heavyWeapon{"Greatsword", WeaponType::Tranchant, WeaponWeight::Lourd, 100, 0};
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
    Weapon sword{"Sword", WeaponType::Tranchant, WeaponWeight::Moyen, 100, 0};
    attacker.weapon = sword;
    
    int result = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(result == -99); // -99 represents blocked
    assert(defender.wounds.empty());

    // Attack with contondant weapon (should hit)
    Weapon hammer{"Hammer", WeaponType::Contondant, WeaponWeight::Moyen, 100, 0};
    attacker.weapon = hammer;
    result = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(result != -99);
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

    std::cout << "All core unit tests passed successfully!" << std::endl;
    return 0;
}
