#include "back/data/DataStore.hpp"
#include "back/systems/CombatSystem.hpp"
#include "back/core/entity.hpp"
#include <iostream>
#include <vector>

void applyAttack(Entity& attacker, Entity& defender, int actionIndex, int freeActions) {
    float multipliers[] = {2.0f, 2.3f, 2.6f, 3.4f, 5.0f, 7.0f};
    float multiplier = 1.0f;
    if (actionIndex >= freeActions) {
        int idx = actionIndex - freeActions;
        if (idx > 5) idx = 5;
        multiplier = multipliers[idx];
    }
    
    std::cout << attacker.getName() << " attacks action " << (actionIndex + 1) 
              << " mult=" << multiplier << std::endl;
    
    int eff = CombatSystem::executeAttack(attacker, defender, multiplier);
    std::cout << "  result: " << eff << " defender dead=" << defender.isDead() 
              << " defender reserve=" << defender.physicalReserve 
              << " attacker dead=" << attacker.isDead() 
              << " attacker reserve=" << attacker.physicalReserve << std::endl;
}

int main() {
    // Load data
    if (!DataStore::getInstance().loadSystemData("data/diff_stats.json")) {
        std::cerr << "Failed to load diff_stats" << std::endl;
        return 1;
    }
    if (!DataStore::getInstance().loadEnergySystem("data/energy_system.json")) {
        std::cerr << "Failed to load energy_system" << std::endl;
        return 1;
    }
    if (!DataStore::getInstance().loadArmors("data/Equipement/Armure")) {
        std::cerr << "Failed to load armors" << std::endl;
        return 1;
    }
    if (!DataStore::getInstance().loadEntities("data/entities")) {
        std::cerr << "Failed to load entities" << std::endl;
        return 1;
    }
    
    auto opt1 = DataStore::getInstance().getEntityTemplate("Agatha Eterm");
    auto opt2 = DataStore::getInstance().getEntityTemplate("Agatha Eterm");
    
    if (!opt1 || !opt2) {
        std::cerr << "Agatha Eterm not found" << std::endl;
        return 1;
    }
    
    Entity fighter1 = *opt1;
    Entity fighter2 = *opt2;
    
    // Simulate 50 queued attacks
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
    
    std::cout << "Starting simulation..." << std::endl;
    while ((firstDone < firstQueued || secondDone < secondQueued) && !first->isDead() && !second->isDead()) {
        if (firstDone < firstQueued) {
            if (!first->isDead() && first->physicalReserve > 0) {
                applyAttack(*first, *second, firstDone, firstFree);
            }
            firstDone++;
        }
        if (second->isDead() || first->isDead()) break;
        
        if (secondDone < secondQueued) {
            if (!second->isDead() && second->physicalReserve > 0) {
                applyAttack(*second, *first, secondDone, secondFree);
            }
            secondDone++;
        }
        if (first->isDead() || second->isDead()) break;
    }
    
    std::cout << "Simulation finished. fighter1 dead=" << fighter1.isDead() 
              << " fighter2 dead=" << fighter2.isDead() 
              << " fighter1 KO/exhausted=" << (fighter1.physicalReserve <= 0)
              << " fighter2 KO/exhausted=" << (fighter2.physicalReserve <= 0) << std::endl;
    return 0;
}
