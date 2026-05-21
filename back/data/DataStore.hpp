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
    bool loadEntities(const std::string& directoryPath);
    
    std::optional<Entity> getEntityTemplate(const std::string& name) const;
    std::vector<std::string> getAvailableEntityNames() const;
    const EnergyThresholds* getEnergyThresholds(int rank) const;

private:
    DataStore() = default;
    ~DataStore() = default;

    DataStore(const DataStore&) = delete;
    DataStore& operator=(const DataStore&) = delete;

    std::map<std::string, Entity> entityTemplates;
    std::map<std::string, Armor> armorTemplates;
    std::map<int, EnergyThresholds> rankThresholds;
};
