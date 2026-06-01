#include "DataStore.hpp"
#include "../systems/CombatSystem.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <print>
#include <cstdio>

namespace fs = std::filesystem;

DataStore& DataStore::getInstance() {
    static DataStore instance;
    return instance;
}

bool DataStore::loadSystemData(std::string_view filepath) {
    std::ifstream file{std::filesystem::path(filepath)};
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
        std::println(stderr, "Failed to parse system data: {}", e.what());
        return false;
    }
}

bool DataStore::loadEnergySystem(std::string_view filepath) {
    std::ifstream file{std::filesystem::path(filepath)};
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
        if (j.contains("multiplicateurs_degats")) {
            auto mults = j["multiplicateurs_degats"];
            dmgMultMainsNu = mults.value("mains_nu", 0.95f);
            dmgMultLegere = mults.value("legere", 1.0f);
            dmgMultMoyenne = mults.value("moyenne", 1.05f);
            dmgMultLourde = mults.value("lourde", 1.1f);
        }
        return true;
    } catch (std::exception& e) {
        std::println(stderr, "Failed to parse energy system: {}", e.what());
        return false;
    }
}

const EnergyThresholds* DataStore::getEnergyThresholds(int rank) const {
    auto it = rankThresholds.find(rank);
    if (it != rankThresholds.end()) return &it->second;
    return nullptr;
}

bool DataStore::loadArmors(std::string_view directoryPath) {
    fs::path dir{directoryPath};
    if (!fs::exists(dir)) {
        std::println(stderr, "Armor directory does not exist: {}", directoryPath);
        return false;
    }
    bool success = true;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == ".json") {
            std::ifstream file(entry.path());
            if (!file.is_open()) {
                std::println(stderr, "Failed to open armor file: {}", entry.path().string());
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
                std::println(stderr, "JSON parse error in armor {}: {}", entry.path().string(), e.what());
                success = false;
            }
        }
    }
    return success;
}

bool DataStore::loadEntities(std::string_view directoryPath) {
    bool success = true;
    fs::path dir{directoryPath};
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == ".json") {
            std::ifstream file(entry.path());
            if (!file.is_open()) {
                std::println(stderr, "Failed to open {}", entry.path().string());
                success = false;
                continue;
            }

            try {
                json j;
                file >> j;
                
                std::string name = j.value("name", "Unknown");
                Entity entity(name);
                entity.stade = j.value("stade", j.value("level", 1));
                entity.isMonster = j.value("isMonster", j.value("is_monster", j.value("monster", !j.contains("characterClass") && !j.contains("class"))));
                
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
                    entity.setClass(j["characterClass"].get<std::string>());
                } else if (j.contains("class") && j["class"].is_string()) {
                    entity.setClass(j["class"].get<std::string>());
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
                


                std::string dmgTypeStr = j.value("damageType", "Neutre");
                if (dmgTypeStr == "Contondant") entity.damageType = DamageType::Contondant;
                else if (dmgTypeStr == "Tranchant") entity.damageType = DamageType::Tranchant;
                else if (dmgTypeStr == "Feu") entity.damageType = DamageType::Feu;
                else entity.damageType = DamageType::Neutre;

                if (auto t = getEnergyThresholds(entity.rank)) {
                    entity.physicalThresholds = *t;
                    entity.maxPhysicalReserve = t->maxReserve;
                    entity.physicalReserve = t->maxReserve; // Start at max
                }
                // Override from JSON if present
                entity.wearMultiplierOnPeau = j.value("wearMultiplierOnPeau", entity.wearMultiplierOnPeau);
                entity.wearMultiplierOnFibre = j.value("wearMultiplierOnFibre", entity.wearMultiplierOnFibre);
                entity.wearMultiplierOnMineral = j.value("wearMultiplierOnMineral", entity.wearMultiplierOnMineral);
                entity.freeParriesPerTurn = j.value("freeParriesPerTurn", entity.freeParriesPerTurn);
                entity.freeAttacksPerTurn = j.value("freeAttacksPerTurn", entity.freeAttacksPerTurn);
                entity.immuneToPoison = j.value("immuneToPoison", entity.immuneToPoison);
                entity.immuneToCharm = j.value("immuneToCharm", entity.immuneToCharm);
                entity.immuneToStun = j.value("immuneToStun", entity.immuneToStun);
                entity.woundDebuffDelayTurns = j.value("woundDebuffDelayTurns", entity.woundDebuffDelayTurns);
                entity.fireDamageResistanceBonus = j.value("fireDamageResistanceBonus", entity.fireDamageResistanceBonus);

                entity.magicType = j.value("magicType", j.value("magic_type", "Offensive"));
                entity.magicReserve = j.value("magicReserve", j.value("magic_reserve", 0.0f));

                if (j.contains("catalyst") && j["catalyst"].is_object()) {
                    auto catJ = j["catalyst"];
                    Catalyst cat;
                    cat.magicType = catJ.value("magicType", catJ.value("magic_type", "Offensive"));
                    cat.reserve = catJ.value("reserve", catJ.value("magicReserve", catJ.value("magic_reserve", 0)));
                    cat.power = catJ.value("power", catJ.value("force_magique", catJ.value("forceMagique", 0)));
                    entity.catalyst = cat;
                    if (!j.contains("magicReserve") && !j.contains("magic_reserve")) {
                        entity.magicReserve = static_cast<float>(cat.reserve);
                    }
                }

                entityTemplates.emplace(name, entity);
            } catch (json::parse_error& e) {
                std::println(stderr, "JSON parse error in {}: {}", entry.path().string(), e.what());
                success = false;
            }
        }
    }
    return success;
}

std::optional<Entity> DataStore::getEntityTemplate(std::string_view name) const {
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

bool DataStore::loadWeapons(std::string_view directoryPath) {
    fs::path dir{directoryPath};
    if (!fs::exists(dir)) {
        std::println(stderr, "Weapon directory does not exist: {}", directoryPath);
        return false;
    }
    bool success = true;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == ".json") {
            std::ifstream file(entry.path());
            if (!file.is_open()) {
                std::println(stderr, "Failed to open weapon file: {}", entry.path().string());
                success = false;
                continue;
            }
            try {
                json j;
                file >> j;
                
                Weapon weapon;
                weapon.name = j.value("nom", j.value("name", "Unknown"));
                
                std::string typeStr = j.value("type", "légère");
                if (typeStr == "moyenne" || typeStr == "Moyenne" || typeStr == "moyen" || typeStr == "Moyen") {
                    weapon.type = WeaponWeight::Moyen;
                } else if (typeStr == "lourde" || typeStr == "Lourde" || typeStr == "lourd" || typeStr == "Lourd") {
                    weapon.type = WeaponWeight::Lourd;
                } else {
                    weapon.type = WeaponWeight::Leger;
                }
                
                std::string dmgTypeStr = j.value("type_de_degat", j.value("type de degat", j.value("damageType", j.value("damage_type", "Tranchant"))));
                if (dmgTypeStr == "Contondant" || dmgTypeStr == "contondant") {
                    weapon.damageType = DamageType::Contondant;
                } else if (dmgTypeStr == "Tranchant" || dmgTypeStr == "tranchant") {
                    weapon.damageType = DamageType::Tranchant;
                } else if (dmgTypeStr == "Feu" || dmgTypeStr == "feu") {
                    weapon.damageType = DamageType::Feu;
                } else {
                    weapon.damageType = DamageType::Neutre;
                }
                
                weapon.durability = j.value("durabilite", j.value("durability", 100));
                weapon.maxDurability = weapon.durability;
                
                int res = 0;
                int resMagique = 0;
                if (j.contains("stats")) {
                    res = j["stats"].value("res", 0);
                    resMagique = j["stats"].value("res_magique", j["stats"].value("resMagique", 0));
                } else {
                    res = j.value("res", 0);
                    resMagique = j.value("res_magique", j.value("resMagique", 0));
                }
                weapon.res = res;
                weapon.resMagique = resMagique;
                
                std::string key = entry.path().stem().string();
                weaponTemplates[key] = weapon;
            } catch (json::parse_error& e) {
                std::println(stderr, "JSON parse error in weapon {}: {}", entry.path().string(), e.what());
                success = false;
            }
        }
    }
    return success;
}

std::vector<std::string> DataStore::getAvailableWeaponNames() const {
    std::vector<std::string> names;
    for (const auto& pair : weaponTemplates) {
        names.push_back(pair.second.name);
    }
    return names;
}

std::vector<std::string> DataStore::getAvailableArmorNames() const {
    std::vector<std::string> names;
    for (const auto& pair : armorTemplates) {
        names.push_back(pair.second.name);
    }
    return names;
}

std::optional<Weapon> DataStore::getWeaponTemplate(std::string_view name) const {
    for (const auto& pair : weaponTemplates) {
        if (pair.second.name == name) {
            return pair.second;
        }
    }
    return std::nullopt;
}

std::optional<Armor> DataStore::getArmorTemplate(std::string_view name) const {
    for (const auto& pair : armorTemplates) {
        if (pair.second.name == name) {
            return pair.second;
        }
    }
    return std::nullopt;
}
