#pragma once

#include <string>
#include <map>
#include <vector>
#include <optional>

enum class Weight {
    Leger,
    Moyen,
    Lourd,
    Surpoids,
    Effondrement
};

enum class WeaponType {
    Tranchant,
    Contondant
};

enum class PhysicalDamageType {
    Neutre,
    Contondant,
    Tranchant
};

enum class ArmorMaterial {
    Fibre,
    Peau,
    Mineral
};

struct Weapon {
    std::string name;
    WeaponType type;
    int durability;
    int baseDamageMod;
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
    PhysicalDamageType damageType;
};

class Entity {
public:
    Entity(const std::string& name);

    // Stats
    int stade;
    int rank;

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

    Weight weight;
    std::optional<std::string> characterClass;

    std::vector<std::string> passives;

    std::optional<Weapon> weapon;
    std::optional<Armor> armor;
    std::optional<Catalyst> catalyst;

    PhysicalDamageType physicalDamageType = PhysicalDamageType::Neutre;

    // Methods
    const std::string& getName() const;
    void applyWound(int effectiveness, PhysicalDamageType type);
    void applyBleeding(int severity);
    bool isDead() const;
    std::string getPhysicalState() const;
    PhysicalDamageType getActiveDamageType() const;
    int getBleedingRate() const;
    std::string getBleedingState() const;
    
    std::vector<Wound> wounds; // Tracks current wound stages

private:
    std::string name;
};