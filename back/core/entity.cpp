#include "entity.hpp"
#include <algorithm>
#include <cmath>

Entity::Entity(const std::string& name)
    : name(name), stade(1), rank(1),
      force(0.0f), resistance(0.0f), vitesse(0.0f), forceMagique(0.0f), resistanceMagique(0.0f),
      blood(32.0f), physicalReserve(0.0f), maxPhysicalReserve(0.0f), magicReserve(0.0f),
      activeParries(0), activeDodges(0),
      physicalDamageType(PhysicalDamageType::Neutre) {
}

const std::string& Entity::getName() const {
    return name;
}

void Entity::applyWound(int eff, PhysicalDamageType type) {
    // Les blessures négatives sont également accumulées !
    // Combiner les blessures identiques : 2 Stade X = Stade X+1
    auto it = std::find_if(wounds.begin(), wounds.end(), [eff](const Wound& w) {
        return w.effectiveness == eff;
    });
    
    while (it != wounds.end()) {
        if (it->damageType == PhysicalDamageType::Tranchant) {
            type = PhysicalDamageType::Tranchant;
        }
        wounds.erase(it);
        eff += 1;
        it = std::find_if(wounds.begin(), wounds.end(), [eff](const Wound& w) {
            return w.effectiveness == eff;
        });
    }
    
    wounds.push_back({eff, type});
    std::sort(wounds.begin(), wounds.end(), [](const Wound& a, const Wound& b) {
        return a.effectiveness > b.effectiveness;
    });
}

void Entity::applyBleeding(int severity) {
    blood -= severity;
    if (blood < 0) blood = 0;
}

bool Entity::isDead() const {
    // Si on a une blessure de Stade 5 (ou plus), ou si la réserve de sang est à 0, c'est la mort
    if (blood <= 0) return true;
    if (!wounds.empty() && wounds.front().effectiveness >= 5) return true;

    // Règle additionnelle : Stade 4+ (Surpuissance) combiné avec Stade 0+ (Neutre ou plus) provoque la mort
    if (wounds.size() >= 2 && wounds[0].effectiveness >= 4 && wounds[1].effectiveness >= 0) {
        return true;
    }
    return false;
}

std::string Entity::getPhysicalState() const {
    if (physicalThresholds.essouffle == -1) {
        // Missing thresholds (e.g. Rank C)
        return "Inconnu";
    }
    if (physicalReserve <= physicalThresholds.inconscient) return "Inconscient";
    if (physicalReserve <= physicalThresholds.aBout) return "A bout";
    if (physicalReserve <= physicalThresholds.epuise) return "Epuisé";
    if (physicalReserve <= physicalThresholds.haletant) return "Haletant";
    if (physicalReserve <= physicalThresholds.essouffle) return "Essoufflé";
    return "En forme";
}

PhysicalDamageType Entity::getActiveDamageType() const {
    if (weapon.has_value()) {
        if (weapon->type == WeaponType::Tranchant) return PhysicalDamageType::Tranchant;
        if (weapon->type == WeaponType::Contondant) return PhysicalDamageType::Contondant;
    }
    return physicalDamageType;
}

int Entity::getBleedingRate() const {
    int maxRate = 0;
    for (const auto& wound : wounds) {
        if (wound.damageType == PhysicalDamageType::Tranchant) {
            int rate = 0;
            if (wound.effectiveness <= 0) {
                rate = 1;
            } else if (wound.effectiveness == 1 || wound.effectiveness == 2) {
                rate = 2;
            } else if (wound.effectiveness >= 3) {
                rate = 3;
            }
            if (rate > maxRate) {
                maxRate = rate;
            }
        }
    }
    return maxRate;
}

std::string Entity::getBleedingState() const {
    int rate = getBleedingRate();
    switch (rate) {
        case 1: return "Bénin";
        case 2: return "Violent";
        case 3: return "Grave";
        default: return "Aucun";
    }
}

float Entity::getEffectiveForce() const {
    float val = force;
    if (!wounds.empty()) {
        int maxW = wounds.front().effectiveness;
        if (maxW >= 4) {
            val *= 0.70f;
        } else if (maxW == 3) {
            val *= 0.85f;
        }
    }
    return std::ceil(val * 4.0f) / 4.0f;
}

float Entity::getEffectiveResistance() const {
    return std::ceil(resistance * 4.0f) / 4.0f;
}

float Entity::getEffectiveVitesse() const {
    float val = vitesse;
    if (!wounds.empty()) {
        int maxW = wounds.front().effectiveness;
        if (maxW >= 4) {
            val *= 0.70f;
        } else if (maxW == 3) {
            val *= 0.85f;
        }
    }
    return std::ceil(val * 4.0f) / 4.0f;
}

float Entity::getEffectiveForceMagique() const {
    float val = forceMagique;
    if (!wounds.empty()) {
        int maxW = wounds.front().effectiveness;
        if (maxW >= 4) {
            val *= 0.70f;
        } else if (maxW == 3) {
            val *= 0.85f;
        }
    }
    return std::ceil(val * 4.0f) / 4.0f;
}

float Entity::getEffectiveResistanceMagique() const {
    return std::ceil(resistanceMagique * 4.0f) / 4.0f;
}

bool Entity::hasPassive(const std::string& passiveName) const {
    for (const auto& passive : passives) {
        if (passive == passiveName) {
            return true;
        }
    }
    return false;
}
