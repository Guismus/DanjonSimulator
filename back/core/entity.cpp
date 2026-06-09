#include "entity.hpp"
#include <algorithm>
#include <cmath>

Entity::Entity(const std::string& name)
    : name(name), stade(1), rank(1), isMonster(false),
      force(0.0f), resistance(0.0f), vitesse(0.0f), forceMagique(0.0f), resistanceMagique(0.0f),
      blood(32.0f), physicalReserve(0.0f), maxPhysicalReserve(0.0f), magicReserve(0.0f),
      activeParries(0), activeDodges(0),
      damageType(DamageType::Neutre) {
}

const std::string& Entity::getName() const {
    return name;
}

void Entity::setClass(const std::string& className) {
    characterClass = className;
    std::string klass = getNormalizedClass();
    if (klass == "ARACHNEE") {
        wearMultiplierOnPeau = 2.0f;
        wearMultiplierOnFibre = 2.0f;
        immuneToPoison = true;
        immuneToCharm = true;
    } else if (klass == "AEGIS") {
        freeParriesPerTurn = 1;
        immuneToStun = true;
        woundDebuffDelayTurns = 2;
    } else if (klass == "FANTOME") {
        freeAttacksPerTurn = 1;
    } else if (klass == "FORGEMAITRE") {
        fireDamageResistanceBonus = 0.10f;
    } else if (klass == "GRIMM") {
        immuneToPoison = true;
        immuneToCharm = true;
    }
}

void Entity::applyWound(int eff, DamageType type) {
    // Les blessures négatives sont également accumulées !
    // Combiner les blessures identiques : 2 Stade X = Stade X+1
    auto it = std::find_if(wounds.begin(), wounds.end(), [eff](const Wound& w) {
        return w.effectiveness == eff;
    });
    
    while (it != wounds.end()) {
        if (it->damageType == DamageType::Tranchant) {
            type = DamageType::Tranchant;
        }
        wounds.erase(it);
        eff += 1;
        it = std::find_if(wounds.begin(), wounds.end(), [eff](const Wound& w) {
            return w.effectiveness == eff;
        });
    }
    
    wounds.push_back(Wound(eff, type, currentTurn));
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

DamageType Entity::getActiveDamageType() const {
    if (weapon.has_value() && weapon->durability > 0) {
        return weapon->damageType;
    }
    return damageType;
}

int Entity::getBleedingRate() const {
    int maxRate = 0;
    for (const auto& wound : wounds) {
        if (wound.damageType == DamageType::Tranchant) {
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
    int maxW = getActiveMaxWoundEffectiveness();
    if (maxW >= 4) {
        val *= 0.70f;
    } else if (maxW == 3) {
        val *= 0.85f;
    }
    return std::ceil(val * 4.0f) / 4.0f;
}

float Entity::getEffectiveResistance() const {
    return std::ceil(resistance * 4.0f) / 4.0f;
}

float Entity::getEffectiveVitesse() const {
    float val = vitesse;
    int maxW = getActiveMaxWoundEffectiveness();
    if (maxW >= 4) {
        val *= 0.70f;
    } else if (maxW == 3) {
        val *= 0.85f;
    }
    return std::ceil(val * 4.0f) / 4.0f;
}

float Entity::getEffectiveForceMagique() const {
    float val = forceMagique;
    int maxW = getActiveMaxWoundEffectiveness();
    if (maxW >= 4) {
        val *= 0.70f;
    } else if (maxW == 3) {
        val *= 0.85f;
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

std::string Entity::getNormalizedClass() const {
    if (!characterClass.has_value()) return "";
    std::string s = characterClass.value();
    std::string res;
    for (char c : s) {
        if (c != ' ' && c != '-' && c != '_') {
            res += std::toupper(static_cast<unsigned char>(c));
        }
    }
    if (res.find("ARACHN") != std::string::npos) return "ARACHNEE";
    if (res.find("FANT") != std::string::npos) return "FANTOME";
    if (res.find("FORGE") != std::string::npos) return "FORGEMAITRE";
    if (res.find("GRIMM") != std::string::npos) return "GRIMM";
    if (res.find("AEGIS") != std::string::npos) return "AEGIS";
    return res;
}

int Entity::getActiveMaxWoundEffectiveness() const {
    int maxW = -999;
    
    for (const auto& w : wounds) {
        if (woundDebuffDelayTurns > 0) {
            if (currentTurn - w.turnApplied < woundDebuffDelayTurns) {
                continue;
            }
        }
        if (w.effectiveness > maxW) {
            maxW = w.effectiveness;
        }
    }
    return maxW;
}

bool Entity::isImmuneToPoison() const {
    return immuneToPoison;
}

bool Entity::isImmuneToCharm() const {
    return immuneToCharm;
}

bool Entity::isImmuneToStun() const {
    return immuneToStun;
}

float Entity::getFireDamageResistanceBonus() const {
    return fireDamageResistanceBonus;
}

float Entity::getResistanceTo(DamageType type) const {
    float resist = 0.0f;
    auto it = damageResistances.find(type);
    if (it != damageResistances.end()) {
        resist += it->second;
    }
    if (type == DamageType::Feu) {
        resist += getFireDamageResistanceBonus();
    }
    return resist;
}

float Entity::getWearMultiplierOn(ArmorMaterial material) const {
    switch (material) {
        case ArmorMaterial::Fibre:   return wearMultiplierOnFibre;
        case ArmorMaterial::Peau:    return wearMultiplierOnPeau;
        case ArmorMaterial::Mineral: return wearMultiplierOnMineral;
    }
    return 1.0f;
}

void Entity::healWounds(int maxEffectiveness) {
    std::erase_if(wounds, [maxEffectiveness](const Wound& w) {
        return w.effectiveness <= maxEffectiveness;
    });
}

void Entity::healExtreme() {
    wounds.clear();
    blood = 32.0f;
}

std::string getStageName(int eff) {
    if (eff == -99) return "Bloqué par l'armure";
    if (eff == 0) return "Neutre";
    std::string name;
    switch (std::abs(eff)) {
        case 1: name = "Faveur"; break;
        case 2: name = "Avantage"; break;
        case 3: name = "Efficace"; break;
        case 4: name = "Surpuissance"; break;
        case 5: name = "Domination"; break;
        case 6: name = "Ecrasement"; break;
        case 7: name = "Tyrannie"; break;
        default: name = "Inconnu"; break;
    }
    if (eff < 0) return "Sous-" + name;
    return name;
}

std::string getDamageTypeName(DamageType type) {
    switch (type) {
        case DamageType::Neutre: return "Neutre (Pugilat)";
        case DamageType::Contondant: return "Contondant";
        case DamageType::Tranchant: return "Tranchant";
        case DamageType::Feu: return "Feu";
        case DamageType::Corrosion: return "Corrosion";
    }
    return "Inconnu";
}

void Entity::consumePhysicalReserve(float amount) {
    physicalReserve -= amount;
    if (physicalReserve < 0.0f) {
        physicalReserve = 0.0f;
    }
}

void Entity::addActiveParry() {
    activeParries++;
}

void Entity::addActiveDodge() {
    activeDodges++;
}

void Entity::consumeActiveParry() {
    if (activeParries > 0) activeParries--;
}

void Entity::consumeActiveDodge() {
    if (activeDodges > 0) activeDodges--;
}

bool Entity::consumeMagicReserve(float amount) {
    if (magicReserve < amount) return false;
    magicReserve -= amount;
    return true;
}

void Entity::applyStatBoost(float forceBoost, float speedBoost) {
    force += forceBoost;
    vitesse += speedBoost;
}

void Entity::reduceWeaponDurability(int amount) {
    if (weapon.has_value()) {
        weapon->durability = std::max(0, weapon->durability - amount);
    }
}

void Entity::reduceArmorDurability(int amount) {
    if (armor.has_value()) {
        armor->durability = std::max(0, armor->durability - amount);
    }
}

void Entity::resetTemporaryCombatStates() {
    activeParries = 0;
    activeDodges = 0;
}


