#include "MagicStatusSystem.hpp"
#include <print>

void StatusSystem::applyBleed(Entity& target, int severity) {
    // Bleed levels: 1 = Faible (-1), 2 = Moyen (-2), 3 = Fort (-5)
    int damage = 0;
    if (severity == 1) damage = 1;
    else if (severity == 2) damage = 2;
    else if (severity >= 3) damage = 5;

    target.applyBleeding(damage);
    std::println("{} bleeds for {} points.", target.getName(), damage);
}

void StatusSystem::processTurnEnd(Entity& target) {
    // Process bleeding over time or other effects
}

void MagicSystem::executeOffensiveMagic(Entity& caster, Entity& target) {
    int magicPower = caster.forceMagique;
    if (caster.catalyst.has_value()) {
        magicPower += caster.catalyst.value().power;
    }
    std::println("{} casts offensive magic on {} with power {}!", caster.getName(), target.getName(), magicPower);
}

void MagicSystem::executeBoostMagic(Entity& caster, const std::string& statToBoost) {
    std::println("{} boosts {}!", caster.getName(), statToBoost);
}

void MagicSystem::executeHealMagic(Entity& caster, Entity& target, int level) {
    std::println("{} heals {} (Level {})!", caster.getName(), target.getName(), level);
}
