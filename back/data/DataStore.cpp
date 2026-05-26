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

bool DataStore::loadEnergySystem(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;
    try {
        json j;
        file >> j;
        if (j.contains("reserves_energie_physique")) {
            for (const auto& item : j["reserves_energie_physique"]) {
                int rank = item["tier_physique"];
                EnergyThresholds t;
                t.maxReserve = item["max"];
                t.essouffle = item["rapport"]["essouffle"];
                t.haletant = item["rapport"]["haletant"];
                t.epuise = item["rapport"]["epuise"];
                t.aBout = item["rapport"]["a_bout"];
                t.inconscient = item["rapport"]["inconscient"];
                rankThresholds[rank] = t;
            }
        }
        return true;
    } catch (std::exception& e) {
        std::cerr << "Failed to parse energy system: " << e.what() << std::endl;
        return false;
    }
}

const EnergyThresholds* DataStore::getEnergyThresholds(int rank) const {
    auto it = rankThresholds.find(rank);
    if (it != rankThresholds.end()) return &it->second;
    return nullptr;
}

bool DataStore::loadArmors(const std::string& directoryPath) {
    if (!fs::exists(directoryPath)) {
        std::cerr << "Armor directory does not exist: " << directoryPath << std::endl;
        return false;
    }
    bool success = true;
    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (entry.path().extension() == ".json") {
            std::ifstream file(entry.path());
            if (!file.is_open()) {
                std::cerr << "Failed to open armor file: " << entry.path() << std::endl;
                success = false;
                continue;
            }
            try {
                json j;
                file >> j;
                
                Armor armor;
                armor.name = j.value("nom", j.value("name", "Unknown"));
                
                std::string matStr = j.value("type_de_materiau", j.value("type de matériau", j.value("materiau", j.value("material", "Fibre"))));
                if (matStr == "Peau" || matStr == "peau" || matStr == "Peau (cuir, écaille)" || matStr == "cuir" || matStr == "écaille") {
                    armor.material = ArmorMaterial::Peau;
                } else if (matStr == "Mineral" || matStr == "mineral" || matStr == "Minéral" || matStr == "minéral" || matStr == "metal" || matStr == "métal" || matStr == "mailles") {
                    armor.material = ArmorMaterial::Mineral;
                } else {
                    armor.material = ArmorMaterial::Fibre;
                }
                
                armor.durability = j.value("durabilite", j.value("durability", 0));
                armor.maxDurability = armor.durability;
                
                int res = 0;
                int resMagique = 0;
                if (j.contains("stats")) {
                    res = j["stats"].value("res", 0);
                    resMagique = j["stats"].value("res_magique", j["stats"].value("resMagique", j["stats"].value("rmag", 0)));
                } else {
                    res = j.value("res", 0);
                    resMagique = j.value("res_magique", j.value("resMagique", j.value("rmag", 0)));
                }
                armor.res = res;
                armor.resMagique = resMagique;
                
                std::string key = entry.path().stem().string();
                armorTemplates[key] = armor;
            } catch (json::parse_error& e) {
                std::cerr << "JSON parse error in armor " << entry.path() << ": " << e.what() << std::endl;
                success = false;
            }
        }
    }
    return success;
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
                
                int rankVal = 1;
                if (j.contains("rank")) {
                    if (j["rank"].is_string()) {
                        std::string rStr = j["rank"].get<std::string>();
                        if (rStr == "None") rankVal = 0;
                        else if (rStr == "F") rankVal = 1;
                        else if (rStr == "E") rankVal = 2;
                        else if (rStr == "D") rankVal = 3;
                        else if (rStr == "C") rankVal = 4;
                        else if (rStr == "B") rankVal = 5;
                        else if (rStr == "A") rankVal = 6;
                        else if (rStr == "S") rankVal = 7;
                    } else if (j["rank"].is_number()) {
                        rankVal = j["rank"].get<int>();
                    }
                }
                entity.rank = rankVal;
                
                if (j.contains("characterClass") && j["characterClass"].is_string()) {
                    entity.characterClass = j["characterClass"].get<std::string>();
                } else if (j.contains("class") && j["class"].is_string()) {
                    entity.characterClass = j["class"].get<std::string>();
                }

                if (j.contains("passives") && j["passives"].is_array()) {
                    for (const auto& passive : j["passives"]) {
                        if (passive.is_string()) {
                            entity.passives.push_back(passive.get<std::string>());
                        }
                    }
                }
                
                entity.force = j.value("force", 0.0f);
                entity.resistance = j.value("resistance", 0.0f);
                entity.vitesse = j.value("vitesse", 0.0f);
                entity.forceMagique = j.value("forceMagique", 0.0f);
                entity.resistanceMagique = j.value("resistanceMagique", 0.0f);
                


                std::string dmgTypeStr = j.value("physicalDamageType", "Neutre");
                if (dmgTypeStr == "Contondant") entity.physicalDamageType = PhysicalDamageType::Contondant;
                else if (dmgTypeStr == "Tranchant") entity.physicalDamageType = PhysicalDamageType::Tranchant;
                else entity.physicalDamageType = PhysicalDamageType::Neutre;

                if (auto t = getEnergyThresholds(entity.rank)) {
                    entity.physicalThresholds = *t;
                    entity.maxPhysicalReserve = t->maxReserve;
                    entity.physicalReserve = t->maxReserve; // Start at max
                }

                std::string armorKey = "";
                for (const char* eqKey : {"equipement", "equipment", "Equipement", "Equipment"}) {
                    if (j.contains(eqKey) && j[eqKey].is_object()) {
                        auto equip = j[eqKey];
                        for (const char* arKey : {"armure", "armor", "Armure", "Armor"}) {
                            if (equip.contains(arKey) && equip[arKey].is_object()) {
                                auto armure = equip[arKey];
                                for (const char* tsKey : {"torse", "chest", "Torse", "Chest"}) {
                                    if (armure.contains(tsKey) && armure[tsKey].is_string()) {
                                        armorKey = armure[tsKey].get<std::string>();
                                        break;
                                    }
                                }
                            }
                            if (!armorKey.empty()) break;
                        }
                    }
                    if (!armorKey.empty()) break;
                }
                
                if (!armorKey.empty()) {
                    auto armorIt = armorTemplates.find(armorKey);
                    if (armorIt != armorTemplates.end()) {
                        entity.armor = armorIt->second;
                    } else {
                        std::cerr << "Armor template not found: " << armorKey << std::endl;
                    }
                }

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
