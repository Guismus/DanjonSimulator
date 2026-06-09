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
    Feu,
    Corrosion
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
    std::string magicSpell = "";
    bool useCatalyst = false;
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

struct ActiveEffect {
    std::string type;             // "invulnerability", "heal", "boost", "attack_buff"
    int duration = 0;             // remaining turns
    float forceBoost = 0.0f;
    float speedBoost = 0.0f;
    float rMagBoost = 0.0f;
    std::string power = "";       // for heal: "extreme", "moyen", etc.
    int casterMagicPower = 0;     // for scaling heals
    float burnMultiplier = 0.0f;  // for attack burn modifiers
    std::string healPower = "";   // for healing triggered on attack
    std::string spellName = "";   // source spell name for log output
};

struct WeaponDamageMultipliers {
    float mainsNu = 0.95f;
    float legere = 1.0f;
    float moyenne = 1.05f;
    float lourde = 1.1f;
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
    std::vector<Catalyst> catalysts;
    std::string magicType = "";

    DamageType damageType = DamageType::Neutre;
    std::map<DamageType, float> damageResistances;

    int activeParries = 0;
    int activeDodges = 0;
    bool balDesLuciolesActive = false;

    float wearMultiplierOnFibre = 1.0f;
    float wearMultiplierOnPeau = 1.0f;
    float wearMultiplierOnMineral = 1.0f;
    int freeParriesPerTurn = 0;
    int freeAttacksPerTurn = 0;
    bool immuneToPoison = false;
    bool immuneToCharm = false;
    bool immuneToStun = false;
    int woundDebuffDelayTurns = 0;
    float fireDamageResistanceBonus = 0.0f;

    // Methods
    const std::string& getName() const;
    void setClass(const std::string& className);
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
    float getWearMultiplierOn(ArmorMaterial material) const;
    void healWounds(int maxEffectiveness);
    void healExtreme();

    void consumePhysicalReserve(float amount);
    void addActiveParry();
    void addActiveDodge();
    void consumeActiveParry();
    void consumeActiveDodge();
    bool consumeMagicReserve(float amount);
    void applyStatBoost(float forceBoost, float speedBoost, float rMagBoost = 0.0f);
    void reduceWeaponDurability(int amount);
    void reduceArmorDurability(int amount);
    void resetTemporaryCombatStates();
    void updateActiveEffects(std::vector<std::string>& logs);
    void decrementActiveEffects(std::vector<std::string>& logs);

    int currentTurn = 1;
    int invulnerableTurnsLeft = 0;
    std::string getNormalizedClass() const;
    int getActiveMaxWoundEffectiveness() const;
    bool isImmuneToPoison() const;
    bool isImmuneToCharm() const;
    bool isImmuneToStun() const;
    float getFireDamageResistanceBonus() const;
    float getResistanceTo(DamageType type) const;

    std::vector<Wound> wounds; // Tracks current wound stages
    std::vector<ActiveEffect> activeEffects;

private:
    std::string name;
};

std::string getStageName(int eff);
std::string getDamageTypeName(DamageType type);