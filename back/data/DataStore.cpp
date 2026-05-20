#include "DataStore.hpp"
#include "../systems/CombatSystem.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

DataStore& DataStore::getInstance() {
    static DataStore instance;
    return instance;
}

bool DataStore::loadSystemData(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;
    
    try {
        json j;
        file >> j;
        
        std::array<std::array<float, 7>, 32> table;
        for (int i = 0; i < 32 && i < j.size(); ++i) {
            for (int k = 0; k < 7 && k < j[i].size(); ++k) {
                table[i][k] = j[i][k].get<float>();
            }
        }
        CombatSystem::setDiffStatsTable(table);
        return true;
    } catch (std::exception& e) {
        std::cerr << "Failed to parse system data: " << e.what() << std::endl;
        return false;
    }
}

bool DataStore::loadEntities(const std::string& directoryPath) {
    bool success = true;
    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (entry.path().extension() == ".json") {
            std::ifstream file(entry.path());
            if (!file.is_open()) {
                std::cerr << "Failed to open " << entry.path() << std::endl;
                success = false;
                continue;
            }

            try {
                json j;
                file >> j;
                
                std::string name = j.value("name", "Unknown");
                Entity entity(name);
                entity.stade = j.value("stade", j.value("level", 1));
                entity.rank = j.value("rank", 1);
                entity.force = j.value("force", 0.0f);
                entity.resistance = j.value("resistance", 0.0f);
                entity.vitesse = j.value("vitesse", 0.0f);
                entity.forceMagique = j.value("forceMagique", 0.0f);
                entity.resistanceMagique = j.value("resistanceMagique", 0.0f);
                
                std::string weightStr = j.value("weight", "Moyen");
                if (weightStr == "Leger") entity.weight = Weight::Leger;
                else if (weightStr == "Lourd") entity.weight = Weight::Lourd;
                else if (weightStr == "Surpoids") entity.weight = Weight::Surpoids;
                else if (weightStr == "Effondrement") entity.weight = Weight::Effondrement;
                else entity.weight = Weight::Moyen;

                entityTemplates.emplace(name, entity);
            } catch (json::parse_error& e) {
                std::cerr << "JSON parse error in " << entry.path() << ": " << e.what() << std::endl;
                success = false;
            }
        }
    }
    return success;
}

std::optional<Entity> DataStore::getEntityTemplate(const std::string& name) const {
    auto it = entityTemplates.find(name);
    if (it != entityTemplates.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<std::string> DataStore::getAvailableEntityNames() const {
    std::vector<std::string> names;
    for (const auto& pair : entityTemplates) {
        names.push_back(pair.first);
    }
    return names;
}
