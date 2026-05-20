#include "entity.hpp"
#include <algorithm>

Entity::Entity(const std::string& name)
    : name(name), stade(1), rank(1),
      force(0.0f), resistance(0.0f), vitesse(0.0f), forceMagique(0.0f), resistanceMagique(0.0f),
      blood(32.0f), physicalReserve(0.0f), magicReserve(0.0f),
      weight(Weight::Moyen) {
}

const std::string& Entity::getName() const {
    return name;
}

void Entity::applyDamage(int damage) {
    // Combat system will handle damage stages, but we can store abstract damage if needed.
    // In this system, damage seems to be represented as stages (-7 to +7). 
    // +5 is dead.
    // For now we just implement the basic structure.
}

void Entity::applyBleeding(int severity) {
    blood -= severity;
    if (blood < 0) blood = 0;
}

bool Entity::isDead() const {
    return blood == 0;
}
