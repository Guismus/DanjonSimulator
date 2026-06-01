#pragma once

#include <string>
#include <map>
#include <vector>
#include <optional>

enum class DamageNature {
    Physique,
    Magique
};

enum class DamageType {
    Neutre,
    Contondant,
    Tranchant,
    Feu
};

enum class WeaponWeight {
    Leger,
    Moyen,
    Lourd
};

enum class ArmorMaterial {
    Fibre,
    Peau,
    Mineral
};

enum class ActionType {
    Attack,
    Parry,
    Dodge,
    Magic
};

enum class ControlMode {
    Manual,
    Script,
    TCP
};

struct QueuedAction {
    ActionType type;
    float overclockMultiplier = 1.0f;
};

struct Weapon {
    std::string name;
    WeaponWeight type;
    DamageType damageType;
    int durability;
    int maxDurability;
    int res;
    int resMagique;
};

struct Armor {
    std::string name;
    ArmorMaterial material;
    int durability;
    int maxDurability;
    int res;
    int resMagique;
};

struct Catalyst {
    std::string magicType; // Offensive, Boost, Soins
    int reserve;
    int power;
};

struct EnergyThresholds {
    float maxReserve = 0;
    float essouffle = -1;
    float haletant = -1;
    float epuise = -1;
    float aBout = -1;
    float inconscient = 0;
};

struct Wound {
    int effectiveness;
    DamageType damageType;
    int turnApplied;

    Wound(int eff, DamageType type, int turn = 1)
        : effectiveness(eff), damageType(type), turnApplied(turn) {}
};

class Entity {
public:
    Entity(const std::string& name);

    // Stats
    int stade;
    int rank;
    bool isMonster = false;

    float force;
    float resistance;
    float vitesse;
    float forceMagique;
    float resistanceMagique;

    // Derived & Resources
    float blood; // max 32
    float physicalReserve;
    float maxPhysicalReserve;
    EnergyThresholds physicalThresholds;
    float magicReserve;

    std::optional<std::string> characterClass;

    std::vector<std::string> passives;

    std::optional<Weapon> weapon;
    std::optional<Armor> armor;
    std::optional<Catalyst> catalyst;

    DamageType damageType = DamageType::Neutre;
    std::map<DamageType, float> damageResistances;

    int activeParries = 0;
    int activeDodges = 0;

    // Methods
    const std::string& getName() const;
    void applyWound(int effectiveness, DamageType type);
    void applyBleeding(int severity);
    bool isDead() const;
    std::string getPhysicalState() const;
    DamageType getActiveDamageType() const;
    int getBleedingRate() const;
    std::string getBleedingState() const;
    
    float getEffectiveForce() const;
    float getEffectiveResistance() const;
    float getEffectiveVitesse() const;
    float getEffectiveForceMagique() const;
    float getEffectiveResistanceMagique() const;
    bool hasPassive(const std::string& passiveName) const;

    int currentTurn = 1;
    std::string getNormalizedClass() const;
    int getActiveMaxWoundEffectiveness() const;
    bool isImmuneToPoison() const;
    bool isImmuneToCharm() const;
    bool isImmuneToStun() const;
    float getFireDamageResistanceBonus() const;
    float getResistanceTo(DamageType type) const;

    std::vector<Wound> wounds; // Tracks current wound stages

private:
    std::string name;
};