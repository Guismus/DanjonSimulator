#pragma once

#include "../core/entity.hpp"

class StatusSystem {
public:
    static void applyBleed(Entity& target, int severity);
    static void processTurnEnd(Entity& target);
};

class MagicSystem {
public:
    static void executeOffensiveMagic(Entity& caster, Entity& target);
    static void executeBoostMagic(Entity& caster, const std::string& statToBoost);
    static void executeHealMagic(Entity& caster, Entity& target, int level);
};
