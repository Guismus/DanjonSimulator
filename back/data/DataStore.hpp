#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <optional>
#include <functional>
#include "../core/entity.hpp"
#include <json.hpp>

using json = nlohmann::json;

struct CatalystTemplate {
    int reserve;
    int power;
};

struct Spell {
    std::string name;
    float cost = 10.0f;
    nlohmann::json effects = nlohmann::json::array();
};

struct Race {
    std::string name;
    std::vector<std::string> passives;
    std::map<std::string, float> dragonStats;
};

class DataStore {
public:
    static DataStore& getInstance();

    bool loadSystemData(std::string_view filepath);
    bool loadEnergySystem(std::string_view filepath);
    bool loadCatalysts(std::string_view filepath);
    bool loadSpells(std::string_view directoryPath);
    bool loadRaces(std::string_view directoryPath);
    bool loadArmors(std::string_view directoryPath);
    bool loadWeapons(std::string_view directoryPath);
    bool loadEntities(std::string_view directoryPath);
    
    std::optional<Entity> getEntityTemplate(std::string_view name) const;
    std::vector<std::string> getAvailableEntityNames() const;
    std::vector<std::string> getAvailableWeaponNames() const;
    std::vector<std::string> getAvailableArmorNames() const;
    std::optional<Weapon> getWeaponTemplate(std::string_view name) const;
    std::optional<Armor> getArmorTemplate(std::string_view name) const;
    const EnergyThresholds* getEnergyThresholds(int rank) const;
    std::optional<CatalystTemplate> getCatalystTemplate(const std::string& rank) const;
    std::optional<Spell> getSpell(std::string_view name) const;
    std::optional<Race> getRace(std::string_view name) const;
    Entity createFighter(std::string_view name, std::string_view weaponName, std::string_view armorName, float defaultForce = 10.0f) const;
    WeaponDamageMultipliers getWeaponDamageMultipliers() const;
    float getDmgMultMainsNu() const { return dmgMultMainsNu; }
    float getDmgMultLegere() const { return dmgMultLegere; }
    float getDmgMultMoyenne() const { return dmgMultMoyenne; }
    float getDmgMultLourde() const { return dmgMultLourde; }

private:
    DataStore() = default;
    ~DataStore() = default;

    DataStore(const DataStore&) = delete;
    DataStore& operator=(const DataStore&) = delete;

    std::map<std::string, Entity, std::less<>> entityTemplates;
    std::map<std::string, Armor, std::less<>> armorTemplates;
    std::map<std::string, Weapon, std::less<>> weaponTemplates;
    std::map<std::string, CatalystTemplate, std::less<>> catalystTemplates;
    std::map<std::string, Spell, std::less<>> magicSpells;
    std::map<std::string, Race, std::less<>> races;
    std::map<int, EnergyThresholds> rankThresholds;


    float dmgMultMainsNu = 0.95f;
    float dmgMultLegere = 1.0f;
    float dmgMultMoyenne = 1.05f;
    float dmgMultLourde = 1.1f;
};
