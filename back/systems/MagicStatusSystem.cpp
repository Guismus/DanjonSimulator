#include "MagicStatusSystem.hpp"
#include <iostream>

void StatusSystem::applyBleed(Entity& target, int severity) {
    // Bleed levels: 1 = Faible (-1), 2 = Moyen (-2), 3 = Fort (-5)
    int damage = 0;
    if (severity == 1) damage = 1;
    else if (severity == 2) damage = 2;
    else if (severity >= 3) damage = 5;

    target.applyBleeding(damage);
    std::cout << target.getName() << " bleeds for " << damage << " points." << std::endl;
}

void StatusSystem::processTurnEnd(Entity& target) {
    // Process bleeding over time or other effects
}

void MagicSystem::executeOffensiveMagic(Entity& caster, Entity& target) {
    int magicPower = caster.forceMagique;
    if (caster.catalyst.has_value()) {
        magicPower += caster.catalyst.value().power;
    }
    std::cout << caster.getName() << " casts offensive magic on " << target.getName() << " with power " << magicPower << "!" << std::endl;
}

void MagicSystem::executeBoostMagic(Entity& caster, const std::string& statToBoost) {
    std::cout << caster.getName() << " boosts " << statToBoost << "!" << std::endl;
}

void MagicSystem::executeHealMagic(Entity& caster, Entity& target, int level) {
    std::cout << caster.getName() << " heals " << target.getName() << " (Level " << level << ")!" << std::endl;
}
