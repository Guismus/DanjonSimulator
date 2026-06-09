#include "simulator.hpp"
#include "../systems/CombatSystem.hpp"
#include "../data/DataStore.hpp"
#include <algorithm>
#include <cmath>
#include <format>


Simulator::Simulator() {
}

Simulator::~Simulator() {
}

void Simulator::startCombat(const Entity& f1, const Entity& f2, ControlMode m1, ControlMode m2) {
    fighter1 = f1;
    fighter2 = f2;
    p1Mode = m1;
    p2Mode = m2;
    p1Actions.clear();
    p2Actions.clear();
    
    float v1 = fighter1->getEffectiveVitesse();
    float v2 = fighter2->getEffectiveVitesse();
    int refLevel = std::min(fighter1->stade, fighter2->stade);
    int speedDiff = CombatSystem::calculateStatDifference(v1, v2, refLevel);
    p1FreeActions = 2 + (speedDiff > 0 ? speedDiff : 0);
    p2FreeActions = 2 + (speedDiff < 0 ? -speedDiff : 0);

    currentTurn = 1;
    p1Finished = false;
    p2Finished = false;
}

void Simulator::addActionP1(ActionType type) {
    if (fighter1.has_value()) {
        float multiplier = getNextMultiplier(fighter1.value(), p1Actions, type, p1FreeActions);
        p1Actions.push_back({type, multiplier});
    }
}

void Simulator::addActionP2(ActionType type) {
    if (fighter2.has_value()) {
        float multiplier = getNextMultiplier(fighter2.value(), p2Actions, type, p2FreeActions);
        p2Actions.push_back({type, multiplier});
    }
}

void Simulator::popActionP1() {
    p1Finished = false;
    if (!p1Actions.empty()) p1Actions.pop_back();
}

void Simulator::popActionP2() {
    p2Finished = false;
    if (!p2Actions.empty()) p2Actions.pop_back();
}

std::vector<float> Simulator::computeOverclockMultipliers(const Entity& entity, const std::vector<ActionType>& actions, int baseFreeActions) const {
    std::vector<float> multipliers(actions.size(), 1.0f);
    float overclockMultipliers[] = {2.0f, 2.3f, 2.6f, 3.4f, 5.0f, 7.0f};

    int freeParries = entity.freeParriesPerTurn;
    int freeAttacks = entity.freeAttacksPerTurn;

    int freeParriesUsed = 0;
    int freeAttacksUsed = 0;
    
    int standardActionsCount = 0;

    for (size_t i = 0; i < actions.size(); ++i) {
        ActionType type = actions[i];
        
        bool isFreeDueToClass = false;
        if (type == ActionType::Parry && freeParriesUsed < freeParries) {
            isFreeDueToClass = true;
            freeParriesUsed++;
        } else if (type == ActionType::Attack && freeAttacksUsed < freeAttacks) {
            isFreeDueToClass = true;
            freeAttacksUsed++;
        }

        if (isFreeDueToClass) {
            multipliers[i] = 1.0f;
        } else {
            if (standardActionsCount < baseFreeActions) {
                multipliers[i] = 1.0f;
            } else {
                int idx = standardActionsCount - baseFreeActions;
                if (idx > 5) idx = 5;
                multipliers[i] = overclockMultipliers[idx];
            }
            standardActionsCount++;
        }
    }
    return multipliers;
}

float Simulator::getNextMultiplier(const Entity& entity, const std::vector<QueuedAction>& currentActions, ActionType nextType, int baseFreeActions) const {
    std::vector<ActionType> types;
    for (const auto& a : currentActions) {
        types.push_back(a.type);
    }
    types.push_back(nextType);
    
    std::vector<float> mults = computeOverclockMultipliers(entity, types, baseFreeActions);
    if (mults.empty()) return 1.0f;
    return mults.back();
}

nlohmann::json Simulator::serializeEntityJson(const Entity& entity, const std::vector<QueuedAction>& queuedActions, int freeActions) const {
    nlohmann::json obj;
    obj["name"] = entity.getName();
    obj["class"] = entity.characterClass.value_or("");
    obj["blood"] = entity.blood;
    obj["physical_reserve"] = entity.physicalReserve;
    obj["max_physical_reserve"] = entity.maxPhysicalReserve;
    obj["magic_reserve"] = entity.magicReserve;
    obj["stade"] = entity.stade;
    obj["rank"] = entity.rank;
    obj["vitesse"] = entity.getEffectiveVitesse();
    obj["force"] = entity.getEffectiveForce();
    obj["resistance"] = entity.getEffectiveResistance();
    obj["force_magique"] = entity.getEffectiveForceMagique();
    obj["resistance_magique"] = entity.getEffectiveResistanceMagique();
    
    std::vector<std::string> actionsArr;
    for (const auto& act : queuedActions) {
        if (act.type == ActionType::Attack) actionsArr.push_back("Attaquer");
        else if (act.type == ActionType::Parry) actionsArr.push_back("Parer");
        else if (act.type == ActionType::Dodge) actionsArr.push_back("Esquiver");
        else if (act.type == ActionType::Magic) actionsArr.push_back("Magie");
    }
    obj["queued_actions"] = actionsArr;
    obj["free_actions"] = freeActions;
    return obj;
}

std::string Simulator::serializeStateJson(const Entity& active, const std::vector<QueuedAction>& activeActions,
                                          const Entity& opponent, const std::vector<QueuedAction>& opponentActions) const {
    nlohmann::json state;
    
    int activeFree = (&active == &(*fighter1)) ? p1FreeActions : p2FreeActions;
    int oppFree = (&opponent == &(*fighter1)) ? p1FreeActions : p2FreeActions;

    state["active_character"] = serializeEntityJson(active, activeActions, activeFree);
    state["opponent_character"] = serializeEntityJson(opponent, opponentActions, oppFree);
    return state.dump();
}

void Simulator::fetchAutomatedActions(Entity& entity, std::vector<QueuedAction>& actions, int freeActions, ControlMode mode, int playerNum) {
    if (mode == ControlMode::Manual) return;
    
    actions.clear();
    
    Entity& opponent = (&entity == &(*fighter1)) ? (*fighter2) : (*fighter1);
    const std::vector<QueuedAction>& opponentActions = (&entity == &(*fighter1)) ? p2Actions : p1Actions;
    
    int maxTries = 10;
    
    for (int i = 0; i < maxTries; ++i) {
        if (entity.isDead() || entity.physicalReserve <= 0) break;
        
        std::string state = serializeStateJson(entity, actions, opponent, opponentActions);
        
        if (!externalAgentQuery) break;
        std::string actionStr = externalAgentQuery(state, playerNum);
        
        // trim whitespace
        actionStr.erase(0, actionStr.find_first_not_of(" \t\r\n"));
        actionStr.erase(actionStr.find_last_not_of(" \t\r\n") + 1);
        
        if (actionStr == "Passer" || actionStr == "Finir le tour" || actionStr.empty()) {
            break;
        }
        
        ActionType type;
        if (actionStr == "Attaquer") {
            type = ActionType::Attack;
        } else if (actionStr == "Parer") {
            type = ActionType::Parry;
        } else if (actionStr == "Esquiver") {
            type = ActionType::Dodge;
        } else if (actionStr == "Magie") {
            type = ActionType::Magic;
        } else {
            break;
        }
        
        float multiplier = getNextMultiplier(entity, actions, type, freeActions);
        actions.push_back({type, multiplier});
    }
}

void Simulator::executeSingleAction(Entity* attacker, Entity* defender, const QueuedAction& action, int actionIndex, std::vector<std::string>& logs) {
    if (attacker->isDead() || attacker->physicalReserve <= 0) return;
    
    std::string logMsg = attacker->getName() + " : ";
    
    if (action.type == ActionType::Attack) {
        logMsg += "Attaque (Action " + std::to_string(actionIndex + 1) + ") ";
        if (action.overclockMultiplier > 1.0f) {
            logMsg += std::format("[Surcadençage x{:.1f}] ", action.overclockMultiplier);
        }
        
        std::optional<int> preArmor;
        if (defender->armor.has_value()) preArmor = defender->armor->durability;

        std::optional<int> preWeapon;
        if (attacker->weapon.has_value()) preWeapon = attacker->weapon->durability;

        bool wasParrying = (defender->activeParries > 0);
        int eff = CombatSystem::executeAttack(*attacker, *defender, action.overclockMultiplier, DamageNature::Physique, std::nullopt, DataStore::getInstance().getWeaponDamageMultipliers());
        
        if (eff == -98) {
            logMsg += "-> L'attaque est esquivée !";
        } else if (eff == -97) {
            logMsg += "-> L'attaque est bloquée par la parade (Bouclier en métal) !";
        } else if (eff == -99) {
            logMsg += "-> L'attaque est bloquée par l'armure !";
        } else {
            logMsg += "et inflige un " + getStageName(eff) + " physique (" + getDamageTypeName(attacker->getActiveDamageType()) + ")";
            if (wasParrying) {
                int pct = (defender->getNormalizedClass() == "AEGIS") ? 25 : 10;
                logMsg += " (paré, efficacité de l'attaque réduite de " + std::to_string(pct) + "%)";
            }
        }

        // Log weapon durability loss
        if (attacker->weapon.has_value() && preWeapon.has_value()) {
            int currentDur = attacker->weapon->durability;
            int diff = preWeapon.value() - currentDur;
            if (diff > 0) {
                logMsg += std::format("\n  [Arme] {} subit -{} de durabilité ({}/{})",
                                      attacker->weapon->name, diff, currentDur, attacker->weapon->maxDurability);
                if (currentDur == 0 && preWeapon.value() > 0) {
                    logMsg += std::format("\n  [Arme] {} est rompue !", attacker->weapon->name);
                }
            }
        }

        // Log armor durability loss
        if (defender->armor.has_value() && preArmor.has_value()) {
            int currentDur = defender->armor->durability;
            int diff = preArmor.value() - currentDur;
            if (diff > 0) {
                logMsg += std::format("\n  [Armure] {} subit -{} de durabilité ({}/{})",
                                      defender->armor->name, diff, currentDur, defender->armor->maxDurability);
                if (currentDur == 0 && preArmor.value() > 0) {
                    logMsg += std::format("\n  [Armure] {} est rompue !", defender->armor->name);
                }
            }
        }

        if (defender->isDead()) {
            logMsg += "\n" + defender->getName() + " est K.O !";
        }
    } else if (action.type == ActionType::Parry) {
        logMsg += "Parade (Action " + std::to_string(actionIndex + 1) + ") ";
        if (action.overclockMultiplier > 1.0f) {
            logMsg += std::format("[Surcadençage x{:.1f}] ", action.overclockMultiplier);
        }
        CombatSystem::executeParry(*attacker, action.overclockMultiplier);
        int pct = (attacker->getNormalizedClass() == "AEGIS") ? 25 : 10;
        logMsg += "-> Prépare une parade (dégâts de la prochaine attaque réduits de " + std::to_string(pct) + "%).";
    } else if (action.type == ActionType::Dodge) {
        logMsg += "Esquive (Action " + std::to_string(actionIndex + 1) + ") ";
        if (action.overclockMultiplier > 1.0f) {
            logMsg += std::format("[Surcadençage x{:.1f}] ", action.overclockMultiplier);
        }
        CombatSystem::executeDodge(*attacker, action.overclockMultiplier);
        logMsg += "-> Prépare une esquive (évitera la prochaine attaque).";
    } else if (action.type == ActionType::Magic) {
        logMsg += "Magie (Action " + std::to_string(actionIndex + 1) + ") ";
        if (action.overclockMultiplier > 1.0f) {
            logMsg += std::format("[Surcadençage x{:.1f}] ", action.overclockMultiplier);
        }
        
        // Coût en mana : 10
        if (!attacker->isMonster && !attacker->consumeMagicReserve(10.0f)) {
            logMsg += "-> Échec : réserve magique insuffisante (10.0 requis, restant : " + std::format("{:.1f}", attacker->magicReserve) + ")";
            logs.push_back(logMsg);
            return;
        }

        std::string spell = attacker->magicType;
        int power = static_cast<int>(attacker->getEffectiveForceMagique());
        if (attacker->catalyst.has_value()) {
            spell = attacker->catalyst->magicType;
            power = attacker->catalyst->power;
        }

        if (spell == "Boost" || spell == "Boost Magic") {
            attacker->applyStatBoost(5.0f, 5.0f);
            logMsg += std::format("-> Canalise une magie de Boost (Force et Vitesse +5.0).");
        } else if (spell == "Soins" || spell == "Soins Magic" || spell.find("Soins") != std::string::npos) {
            if (spell.find("TresFaible") != std::string::npos || (spell == "Soins" && power < 5)) {
                attacker->healWounds(-2);
                logMsg += "-> Canalise une magie de Soins (Très Faible : blessures <= -2 guéries).";
            } else if (spell.find("Faible") != std::string::npos || (spell == "Soins" && power < 10)) {
                attacker->healWounds(0);
                logMsg += "-> Canalise une magie de Soins (Faible : blessures <= 0 guéries).";
            } else if (spell.find("Fort") != std::string::npos || (spell == "Soins" && power < 20)) {
                attacker->healWounds(3);
                logMsg += "-> Canalise une magie de Soins (Fort : blessures <= 3 guéries).";
            } else if (spell.find("TresFort") != std::string::npos || (spell == "Soins" && power < 25)) {
                attacker->healWounds(4);
                logMsg += "-> Canalise une magie de Soins (Très Fort : blessures <= 4 guéries).";
            } else if (spell.find("Extreme") != std::string::npos || (spell == "Soins" && power >= 25)) {
                attacker->healExtreme();
                logMsg += "-> Canalise une magie de Soins (Extrême : toutes les blessures guéries, sang restauré).";
            } else { // Moyen par défaut
                attacker->healWounds(2);
                logMsg += "-> Canalise une magie de Soins (Moyen : blessures <= 2 guéries).";
            }
        } else { // Offensive par défaut
            std::optional<int> preArmor;
            if (defender->armor.has_value()) preArmor = defender->armor->durability;

            bool wasParrying = (defender->activeParries > 0);
            
            float magCostMult = action.overclockMultiplier;
            if (attacker->catalyst.has_value()) {
                magCostMult *= (1.0f + static_cast<float>(power) / 100.0f);
            }
            
            int eff = CombatSystem::executeAttack(*attacker, *defender, magCostMult, DamageNature::Magique, DamageType::Feu, DataStore::getInstance().getWeaponDamageMultipliers());
            
            if (eff == -98) {
                logMsg += "-> La magie offensive (Feu) est esquivée !";
            } else if (eff == -97) {
                logMsg += "-> La magie offensive (Feu) est bloquée par la parade !";
            } else if (eff == -99) {
                logMsg += "-> La magie offensive (Feu) est bloquée par l'armure !";
            } else {
                logMsg += "-> Lance un sort de Feu et inflige un " + getStageName(eff) + " magique (Feu)";
                if (wasParrying) {
                    int pct = (defender->getNormalizedClass() == "AEGIS") ? 25 : 10;
                    logMsg += " (paré, efficacité réduite de " + std::to_string(pct) + "%)";
                }
            }

            // Log armor durability loss
            if (defender->armor.has_value() && preArmor.has_value()) {
                int currentDur = defender->armor->durability;
                int diff = preArmor.value() - currentDur;
                if (diff > 0) {
                    logMsg += std::format("\n  [Armure] {} subit -{} de durabilité ({}/{})",
                                          defender->armor->name, diff, currentDur, defender->armor->maxDurability);
                    if (currentDur == 0 && preArmor.value() > 0) {
                        logMsg += std::format("\n  [Armure] {} est rompue !", defender->armor->name);
                    }
                }
            }
        }

        if (defender->isDead()) {
            logMsg += "\n" + defender->getName() + " est K.O !";
        }
    }
    
    logs.push_back(logMsg);
}

Simulator::TurnResult Simulator::resolveTurn() {
    TurnResult result;
    if (!fighter1 || !fighter2) return result;
    
    fighter1->currentTurn = currentTurn;
    fighter2->currentTurn = currentTurn;
    
    Entity* speedFirst = &(*fighter1);
    Entity* speedSecond = &(*fighter2);
    std::vector<QueuedAction>* firstActionsPtr = &p1Actions;
    std::vector<QueuedAction>* secondActionsPtr = &p2Actions;
    ControlMode firstMode = p1Mode;
    ControlMode secondMode = p2Mode;
    int firstFree = p1FreeActions;
    int secondFree = p2FreeActions;
    int firstPlayerNum = 1;
    int secondPlayerNum = 2;

    if (fighter2->getEffectiveVitesse() > fighter1->getEffectiveVitesse()) {
        speedFirst = &(*fighter2);
        speedSecond = &(*fighter1);
        firstActionsPtr = &p2Actions;
        secondActionsPtr = &p1Actions;
        firstMode = p2Mode;
        secondMode = p1Mode;
        firstFree = p2FreeActions;
        secondFree = p1FreeActions;
        firstPlayerNum = 2;
        secondPlayerNum = 1;
    }

    fetchAutomatedActions(*speedFirst, *firstActionsPtr, firstFree, firstMode, firstPlayerNum);
    fetchAutomatedActions(*speedSecond, *secondActionsPtr, secondFree, secondMode, secondPlayerNum);

    result.logs.push_back("\n--- Résolution du Tour " + std::to_string(currentTurn) + " ---");

    // Reset temporary combat states
    fighter1->resetTemporaryCombatStates();
    fighter2->resetTemporaryCombatStates();

    Entity* first = &(*fighter1);
    Entity* second = &(*fighter2);
    const std::vector<QueuedAction>* firstActions = &p1Actions;
    const std::vector<QueuedAction>* secondActions = &p2Actions;
    
    if (fighter2->getEffectiveVitesse() > fighter1->getEffectiveVitesse()) {
        first = &(*fighter2);
        second = &(*fighter1);
        firstActions = &p2Actions;
        secondActions = &p1Actions;
    }

    size_t firstQueued = firstActions->size();
    size_t secondQueued = secondActions->size();
    size_t maxActions = std::max(firstQueued, secondQueued);

    // Phase 1 : Résolution de toutes les esquives et parades préparées (dans l'ordre de vitesse/alternance)
    for (size_t i = 0; i < maxActions; ++i) {
        if (i < firstQueued) {
            if (!first->isDead() && first->physicalReserve > 0) {
                const auto& action = firstActions->at(i);
                if (action.type == ActionType::Parry || action.type == ActionType::Dodge) {
                    executeSingleAction(first, second, action, i, result.logs);
                }
            }
        }
        if (second->isDead() || second->physicalReserve <= 0 || first->isDead() || first->physicalReserve <= 0) break;
        
        if (i < secondQueued) {
            if (!second->isDead() && second->physicalReserve > 0) {
                const auto& action = secondActions->at(i);
                if (action.type == ActionType::Parry || action.type == ActionType::Dodge) {
                    executeSingleAction(second, first, action, i, result.logs);
                }
            }
        }
        if (first->isDead() || first->physicalReserve <= 0 || second->isDead() || second->physicalReserve <= 0) break;
    }

    // Phase 2 : Résolution de toutes les attaques et magies (dans l'ordre de vitesse/alternance)
    if (!(first->isDead() || first->physicalReserve <= 0 || second->isDead() || second->physicalReserve <= 0)) {
        for (size_t i = 0; i < maxActions; ++i) {
            if (i < firstQueued) {
                if (!first->isDead() && first->physicalReserve > 0) {
                    const auto& action = firstActions->at(i);
                    if (action.type == ActionType::Attack || action.type == ActionType::Magic) {
                        executeSingleAction(first, second, action, i, result.logs);
                    }
                }
            }
            if (second->isDead() || second->physicalReserve <= 0 || first->isDead() || first->physicalReserve <= 0) break;
            
            if (i < secondQueued) {
                if (!second->isDead() && second->physicalReserve > 0) {
                    const auto& action = secondActions->at(i);
                    if (action.type == ActionType::Attack || action.type == ActionType::Magic) {
                        executeSingleAction(second, first, action, i, result.logs);
                    }
                }
            }
            if (first->isDead() || first->physicalReserve <= 0 || second->isDead() || second->physicalReserve <= 0) break;
        }
    }

    // Apply bleeding at turn end
    std::string bleedMsg = "\n--- Effets de Saignement ---";
    bool bleedingHappened = false;
    for (Entity* entity : { &(*fighter1), &(*fighter2) }) {
        if (!entity->isDead()) {
            int rate = entity->getBleedingRate();
            if (rate > 0) {
                entity->applyBleeding(rate);
                bleedMsg += "\n" + entity->getName() + " perd " + std::to_string(rate) + " tic(s) de sang (Sang restant : " + std::format("{:.1f}", entity->blood) + "/32.0).";
                bleedingHappened = true;
                if (entity->isDead()) {
                    bleedMsg += "\n" + entity->getName() + " succombe à l'hémorragie (K.O) !";
                }
            }
        }
    }
    if (!bleedingHappened) bleedMsg += "\nAucun saignement actif.";
    result.logs.push_back(bleedMsg + "\n");

    p1Actions.clear();
    p2Actions.clear();
    p1Finished = false;
    p2Finished = false;

    bool combatFinished = fighter1->isDead() || fighter2->isDead() || fighter1->physicalReserve <= 0 || fighter2->physicalReserve <= 0;
    if (combatFinished) {
        result.combatFinished = true;
        std::string endMsg = "\n======================================";
        endMsg += "\n           FIN DU COMBAT";
        endMsg += "\n======================================";
        
        bool f1_out = fighter1->isDead() || fighter1->physicalReserve <= 0;
        bool f2_out = fighter2->isDead() || fighter2->physicalReserve <= 0;
        
        if (f1_out && f2_out) {
            result.winnerName = "";
            result.reason = "match nul";
            endMsg += "\nMatch nul ! Les deux combattants sont hors de combat.";
        } else if (f1_out) {
            result.winnerName = fighter2->getName();
            result.reason = fighter1->isDead() ? "mort" : "épuisement";
            endMsg += "\n" + fighter1->getName() + " est hors de combat (" + result.reason + ").";
            endMsg += "\nVictoire de " + fighter2->getName() + " !";
        } else {
            result.winnerName = fighter1->getName();
            result.reason = fighter2->isDead() ? "mort" : "épuisement";
            endMsg += "\n" + fighter2->getName() + " est hors de combat (" + result.reason + ").";
            endMsg += "\nVictoire de " + fighter1->getName() + " !";
        }
        endMsg += "\n======================================\n";
        result.logs.push_back(endMsg);
    } else {
        currentTurn++;
        result.logs.push_back("--- Préparation du Tour " + std::to_string(currentTurn) + " ---\n");
    }

    return result;
}