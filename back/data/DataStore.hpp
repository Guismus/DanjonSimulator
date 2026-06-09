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

class DataStore {
public:
    static DataStore& getInstance();

    bool loadSystemData(std::string_view filepath);
    bool loadEnergySystem(std::string_view filepath);
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
    std::map<int, EnergyThresholds> rankThresholds;

    float dmgMultMainsNu = 0.95f;
    float dmgMultLegere = 1.0f;
    float dmgMultMoyenne = 1.05f;
    float dmgMultLourde = 1.1f;
};
