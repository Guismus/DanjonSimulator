#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include "../core/entity.hpp"
#include <json.hpp>

using json = nlohmann::json;

class DataStore {
public:
    static DataStore& getInstance();

    bool loadSystemData(const std::string& filepath);
    bool loadEnergySystem(const std::string& filepath);
    bool loadArmors(const std::string& directoryPath);
    bool loadWeapons(const std::string& directoryPath);
    bool loadEntities(const std::string& directoryPath);
    
    std::optional<Entity> getEntityTemplate(const std::string& name) const;
    std::vector<std::string> getAvailableEntityNames() const;
    std::vector<std::string> getAvailableWeaponNames() const;
    std::vector<std::string> getAvailableArmorNames() const;
    std::optional<Weapon> getWeaponTemplate(const std::string& name) const;
    std::optional<Armor> getArmorTemplate(const std::string& name) const;
    const EnergyThresholds* getEnergyThresholds(int rank) const;
    float getDmgMultMainsNu() const { return dmgMultMainsNu; }
    float getDmgMultLegere() const { return dmgMultLegere; }
    float getDmgMultMoyenne() const { return dmgMultMoyenne; }
    float getDmgMultLourde() const { return dmgMultLourde; }

private:
    DataStore() = default;
    ~DataStore() = default;

    DataStore(const DataStore&) = delete;
    DataStore& operator=(const DataStore&) = delete;

    std::map<std::string, Entity> entityTemplates;
    std::map<std::string, Armor> armorTemplates;
    std::map<std::string, Weapon> weaponTemplates;
    std::map<int, EnergyThresholds> rankThresholds;

    float dmgMultMainsNu = 0.95f;
    float dmgMultLegere = 1.0f;
    float dmgMultMoyenne = 1.05f;
    float dmgMultLourde = 1.1f;
};
