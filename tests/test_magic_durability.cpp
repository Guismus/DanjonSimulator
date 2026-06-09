#include "../back/core/entity.hpp"
#include "../back/systems/CombatSystem.hpp"
#include "../back/core/simulator.hpp"
#include "../back/data/DataStore.hpp"
#include <cassert>
#include <cmath>
#include <print>

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
    mage1.magicType = "Bal des lucioles";
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
    
    // Mage 1 should have consumed 15 mana
    assert(sim.getFighter1()->magicReserve == 15.0f);
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
    mage2.magicReserve = 50.0f;
    mage2.magicType = "Pouls du tonnerre";
    mage2.resistanceMagique = 10.0f;
    mage2.maxPhysicalReserve = 100.0f;
    mage2.physicalReserve = 100.0f;
    
    sim.startCombat(mage2, target, ControlMode::Manual, ControlMode::Manual);
    sim.addActionP1(ActionType::Magic);
    sim.setP1Finished(true);
    sim.setP2Finished(true);
    turnRes = sim.resolveTurn();

    // Verify Pouls du tonnerre was cast and logged
    bool foundLog = false;
    for (const auto& log : turnRes.logs) {
        if (log.find("Pouls du tonnerre") != std::string::npos) {
            foundLog = true;
        }
    }
    assert(foundLog);

    // Mage 2 force remains untouched (10.0f)
    // Mage 2 speed and r_mag were boosted but reverted back to their base values (10.0f) since duration is 1 turn
    assert(sim.getFighter1()->magicReserve == 10.0f);
    assert(sim.getFighter1()->force == 10.0f);
    assert(sim.getFighter1()->vitesse == 10.0f);
    assert(sim.getFighter1()->resistanceMagique == 10.0f);

    // Test C: Magic Soins (healing wounds)
    Entity mage3("Mage 3");
    mage3.stade = 1;
    mage3.vitesse = 10.0f;
    mage3.forceMagique = 15.0f;
    mage3.magicReserve = 30.0f;
    mage3.magicType = "Bal des lucioles";
    mage3.maxPhysicalReserve = 100.0f;
    mage3.physicalReserve = 100.0f;
    mage3.blood = 20.0f;

    // Apply multiple wounds
    mage3.applyWound(-3, DamageType::Contondant); // Healed by <= 2 (Moyen)
    mage3.applyWound(-1, DamageType::Contondant); // Healed by <= 2 (Moyen)
    mage3.applyWound(4, DamageType::Contondant);  // Needs Extreme or Très Fort

    sim.startCombat(mage3, target, ControlMode::Manual, ControlMode::Manual);
    sim.addActionP1(ActionType::Magic);
    sim.setP1Finished(true);
    sim.setP2Finished(true);
    sim.resolveTurn();

    // Remaining wounds should only be the ones with effectiveness > 2, which is effectiveness 4.
    assert(sim.getFighter1()->wounds.size() == 1);
    assert(sim.getFighter1()->wounds[0].effectiveness == 4);
    assert(sim.getFighter1()->magicReserve == 15.0f);

    // Test D: Extreme Healing restores all wounds and blood
    Entity mage4("Mage 4");
    mage4.stade = 1;
    mage4.vitesse = 10.0f;
    mage4.forceMagique = 36.0f;
    mage4.magicReserve = 30.0f;
    mage4.magicType = "Eaux maternelles";
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
    assert(sim.getFighter1()->magicReserve == 5.0f);
}

void test_eaux_maternelles() {
    Simulator sim;
    Entity mage("Mage");
    Entity target("Target");

    mage.stade = 1;
    target.stade = 1;
    mage.vitesse = 20.0f;
    target.vitesse = 10.0f;
    mage.magicReserve = 30.0f;
    mage.forceMagique = 15.0f;
    mage.magicType = "Eaux maternelles";
    mage.blood = 15.0f;
    mage.maxPhysicalReserve = 100.0f;
    mage.physicalReserve = 100.0f;

    target.maxPhysicalReserve = 100.0f;
    target.physicalReserve = 100.0f;
    target.vitesse = 10.0f;

    // Apply a wound to Mage so we can test the extreme heal
    mage.applyWound(3, DamageType::Tranchant);
    assert(mage.wounds.size() == 1);
    assert(mage.blood == 15.0f);

    // Start combat - P1 (Mage) vs P2 (Target)
    sim.startCombat(mage, target, ControlMode::Manual, ControlMode::Manual);

    // Turn 1:
    // Mage casts Eaux maternelles (Magic)
    // Mage also attempts to Attack (to test that subsequent actions in the same turn are skipped once the bubble is up)
    sim.addActionP1(ActionType::Magic);
    sim.addActionP1(ActionType::Attack);

    // Target attacks Mage
    sim.addActionP2(ActionType::Attack);

    sim.setP1Finished(true);
    sim.setP2Finished(true);

    assert(sim.checkResolve());
    Simulator::TurnResult res1 = sim.resolveTurn();

    // Verify Mage:
    // 1. Extreme healing has occurred: wounds empty, blood restored to 32.0f
    assert(sim.getFighter1()->wounds.empty());
    assert(sim.getFighter1()->blood == 32.0f);
    // 2. Magic cost was paid (30.0f - 25.0f = 5.0f)
    assert(sim.getFighter1()->magicReserve == 5.0f);
    // 3. Bubble is active. After turn end decrement, it should be 1 (started at 2 during action, then decremented by 1 at turn end)
    assert(sim.getFighter1()->invulnerableTurnsLeft == 1);

    // Verify Target:
    // 1. Target's attack against Mage was blocked by bubble (logged as "glisse sur sa protection d'invulnérabilité")
    bool bubbleBlockedLogFound = false;
    bool actionSkippedLogFound = false;
    for (const auto& log : res1.logs) {
        if (log.find("glisse sur sa protection") != std::string::npos) {
            bubbleBlockedLogFound = true;
        }
        if (log.find("Incapacité (Invulnérabilité active") != std::string::npos) {
            actionSkippedLogFound = true;
        }
    }
    assert(bubbleBlockedLogFound);
    assert(actionSkippedLogFound);

    // Turn 2: Mage's bubble is at 1.
    // Let's queue an action for Mage (even though UI disables it, the backend should block/skip it if queued)
    // and an attack for Target
    sim.addActionP1(ActionType::Attack);
    sim.addActionP2(ActionType::Attack);
    sim.setP1Finished(true);
    sim.setP2Finished(true);

    Simulator::TurnResult res2 = sim.resolveTurn();

    // Verify that during Turn 2:
    // 1. Mage's action was skipped again
    // 2. Target's attack was blocked again
    // 3. At the end of Turn 2, bubble expires (bubbleTurnsLeft becomes 0)
    assert(sim.getFighter1()->invulnerableTurnsLeft == 0);

    bool res2BlockedFound = false;
    bool res2SkippedFound = false;
    for (const auto& log : res2.logs) {
        if (log.find("glisse sur sa protection") != std::string::npos) {
            res2BlockedFound = true;
        }
        if (log.find("Incapacité (Invulnérabilité active") != std::string::npos) {
            res2SkippedFound = true;
        }
    }
    assert(res2BlockedFound);
    assert(res2SkippedFound);

    // Turn 3: Bubble is now 0.
    // Mage can attack, and Target can attack normally
    sim.addActionP1(ActionType::Attack);
    sim.addActionP2(ActionType::Attack);
    sim.setP1Finished(true);
    sim.setP2Finished(true);

    Simulator::TurnResult res3 = sim.resolveTurn();
    
    // Bubble is still 0
    assert(sim.getFighter1()->invulnerableTurnsLeft == 0);

    // Verify that actions were NOT skipped or blocked by bubble
    for (const auto& log : res3.logs) {
        assert(log.find("glisse sur sa protection") == std::string::npos);
        assert(log.find("Incapacité (Invulnérabilité active") == std::string::npos);
    }
}

void test_magic_catalyst_selection() {
    Simulator sim;
    Entity fighter("Fighter");
    Entity target("Target");

    fighter.stade = 1;
    target.stade = 1;
    fighter.vitesse = 10.0f;
    target.vitesse = 10.0f;
    fighter.maxPhysicalReserve = 100.0f;
    fighter.physicalReserve = 100.0f;
    target.maxPhysicalReserve = 100.0f;
    target.physicalReserve = 100.0f;

    // Fighter has base magic: Eaux maternelles (30 mana)
    fighter.magicType = "Eaux maternelles";
    fighter.magicReserve = 30.0f;
    fighter.blood = 15.0f;

    // Fighter has catalyst: Pouls du tonnerre (100 reserve)
    Catalyst cat;
    cat.magicType = "Pouls du tonnerre";
    cat.reserve = 100;
    cat.power = 10;
    fighter.catalyst = cat;
    fighter.catalysts.push_back(cat);

    sim.startCombat(fighter, target, ControlMode::Manual, ControlMode::Manual);

    // 1. Cast catalyst spell (Pouls du tonnerre)
    sim.addActionP1(ActionType::Magic, "Pouls du tonnerre", true);
    sim.setP1Finished(true);
    sim.setP2Finished(true);
    auto res1 = sim.resolveTurn();


    // Verify catalyst reserve decreased (100 - 40 = 60)
    assert(sim.getFighter1()->catalyst.has_value());
    assert(sim.getFighter1()->catalyst->reserve == 60);
    // Base magicReserve remains untouched (30.0f)
    assert(sim.getFighter1()->magicReserve == 30.0f);
    // Wounds/blood remains untouched (not healed by Pouls du tonnerre)
    assert(sim.getFighter1()->blood == 15.0f);

    // 2. Cast base spell (Eaux maternelles, cost 25)
    // We queue: magicType = "Eaux maternelles", useCatalyst = false
    sim.addActionP1(ActionType::Magic, "Eaux maternelles", false);
    sim.setP1Finished(true);
    sim.setP2Finished(true);
    sim.resolveTurn();

    // Verify base magic reserve decreased (30 - 25 = 5)
    assert(sim.getFighter1()->magicReserve == 5.0f);
    // Catalyst reserve remains untouched (60)
    assert(sim.getFighter1()->catalyst->reserve == 60);
    // Blood restored to 32.0f by Eaux maternelles
    assert(sim.getFighter1()->blood == 32.0f);
    // Invulnerability turns left should be 1 (started at 2, decremented at turn end)
    assert(sim.getFighter1()->invulnerableTurnsLeft == 1);
}

void test_bal_des_lucioles() {
    assert(DataStore::getInstance().loadCatalysts("data/catalysts.json"));
    assert(DataStore::getInstance().loadEntities("data/entities"));
    
    auto agathaOpt = DataStore::getInstance().getEntityTemplate("Agatha Eterm");
    assert(agathaOpt.has_value());
    Entity agatha = *agathaOpt;
    assert(agatha.magicType == "Bal des lucioles");
    assert(agatha.magicReserve == 150.0f);
    
    Simulator sim;
    Entity target("Target");
    target.stade = agatha.stade;
    target.resistanceMagique = 10.0f;
    target.resistance = 10.0f;
    target.maxPhysicalReserve = 100.0f;
    target.physicalReserve = 100.0f;
    
    agatha.maxPhysicalReserve = 100.0f;
    agatha.physicalReserve = 100.0f;
    
    agatha.applyWound(2, DamageType::Contondant);
    assert(agatha.wounds.size() == 1);
    
    sim.startCombat(agatha, target, ControlMode::Manual, ControlMode::Manual);
    
    sim.addActionP1(ActionType::Magic, "Bal des lucioles", false);
    sim.addActionP1(ActionType::Attack);
    
    sim.setP1Finished(true);
    sim.setP2Finished(true);
    sim.resolveTurn();
    
    assert(sim.getFighter1()->magicReserve == 135.0f);
    
    bool targetHasFireWound = false;
    for (const auto& w : sim.getFighter2()->wounds) {
        if (w.damageType == DamageType::Feu) {
            targetHasFireWound = true;
        }
    }
    assert(targetHasFireWound);
    assert(sim.getFighter1()->wounds.empty());
    assert(!sim.getFighter1()->balDesLuciolesActive);
}

void test_generic_duration_heal_and_boost() {
    // Test continuous healing
    Entity tester("Tester");
    tester.blood = 10.0f;
    tester.applyWound(3, DamageType::Contondant);
    
    // Add active heal of Moyen power for 2 turns
    ActiveEffect ae;
    ae.type = "heal";
    ae.duration = 2;
    ae.power = "moyen";
    ae.spellName = "Regen";
    tester.activeEffects.push_back(ae);
    
    std::vector<std::string> logs;
    // Tick 1
    tester.updateActiveEffects(logs);
    tester.decrementActiveEffects(logs);
    
    // Wound of 3 shouldn't be healed by Moyen (<= 2). Wound of 3 is still there.
    assert(tester.wounds.size() == 1);
    assert(tester.activeEffects.size() == 1); // duration becomes 1
    
    // Apply a wound of 1
    tester.applyWound(1, DamageType::Contondant);
    assert(tester.wounds.size() == 2);
    
    // Tick 2
    tester.updateActiveEffects(logs);
    tester.decrementActiveEffects(logs);
    
    // The wound of 1 should be healed by Moyen. Wound of 3 remains.
    assert(tester.wounds.size() == 1);
    assert(tester.wounds[0].effectiveness == 3);
    assert(tester.activeEffects.empty()); // expired

    // Test temporary boost
    tester.force = 10.0f;
    tester.vitesse = 10.0f;
    
    ActiveEffect aeBoost;
    aeBoost.type = "boost";
    aeBoost.duration = 2;
    aeBoost.forceBoost = 5.0f;
    aeBoost.speedBoost = 3.0f;
    aeBoost.spellName = "TempBoost";
    
    tester.applyStatBoost(aeBoost.forceBoost, aeBoost.speedBoost);
    tester.activeEffects.push_back(aeBoost);
    
    assert(tester.force == 15.0f);
    assert(tester.vitesse == 13.0f);
    
    tester.updateActiveEffects(logs);
    tester.decrementActiveEffects(logs);
    
    assert(tester.force == 15.0f);
    assert(tester.vitesse == 13.0f);
    assert(tester.activeEffects.size() == 1);
    
    tester.updateActiveEffects(logs);
    tester.decrementActiveEffects(logs);
    
    // Boost reverted after duration expires
    assert(tester.force == 10.0f);
    assert(tester.vitesse == 10.0f);
    assert(tester.activeEffects.empty());
}

void test_races() {
    // 1. Marwoeth bleeding immunity
    Entity marwoeth("Marwoeth Tester");
    marwoeth.race = "Marwoeth";
    marwoeth.racePassives = {"no_bleeding"};
    marwoeth.blood = 32.0f;
    
    // Apply a severe bleeding trigger of 3 points
    marwoeth.applyBleeding(3);
    assert(marwoeth.blood == 32.0f); // Should not decrease since Marwoeth is immune to bleeding

    // Apply a tranchant wound which usually causes bleeding
    marwoeth.applyWound(3, DamageType::Tranchant);
    assert(marwoeth.getBleedingRate() == 0); // Bleeding rate should remain 0
    
    // 2. Jiaodan dragon transformation
    Entity jiaodan("Jiaodan Tester");
    jiaodan.race = "Jiaodan";
    jiaodan.racePassives = {"dragon_transform"};
    jiaodan.dragonStats = {
        {"force", 40.0f},
        {"resistance", 35.0f},
        {"vitesse", 20.0f},
        {"forceMagique", 30.0f},
        {"resistanceMagique", 30.0f}
    };
    jiaodan.force = 10.0f;
    jiaodan.resistance = 10.0f;
    jiaodan.vitesse = 10.0f;
    jiaodan.forceMagique = 10.0f;
    jiaodan.resistanceMagique = 10.0f;
    jiaodan.magicReserve = 100.0f;
    jiaodan.maxPhysicalReserve = 100.0f;
    jiaodan.physicalReserve = 100.0f;
    
    Simulator sim;
    Entity target("Target");
    target.stade = 1;
    target.vitesse = 5.0f;
    target.maxPhysicalReserve = 100.0f;
    target.physicalReserve = 100.0f;
    
    sim.startCombat(jiaodan, target, ControlMode::Manual, ControlMode::Manual);
    // Queue dragon transform spell
    sim.addActionP1(ActionType::Magic, "Transformation en dragon", false);
    sim.setP1Finished(true);
    sim.setP2Finished(true);
    sim.resolveTurn();
    
    // Check that stats are replaced by dragon stats
    assert(sim.getFighter1()->isTransformed);
    assert(sim.getFighter1()->force == 40.0f);
    assert(sim.getFighter1()->resistance == 35.0f);
    assert(sim.getFighter1()->vitesse == 20.0f);
    assert(sim.getFighter1()->forceMagique == 30.0f);
    assert(sim.getFighter1()->resistanceMagique == 30.0f);

    // 3. Test overriding dragon stats in DataStore from JSON
    std::string testJson = R"({
        "name": "Custom Jiaodan",
        "race": "Jiaodan",
        "dragon_stats": {
            "force": 50.0,
            "vitesse": 25.0
        }
    })";
    
    nlohmann::json j = nlohmann::json::parse(testJson);
    Entity testEntity("Custom Jiaodan");
    testEntity.race = j.value("race", "");
    auto raceOpt = DataStore::getInstance().getRace(testEntity.race);
    assert(raceOpt.has_value());
    testEntity.racePassives = raceOpt->passives;
    testEntity.dragonStats = raceOpt->dragonStats;
    
    if (j.contains("dragon_stats") && j["dragon_stats"].is_object()) {
        for (auto it = j["dragon_stats"].begin(); it != j["dragon_stats"].end(); ++it) {
            testEntity.dragonStats[it.key()] = it.value().get<float>();
        }
    }
    
    assert(testEntity.racePassives.size() == 1);
    assert(testEntity.racePassives[0] == "dragon_transform");
    assert(testEntity.dragonStats["force"] == 50.0f); // Overridden!
    assert(testEntity.dragonStats["vitesse"] == 25.0f); // Overridden!
    assert(testEntity.dragonStats["resistance"] == 35.0f); // Fallback from race default!

    // Verify loaded files from data/entities/
    auto lucielaOpt = DataStore::getInstance().getEntityTemplate("Luciela Taisetsu");
    assert(lucielaOpt.has_value());
    assert(lucielaOpt->race == "Jiaodan");
    assert(lucielaOpt->force == 13.00f);
    assert(lucielaOpt->vitesse == 22.75f);
    assert(lucielaOpt->dragonStats.at("force") == 22.00f);
    assert(lucielaOpt->dragonStats.at("resistance") == 19.50f);
    assert(lucielaOpt->dragonStats.at("vitesse") == 17.75f);

    auto haineOpt = DataStore::getInstance().getEntityTemplate("Haïné Sukehira");
    assert(haineOpt.has_value());
    assert(haineOpt->race == "Jiaodan");
    assert(haineOpt->force == 12.25f);
    assert(haineOpt->dragonStats.at("force") == 21.00f);
    assert(haineOpt->dragonStats.at("forceMagique") == 26.25f);
}


