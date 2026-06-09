#include "simulator.hpp"
#include "../systems/CombatSystem.hpp"
#include "../data/DataStore.hpp"
#include <algorithm>
#include <cmath>
#include <format>

static int parseDuration(const nlohmann::json& val) {
    if (val.is_number()) {
        return val.get<int>();
    }
    if (val.is_string()) {
        std::string str = val.get<std::string>();
        size_t idx = 0;
        try {
            int d = std::stoi(str, &idx);
            return d;
        } catch (...) {}
    }
    return 1;
}


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
    fighter1->invulnerableTurnsLeft = 0;
    fighter2->invulnerableTurnsLeft = 0;
}

void Simulator::addActionP1(ActionType type, const std::string& magicSpell, bool useCatalyst) {
    if (fighter1.has_value()) {
        float multiplier = getNextMultiplier(fighter1.value(), p1Actions, type, p1FreeActions);
        p1Actions.push_back({type, multiplier, magicSpell, useCatalyst});
    }
}

void Simulator::addActionP2(ActionType type, const std::string& magicSpell, bool useCatalyst) {
    if (fighter2.has_value()) {
        float multiplier = getNextMultiplier(fighter2.value(), p2Actions, type, p2FreeActions);
        p2Actions.push_back({type, multiplier, magicSpell, useCatalyst});
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
        if (entity.isDead() || entity.physicalReserve <= 0 || entity.invulnerableTurnsLeft > 0) break;
        
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
    if (attacker->invulnerableTurnsLeft > 0) {
        logs.push_back(attacker->getName() + " : Incapacité (Invulnérabilité active, ne peut pas agir).");
        return;
    }
    
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
        } else if (eff == -96) {
            logMsg += "-> L'attaque glisse sur sa protection d'invulnérabilité !";
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

        float totalBurnMult = 0.0f;
        std::vector<std::string> healPowers;
        std::string sourceSpell = "Bal des lucioles";
        
        if (attacker->balDesLuciolesActive) {
            totalBurnMult += 0.9f;
            healPowers.push_back("moyen");
        }
        for (const auto& ae : attacker->activeEffects) {
            if (ae.type == "attack_buff" || ae.type == "bal_des_lucioles") {
                totalBurnMult += ae.burnMultiplier;
                if (!ae.healPower.empty()) {
                    healPowers.push_back(ae.healPower);
                }
                if (!ae.spellName.empty()) {
                    sourceSpell = ae.spellName;
                }
            }
        }

        if (totalBurnMult > 0.0f && eff != -98 && eff != -96) {
            float attForce = attacker->getEffectiveForceMagique() * totalBurnMult;
            float defRes = defender->getEffectiveResistanceMagique();
            int refLevel = (attForce < defRes) ? attacker->stade : defender->stade;
            int burnEff = CombatSystem::calculateStatDifference(attForce, defRes, refLevel);
            float resist = defender->getResistanceTo(DamageType::Feu);
            if (resist > 0.0f && burnEff >= 0) {
                burnEff = static_cast<int>(burnEff * (1.0f - resist));
            }
            defender->applyWound(burnEff, DamageType::Feu);
            
            logMsg += std::format("\n  [{}] Brûle l'adversaire (puissance {:.2f} vs RM {:.2f} -> {})",
                                  sourceSpell, attForce, defRes, getStageName(burnEff));
            
            for (const auto& hp : healPowers) {
                int maxWound = 2; // Moyen par défaut
                std::string hpLower = hp;
                std::transform(hpLower.begin(), hpLower.end(), hpLower.begin(), ::tolower);
                if (hpLower == "tres_faible" || hpLower == "tres faible" || hpLower == "très faible") maxWound = -2;
                else if (hpLower == "faible" || hpLower == "neutre") maxWound = 0;
                else if (hpLower == "fort") maxWound = 3;
                else if (hpLower == "tres_fort" || hpLower == "tres fort" || hpLower == "très fort") maxWound = 4;
                else if (hpLower == "extreme") maxWound = 999;
                
                if (maxWound == 999) {
                    attacker->healExtreme();
                    logMsg += " et se soigne totalement.";
                } else {
                    attacker->healWounds(maxWound);
                    std::string hpLabel = (maxWound == 2) ? "Moyen" : hpLower;
                    logMsg += std::format(" et se soigne du {}.", hpLabel);
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
        
        std::string spell = action.magicSpell;
        bool useCat = action.useCatalyst;
        int power = static_cast<int>(attacker->getEffectiveForceMagique());

        if (spell.empty()) {
            if (!attacker->catalysts.empty()) {
                spell = attacker->catalysts[0].magicType;
                useCat = true;
            } else {
                spell = attacker->magicType;
                useCat = false;
            }
        }

        Catalyst* activeCat = nullptr;
        if (useCat) {
            for (auto& cat : attacker->catalysts) {
                if (cat.magicType == spell) {
                    activeCat = &cat;
                    break;
                }
            }
        }

        if (useCat && activeCat) {
            power = activeCat->power;
        }

        auto spellOpt = DataStore::getInstance().getSpell(spell);
        if (!spellOpt) {
            logMsg += std::format("-> Échec : sort inconnu '{}'.", spell);
            logs.push_back(logMsg);
            return;
        }

        float manaCost = spellOpt->cost;

        if (!attacker->isMonster) {
            if (useCat && activeCat) {
                if (activeCat->reserve < static_cast<int>(manaCost)) {
                    logMsg += "-> Échec : réserve du catalyseur insuffisante (" + std::format("{:.1f}", manaCost) + " requise, restant : " + std::to_string(activeCat->reserve) + ")";
                    logs.push_back(logMsg);
                    return;
                }
                activeCat->reserve -= static_cast<int>(manaCost);
                // Sync backwards compatible single catalyst field
                if (!attacker->catalysts.empty() && attacker->catalyst.has_value()) {
                    attacker->catalyst = attacker->catalysts[0];
                }
            } else {
                if (!attacker->consumeMagicReserve(manaCost)) {
                    logMsg += "-> Échec : réserve magique insuffisante (" + std::format("{:.1f}", manaCost) + " requise, restant : " + std::format("{:.1f}", attacker->magicReserve) + ")";
                    logs.push_back(logMsg);
                    return;
                }
            }
        }
        if (spellOpt) {
            for (const auto& effect : spellOpt->effects) {
                std::string type = effect.value("type", "");
                // transform to lowercase
                std::transform(type.begin(), type.end(), type.begin(), ::tolower);

                if (type == "boost" || type == "stat_boost") {
                    int duration = 0;
                    if (effect.contains("duration")) {
                        duration = parseDuration(effect["duration"]);
                    }
                    float forceBoost = effect.value("force", 0.0f);
                    float speedBoost = effect.value("vitesse", effect.value("speed", 0.0f));
                    attacker->applyStatBoost(forceBoost, speedBoost);
                    if (duration > 0) {
                        ActiveEffect ae;
                        ae.type = "boost";
                        ae.duration = duration;
                        ae.forceBoost = forceBoost;
                        ae.speedBoost = speedBoost;
                        ae.spellName = spellOpt->name;
                        attacker->activeEffects.push_back(ae);
                    }
                    std::string statsStr = (forceBoost > 0.0f && speedBoost > 0.0f) ? "la force et la vitesse" 
                                         : (forceBoost > 0.0f ? "la force" : "la vitesse");
                    float boostLevel = forceBoost > 0.0f ? forceBoost : speedBoost;
                    logMsg += std::format("-> {} s'est boosté {} grâce à {} de {:.1f}",
                                          attacker->getName(), statsStr, spellOpt->name, boostLevel);
                    if (duration > 0) {
                        logMsg += std::format(" (pendant {} tours)", duration);
                    }
                    logMsg += ".";
                } else if (type == "heal" || type == "heal_scaling" || type == "heal_fixed") {
                    int duration = 0;
                    if (effect.contains("duration")) {
                        duration = parseDuration(effect["duration"]);
                    }
                    std::string powerStr = effect.value("power", "");
                    if (powerStr.empty()) {
                        if (effect.value("extreme", false)) {
                            powerStr = "extreme";
                        } else {
                            powerStr = "moyen";
                        }
                    }
                    std::transform(powerStr.begin(), powerStr.end(), powerStr.begin(), ::tolower);
                    
                    std::string hpLabel = powerStr;
                    if (powerStr == "extreme") {
                        attacker->healExtreme();
                    } else if (powerStr == "scaling") {
                        int maxWound = -999;
                        bool extreme = false;
                        if (power >= 25) {
                            extreme = true;
                        } else {
                            if (power < 5) maxWound = -2;
                            else if (power < 10) maxWound = 0;
                            else if (power < 20) maxWound = 2;
                            else if (power < 25) maxWound = 3;
                        }
                        if (extreme) {
                            attacker->healExtreme();
                            hpLabel = "extreme";
                        } else {
                            attacker->healWounds(maxWound);
                            hpLabel = std::format("blessures <= {}", maxWound);
                        }
                    } else {
                        int maxWound = 2; // Moyen par défaut
                        if (powerStr == "tres_faible" || powerStr == "tres faible" || powerStr == "très faible") maxWound = -2;
                        else if (powerStr == "faible" || powerStr == "neutre") maxWound = 0;
                        else if (powerStr == "fort") maxWound = 3;
                        else if (powerStr == "tres_fort" || powerStr == "tres fort" || powerStr == "très fort") maxWound = 4;
                        
                        attacker->healWounds(maxWound);
                    }

                    logMsg += std::format("-> {} s'est soigné à l'aide de {} (niveau {})", 
                                          attacker->getName(), spellOpt->name, hpLabel);

                    if (duration > 1) {
                        ActiveEffect ae;
                        ae.type = "heal";
                        ae.duration = duration - 1; // Le soin immédiat a déjà eu lieu
                        ae.power = powerStr;
                        ae.casterMagicPower = power;
                        ae.spellName = spellOpt->name;
                        attacker->activeEffects.push_back(ae);
                        logMsg += std::format(" (continu pendant {} tours)", duration);
                    }
                    logMsg += ".";
                } else if (type == "invulnerability" || type == "invulnerable") {
                    int duration = parseDuration(effect.value("duration", nlohmann::json(2)));
                    attacker->invulnerableTurnsLeft = duration;
                    
                    ActiveEffect ae;
                    ae.type = "invulnerability";
                    ae.duration = duration;
                    ae.spellName = spellOpt->name;
                    attacker->activeEffects.push_back(ae);

                    logMsg += std::format(" et devient invulnérable pendant {} tours !", duration);
                } else if (type == "bal_des_lucioles" || type == "attack_buff") {
                    int duration = parseDuration(effect.value("duration", nlohmann::json(1)));
                    float burnMultiplier = effect.value("burn_multiplier", effect.value("burn", 0.9f));
                    std::string healPower = effect.value("heal_power", effect.value("heal", "moyen"));

                    ActiveEffect ae;
                    ae.type = "attack_buff";
                    ae.duration = duration;
                    ae.burnMultiplier = burnMultiplier;
                    ae.healPower = healPower;
                    ae.spellName = spellOpt->name;
                    attacker->activeEffects.push_back(ae);

                    attacker->balDesLuciolesActive = true;
                    if (spellOpt->name == "Bal des lucioles" || type == "bal_des_lucioles") {
                        logMsg += "-> Canalise la magie de feu [Bal des lucioles] : s'entoure de flammes pour ce tour.";
                    } else {
                        logMsg += std::format("-> Canalise le buff d'attaque [{}] pendant {} tours.", spellOpt->name, duration);
                    }
                } else if (type == "damage") {
                    std::string natureStr = effect.value("nature", "Magique");
                    std::string dmgTypeStr = effect.value("damage_type", "Feu");
                    // lowercase them
                    std::transform(natureStr.begin(), natureStr.end(), natureStr.begin(), ::tolower);
                    std::transform(dmgTypeStr.begin(), dmgTypeStr.end(), dmgTypeStr.begin(), ::tolower);

                    DamageNature nat = (natureStr == "physique") ? DamageNature::Physique : DamageNature::Magique;
                    DamageType dt = DamageType::Feu;
                    if (dmgTypeStr == "contondant") dt = DamageType::Contondant;
                    else if (dmgTypeStr == "tranchant") dt = DamageType::Tranchant;
                    else if (dmgTypeStr == "corrosion") dt = DamageType::Corrosion;
                    else if (dmgTypeStr == "neutre") dt = DamageType::Neutre;
                    
                    std::optional<int> preArmor;
                    if (defender->armor.has_value()) preArmor = defender->armor->durability;

                    bool wasParrying = (defender->activeParries > 0);
                    float magCostMult = action.overclockMultiplier * effect.value("multiplier", effect.value("power_multiplier", 1.0f));
                    if (attacker->catalyst.has_value()) {
                        magCostMult *= (1.0f + static_cast<float>(power) / 100.0f);
                    }
                    
                    int eff = CombatSystem::executeAttack(*attacker, *defender, magCostMult, nat, dt, DataStore::getInstance().getWeaponDamageMultipliers());
                    
                    if (eff == -98) {
                        logMsg += std::format("-> La magie offensive ({}) est esquivée !", dmgTypeStr);
                    } else if (eff == -97) {
                        logMsg += "-> La magie offensive est bloquée par la parade !";
                    } else if (eff == -99) {
                        logMsg += "-> La magie offensive est bloquée par l'armure !";
                    } else if (eff == -96) {
                        logMsg += "-> La magie offensive glisse sur sa protection d'invulnérabilité !";
                    } else {
                        logMsg += std::format("-> Lance un sort de {} et inflige un {} magique ({})", dmgTypeStr, getStageName(eff), dmgTypeStr);
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

    // Update and decrement active effects at turn end
    std::string effectMsg = "\n--- Résolution des Effets Continus ---";
    std::vector<std::string> effectLogs;
    if (!fighter1->isDead() && fighter1->physicalReserve > 0) {
        fighter1->updateActiveEffects(effectLogs);
        fighter1->decrementActiveEffects(effectLogs);
    }
    if (!fighter2->isDead() && fighter2->physicalReserve > 0) {
        fighter2->updateActiveEffects(effectLogs);
        fighter2->decrementActiveEffects(effectLogs);
    }
    if (!effectLogs.empty()) {
        for (const auto& log : effectLogs) {
            effectMsg += "\n" + log;
        }
        result.logs.push_back(effectMsg + "\n");
    }

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
        if (fighter1->invulnerableTurnsLeft > 0) fighter1->invulnerableTurnsLeft--;
        if (fighter2->invulnerableTurnsLeft > 0) fighter2->invulnerableTurnsLeft--;
        
        float v1 = fighter1->getEffectiveVitesse();
        float v2 = fighter2->getEffectiveVitesse();
        int refLevel = std::min(fighter1->stade, fighter2->stade);
        int speedDiff = CombatSystem::calculateStatDifference(v1, v2, refLevel);
        p1FreeActions = 2 + (speedDiff > 0 ? speedDiff : 0);
        p2FreeActions = 2 + (speedDiff < 0 ? -speedDiff : 0);
        result.logs.push_back("--- Préparation du Tour " + std::to_string(currentTurn) + " ---\n");
    }

    fighter1->balDesLuciolesActive = false;
    fighter2->balDesLuciolesActive = false;
    return result;
}