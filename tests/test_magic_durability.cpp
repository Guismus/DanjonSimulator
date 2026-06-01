#include "../back/core/entity.hpp"
#include "../back/systems/CombatSystem.hpp"
#include "../back/core/simulator.hpp"
#include <cassert>
#include <cmath>

void test_armor_blocking() {
    Entity attacker("Attacker");
    Entity defender("Defender");
    
    attacker.stade = 1;
    defender.stade = 1;
    attacker.force = 10.0f;
    defender.resistance = 10.0f;

    Armor mineralArmor{"Mineral Armor", ArmorMaterial::Mineral, 100, 100, 10, 0};
    defender.armor = mineralArmor;

    Weapon sword{"Sword", WeaponWeight::Moyen, DamageType::Tranchant, 100, 100, 15, 8};
    attacker.weapon = sword;
    
    int result = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(result == -99);
    assert(defender.wounds.empty());

    Weapon hammer{"Hammer", WeaponWeight::Moyen, DamageType::Contondant, 100, 100, 15, 8};
    attacker.weapon = hammer;
    result = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(result != -99);
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

    int res = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(res == 0);
    assert(std::abs(attacker.physicalReserve - 92.5f) < 0.001f);

    attacker.isMonster = true;
    attacker.physicalReserve = 100.0f;
    res = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(res == 0);
    assert(std::abs(attacker.physicalReserve - 92.5f) < 0.001f);
    attacker.isMonster = false;

    Weapon weakWeapon{"Weak Sword", WeaponWeight::Leger, DamageType::Tranchant, 10, 10, 10, 5};
    attacker.weapon = weakWeapon;
    attacker.physicalReserve = 100.0f;
    res = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(attacker.weapon->durability == 5);
    assert(std::abs(attacker.physicalReserve - 92.5f) < 0.001f);

    res = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(attacker.weapon->durability == 0);

    Weapon brokenHeavy{"Broken Greatsword", WeaponWeight::Lourd, DamageType::Tranchant, 0, 150, 20, 10};
    attacker.weapon = brokenHeavy;
    attacker.physicalReserve = 100.0f;
    res = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(std::abs(attacker.physicalReserve - 92.5f) < 0.001f);
}

void test_arachnee_armor_wear() {
    Entity attacker("Attacker");
    attacker.setClass("ARACHNEE");
    Entity defender("Defender");
    
    attacker.stade = 1;
    defender.stade = 1;
    attacker.force = 10.0f;
    defender.resistance = 10.0f;

    Armor leatherArmor{"Leather Armor", ArmorMaterial::Peau, 10, 10, 10, 0};
    defender.armor = leatherArmor;

    Weapon hammer{"Hammer", WeaponWeight::Moyen, DamageType::Contondant, 100, 100, 15, 8};
    attacker.weapon = hammer;

    int result = CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(defender.armor->durability == 0);
}

void test_fire_resistance() {
    Entity attacker("Attacker");
    Entity defender1("Defender 1");
    attacker.stade = 1;
    defender1.stade = 1;
    attacker.force = 15.0f;
    defender1.resistance = 10.0f;
    
    Weapon fireSword{"Fire Sword", WeaponWeight::Leger, DamageType::Feu, 100, 100, 10, 5};
    attacker.weapon = fireSword;

    int result1 = CombatSystem::executeAttack(attacker, defender1, 1.0f);
    assert(result1 == 5);

    Entity defender2("Defender 2 (Forge Maître)");
    defender2.setClass("FORGEMAITRE");
    defender2.stade = 1;
    defender2.resistance = 10.0f;

    int result2 = CombatSystem::executeAttack(attacker, defender2, 1.0f);
    assert(result2 == 4);

    Entity defender3("Defender 3 (Resistant)");
    defender3.stade = 1;
    defender3.resistance = 10.0f;
    defender3.damageResistances[DamageType::Feu] = 0.20f;

    int result3 = CombatSystem::executeAttack(attacker, defender3, 1.0f);
    assert(result3 == 4);
}

void test_magical_slashing_damage() {
    Entity attacker("Mage Attacker");
    Entity defender("Fighter Defender");

    attacker.stade = 1;
    defender.stade = 1;
    attacker.forceMagique = 13.0f;
    attacker.force = 5.0f;
    
    defender.resistance = 10.0f;
    defender.resistanceMagique = 8.0f;

    Weapon sword{"Sword", WeaponWeight::Leger, DamageType::Tranchant, 100, 100, 10, 5};
    attacker.weapon = sword;
    
    int physicalRes = CombatSystem::executeAttack(attacker, defender, 1.0f, DamageNature::Physique);
    assert(physicalRes < 0);

    int magicalRes = CombatSystem::executeAttack(attacker, defender, 1.0f, DamageNature::Magique, DamageType::Tranchant);
    assert(magicalRes == 5); 
    
    assert(!defender.wounds.empty());
    assert(defender.wounds[0].effectiveness == 5);
    assert(defender.wounds[0].damageType == DamageType::Tranchant);
    assert(defender.getBleedingRate() == 3);
}

void test_advanced_wear_and_magic() {
    // 1. Wear rules
    // Slashing vs Fibre/Peau -> wear x2
    Entity attacker("Attacker");
    Entity defender("Defender");
    attacker.stade = 1;
    defender.stade = 1;
    attacker.force = 10.0f;
    defender.resistance = 10.0f;
    
    // Fibre armor, durability 20. Attacker force 10 vs armor res 10 -> diff 0 -> base wear 5.
    // Slashing vs Fibre -> wear x2 -> 10.
    Armor fibreArmor{"Fibre Armor", ArmorMaterial::Fibre, 20, 20, 10, 0};
    defender.armor = fibreArmor;
    Weapon sword{"Sword", WeaponWeight::Moyen, DamageType::Tranchant, 100, 100, 15, 8};
    attacker.weapon = sword;
    CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(defender.armor->durability == 10); // 20 - 10

    // Blunt vs Mineral -> wear x2.
    // Mineral armor, durability 20. Attacker force 10 vs armor res 10 -> diff 0 -> base wear 5.
    // Blunt vs Mineral -> wear x2 -> 10.
    Armor mineralArmor{"Mineral Armor", ArmorMaterial::Mineral, 20, 20, 10, 0};
    defender.armor = mineralArmor;
    Weapon hammer{"Hammer", WeaponWeight::Moyen, DamageType::Contondant, 100, 100, 15, 8};
    attacker.weapon = hammer;
    CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(defender.armor->durability == 10);

    // Ignition vs Fibre/Peau -> wear + 20.
    // Attacker force 10 vs armor res 10 -> base wear 5.
    // Fire weapon vs Fibre -> wear +20 -> 25.
    // Armor has durability 30.
    Armor fibreArmor2{"Fibre Armor 2", ArmorMaterial::Fibre, 30, 30, 10, 0};
    defender.armor = fibreArmor2;
    Weapon fireSword{"Fire Sword", WeaponWeight::Moyen, DamageType::Feu, 100, 100, 15, 8};
    attacker.weapon = fireSword;
    CombatSystem::executeAttack(attacker, defender, 1.0f);
    assert(defender.armor->durability == 5); // 30 - 25

    // Corrosion vs Mineral -> wear + 20.
    // Attacker force 10 vs armor res 10 -> base wear 5.
    // Corrosion vs Mineral -> wear +20 -> 25.
    Armor mineralArmor2{"Mineral Armor 2", ArmorMaterial::Mineral, 30, 30, 10, 0};
    defender.armor = mineralArmor2;
    // We use a custom attack call with Corrosion type
    CombatSystem::executeAttack(attacker, defender, 1.0f, DamageNature::Physique, DamageType::Corrosion);
    assert(defender.armor->durability == 5); // 30 - 25

    // 2. Magic Resolution tests
    Simulator sim;
    Entity mage1("Mage 1");
    Entity target("Target");
    mage1.stade = 1;
    target.stade = 1;
    mage1.vitesse = 10.0f;
    target.vitesse = 10.0f;
    mage1.magicReserve = 30.0f;
    mage1.forceMagique = 15.0f;
    target.resistanceMagique = 10.0f;
    mage1.blood = 32.0f;
    target.blood = 32.0f;
    mage1.maxPhysicalReserve = 100.0f;
    mage1.physicalReserve = 100.0f;
    target.maxPhysicalReserve = 100.0f;
    target.physicalReserve = 100.0f;

    // Test A: Offensive Magic (Feu)
    // Uses forceMagique 15 vs resistanceMagique 10 -> diff 5 -> Domination (5).
    Weapon staff{"Staff", WeaponWeight::Leger, DamageType::Neutre, 100, 100, 0, 0};
    mage1.weapon = staff;

    sim.startCombat(mage1, target, ControlMode::Manual, ControlMode::Manual);
    sim.addActionP1(ActionType::Magic);
    sim.setP1Finished(true);
    sim.setP2Finished(true);
    auto turnRes = sim.resolveTurn();
    
    // Mage 1 should have consumed 10 mana
    assert(sim.getFighter1()->magicReserve == 20.0f);
    // target should have a Feu wound of stage 5
    assert(sim.getFighter2()->wounds.size() == 1);
    assert(sim.getFighter2()->wounds[0].effectiveness == 5);
    assert(sim.getFighter2()->wounds[0].damageType == DamageType::Feu);

    // Test B: Magic Boost
    Entity mage2("Mage 2");
    mage2.stade = 1;
    mage2.vitesse = 10.0f;
    mage2.force = 10.0f;
    mage2.forceMagique = 10.0f;
    mage2.magicReserve = 20.0f;
    mage2.magicType = "Boost";
    mage2.maxPhysicalReserve = 100.0f;
    mage2.physicalReserve = 100.0f;
    
    sim.startCombat(mage2, target, ControlMode::Manual, ControlMode::Manual);
    sim.addActionP1(ActionType::Magic);
    sim.setP1Finished(true);
    sim.setP2Finished(true);
    sim.resolveTurn();

    // Mage 2 force and speed should be increased by 5
    assert(sim.getFighter1()->magicReserve == 10.0f);
    assert(sim.getFighter1()->force == 15.0f);
    assert(sim.getFighter1()->vitesse == 15.0f);

    // Test C: Magic Soins (healing wounds)
    Entity mage3("Mage 3");
    mage3.stade = 1;
    mage3.vitesse = 10.0f;
    mage3.forceMagique = 15.0f; // Soins Faible/Moyen threshold after wound penalty
    mage3.magicReserve = 30.0f;
    mage3.magicType = "Soins";
    mage3.maxPhysicalReserve = 100.0f;
    mage3.physicalReserve = 100.0f;
    mage3.blood = 20.0f;

    // Apply multiple wounds (avoiding combinations that trigger death: Stade 4 + Stade 0+)
    mage3.applyWound(-3, DamageType::Contondant); // Healed by <= 3
    mage3.applyWound(-1, DamageType::Contondant); // Healed by <= 3
    mage3.applyWound(4, DamageType::Contondant);  // Needs Extreme or Très Fort

    // With magic force 10, "Soins" default resolves wounds <= 2 (Moyen).
    sim.startCombat(mage3, target, ControlMode::Manual, ControlMode::Manual);
    sim.addActionP1(ActionType::Magic);
    sim.setP1Finished(true);
    sim.setP2Finished(true);
    sim.resolveTurn();

    // Remaining wounds should only be the ones with effectiveness > 2, which is effectiveness 4.
    // The wounds at -3, 0, and 2 should be healed.
    assert(sim.getFighter1()->wounds.size() == 1);
    assert(sim.getFighter1()->wounds[0].effectiveness == 4);

    // Test D: Extreme Healing restores all wounds and blood
    Entity mage4("Mage 4");
    mage4.stade = 1;
    mage4.vitesse = 10.0f;
    mage4.forceMagique = 36.0f; // Extreme healing threshold >= 25 after wound penalty
    mage4.magicReserve = 20.0f;
    mage4.magicType = "Soins";
    mage4.maxPhysicalReserve = 100.0f;
    mage4.physicalReserve = 100.0f;
    mage4.blood = 15.0f;
    mage4.applyWound(4, DamageType::Contondant);

    sim.startCombat(mage4, target, ControlMode::Manual, ControlMode::Manual);
    sim.addActionP1(ActionType::Magic);
    sim.setP1Finished(true);
    sim.setP2Finished(true);
    sim.resolveTurn();

    // Wounds should be empty, and blood should be restored to 32.0f
    assert(sim.getFighter1()->wounds.empty());
    assert(sim.getFighter1()->blood == 32.0f);
}
