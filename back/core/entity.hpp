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

enum class ArmorType {
    CuirEcaille,
    MetalChitineCristal
};

struct Weapon {
    std::string name;
    WeaponType type;
    int durability;
    int baseDamageMod;
};

struct Armor {
    std::string name;
    ArmorType type;
    int durability;
    int resistanceMod;
};

struct Catalyst {
    std::string magicType; // Offensive, Boost, Soins
    int reserve;
    int power;
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
    float magicReserve;

    Weight weight;
    std::optional<std::string> characterClass;

    std::vector<std::string> passives;

    std::optional<Weapon> weapon;
    std::optional<Armor> armor;
    std::optional<Catalyst> catalyst;

    // Methods
    const std::string& getName() const;
    void applyWound(int effectiveness);
    void applyBleeding(int severity);
    bool isDead() const;
    
    std::vector<int> wounds; // Tracks current wound stages

private:
    std::string name;
};