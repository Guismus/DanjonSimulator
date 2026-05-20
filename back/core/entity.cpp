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

void Entity::applyWound(int eff) {
    // Les blessures négatives sont également accumulées !
    // Combiner les blessures identiques : 2 Stade X = Stade X+2
    while (std::find(wounds.begin(), wounds.end(), eff) != wounds.end()) {
        wounds.erase(std::find(wounds.begin(), wounds.end(), eff));
        eff += 2;
    }
    
    wounds.push_back(eff);
    std::sort(wounds.begin(), wounds.end(), std::greater<int>()); // Keep largest first
}

void Entity::applyBleeding(int severity) {
    blood -= severity;
    if (blood < 0) blood = 0;
}

bool Entity::isDead() const {
    // Si on a une blessure de Stade 5 (ou plus), c'est la mort
    return !wounds.empty() && wounds.front() >= 5;
}
