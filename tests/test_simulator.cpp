#include "../back/core/entity.hpp"
#include "../back/systems/CombatSystem.hpp"
#include "../back/core/simulator.hpp"
#include "../back/data/DataStore.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

void test_simulator() {
    Simulator sim;
    Entity f1("F1");
    Entity f2("F2");
    f1.stade = 1;
    f2.stade = 1;
    f1.vitesse = 10.0f;
    f2.vitesse = 10.0f;
    f1.blood = 32.0f;
    f1.maxPhysicalReserve = 100.0f;
    f1.physicalReserve = 100.0f;
    f2.blood = 32.0f;
    f2.maxPhysicalReserve = 100.0f;
    f2.physicalReserve = 100.0f;
    
    sim.startCombat(f1, f2, ControlMode::Manual, ControlMode::Manual);
    
    assert(sim.getFighter1().has_value());
    assert(sim.getFighter2().has_value());
    assert(sim.getCurrentTurn() == 1);
    assert(!sim.isP1Finished());
    assert(!sim.isP2Finished());
    
    // Add manual actions
    sim.addActionP1(ActionType::Attack);
    assert(sim.getP1Actions().size() == 1);
    assert(sim.getP1Actions()[0].type == ActionType::Attack);
    
    sim.popActionP1();
    assert(sim.getP1Actions().empty());
    
    // Test overclocking calculations
    sim.addActionP1(ActionType::Attack); // 1st free
    sim.addActionP1(ActionType::Attack); // 2nd free
    sim.addActionP1(ActionType::Attack); // 3rd (overclocked)
    
    assert(sim.getP1Actions().size() == 3);
    assert(sim.getP1Actions()[0].overclockMultiplier == 1.0f);
    assert(sim.getP1Actions()[1].overclockMultiplier == 1.0f);
    assert(sim.getP1Actions()[2].overclockMultiplier == 2.0f);
    
    // Test resolve
    sim.setP1Finished(true);
    sim.setP2Finished(true);
    assert(sim.checkResolve());
    
    Simulator::TurnResult result = sim.resolveTurn();
    assert(!result.logs.empty());
    assert(sim.getCurrentTurn() == 2);
}

static void applyAttackSim(Entity& attacker, Entity& defender, int actionIndex, int freeActions) {
    float multipliers[] = {2.0f, 2.3f, 2.6f, 3.4f, 5.0f, 7.0f};
    float multiplier = 1.0f;
    if (actionIndex >= freeActions) {
        int idx = actionIndex - freeActions;
        if (idx > 5) idx = 5;
        multiplier = multipliers[idx];
    }
    
    CombatSystem::executeAttack(attacker, defender, multiplier);
}

void test_overclock_simulation() {
    // Load templates if they weren't loaded yet
    DataStore::getInstance().loadSystemData("data/diff_stats.json");
    DataStore::getInstance().loadEnergySystem("data/energy_system.json");
    DataStore::getInstance().loadArmors("data/Equipement/Armure");
    DataStore::getInstance().loadEntities("data/entities");

    auto opt1 = DataStore::getInstance().getEntityTemplate("Agatha Eterm");
    auto opt2 = DataStore::getInstance().getEntityTemplate("Agatha Eterm");
    
    if (!opt1 || !opt2) {
        std::cerr << "[WARNING] Agatha Eterm not found in entities folder, skipping overclock simulation test." << std::endl;
        return;
    }
    
    Entity fighter1 = *opt1;
    Entity fighter2 = *opt2;
    
    int firstQueued = 50;
    int secondQueued = 0;
    float v1 = fighter1.getEffectiveVitesse();
    float v2 = fighter2.getEffectiveVitesse();
    int refLevel = std::min(fighter1.stade, fighter2.stade);
    int speedDiff = CombatSystem::calculateStatDifference(v1, v2, refLevel);
    int firstFree = 2 + (speedDiff > 0 ? speedDiff : 0);
    int secondFree = 2 + (speedDiff < 0 ? -speedDiff : 0);
    
    Entity* first = &fighter1;
    Entity* second = &fighter2;
    
    int firstDone = 0;
    int secondDone = 0;
    
    std::cout << "DEBUG Overclock simulation:" << std::endl;
    std::cout << "  v1=" << v1 << ", v2=" << v2 << ", firstFree=" << firstFree << ", secondFree=" << secondFree << std::endl;
    std::cout << "  fighter1 force=" << fighter1.force << ", resistance=" << fighter1.resistance << ", physicalReserve=" << fighter1.physicalReserve << std::endl;
    std::cout << "  fighter2 force=" << fighter2.force << ", resistance=" << fighter2.resistance << ", physicalReserve=" << fighter2.physicalReserve << std::endl;

    while ((firstDone < firstQueued || secondDone < secondQueued) && !first->isDead() && !second->isDead()) {
        if (firstDone < firstQueued) {
            if (!first->isDead() && first->physicalReserve > 0) {
                applyAttackSim(*first, *second, firstDone, firstFree);
            }
            firstDone++;
        }
        if (second->isDead() || first->isDead()) break;
        
        if (secondDone < secondQueued) {
            if (!second->isDead() && second->physicalReserve > 0) {
                applyAttackSim(*second, *first, secondDone, secondFree);
            }
            secondDone++;
        }
        if (first->isDead() || second->isDead()) break;
    }
}
