#include "../include/RunPage.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "../../back/data/DataStore.hpp"
#include "../../back/systems/CombatSystem.hpp"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

static QString getStageName(int eff) {
    if (eff == -99) return "Bloqué par l'armure";
    if (eff == 0) return "Neutre";
    QString name;
    switch (std::abs(eff)) {
        case 1: name = "Faveur"; break;
        case 2: name = "Avantage"; break;
        case 3: name = "Efficace"; break;
        case 4: name = "Surpuissance"; break;
        case 5: name = "Domination"; break;
        case 6: name = "Ecrasement"; break;
        case 7: name = "Tyrannie"; break;
        default: name = "Inconnu"; break;
    }
    if (eff < 0) return "Sous-" + name;
    return name;
}

static QString getDamageTypeName(PhysicalDamageType type) {
    switch (type) {
        case PhysicalDamageType::Neutre: return "Neutre (Pugilat)";
        case PhysicalDamageType::Contondant: return "Contondant";
        case PhysicalDamageType::Tranchant: return "Tranchant";
    }
    return "Inconnu";
}

RunPage::RunPage(QWidget *parent) : QWidget(parent) {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // --- Selection UI ---
    selectionWidget = new QWidget();
    QVBoxLayout* selLayout = new QVBoxLayout(selectionWidget);
    selLayout->addWidget(new QLabel("Character 1:"));
    char1Combo = new QComboBox();
    selLayout->addWidget(char1Combo);
    
    selLayout->addWidget(new QLabel("Character 2:"));
    char2Combo = new QComboBox();
    selLayout->addWidget(char2Combo);
    
    runButton = new QPushButton("Lancer le combat");
    selLayout->addWidget(runButton);
    mainLayout->addWidget(selectionWidget);
    
    // --- Combat UI ---
    combatWidget = new QWidget();
    QVBoxLayout* combatLayout = new QVBoxLayout(combatWidget);
    
    QHBoxLayout* fightersLayout = new QHBoxLayout();
    
    // Fighter 1 UI
    QVBoxLayout* f1Layout = new QVBoxLayout();
    p1NameLabel = new QLabel("P1");
    p1NameLabel->setStyleSheet("font-weight: bold; font-size: 16px;");
    p1HpLabel = new QLabel("HP: ");
    p1AttackBtn = new QPushButton("Préparer Attaque");
    p1CancelBtn = new QPushButton("Annuler Action");
    f1Layout->addWidget(p1NameLabel);
    f1Layout->addWidget(p1HpLabel);
    f1Layout->addWidget(p1AttackBtn);
    f1Layout->addWidget(p1CancelBtn);
    
    // Fighter 2 UI
    QVBoxLayout* f2Layout = new QVBoxLayout();
    p2NameLabel = new QLabel("P2");
    p2NameLabel->setStyleSheet("font-weight: bold; font-size: 16px;");
    p2HpLabel = new QLabel("HP: ");
    p2AttackBtn = new QPushButton("Préparer Attaque");
    p2CancelBtn = new QPushButton("Annuler Action");
    f2Layout->addWidget(p2NameLabel);
    f2Layout->addWidget(p2HpLabel);
    f2Layout->addWidget(p2AttackBtn);
    f2Layout->addWidget(p2CancelBtn);
    
    fightersLayout->addLayout(f1Layout);
    fightersLayout->addStretch();
    
    QVBoxLayout* centerLayout = new QVBoxLayout();
    QLabel* vsLabel = new QLabel("VS");
    vsLabel->setAlignment(Qt::AlignCenter);
    centerLayout->addWidget(vsLabel);
    endTurnBtn = new QPushButton("Résoudre le Tour");
    centerLayout->addWidget(endTurnBtn);
    fightersLayout->addLayout(centerLayout);
    
    fightersLayout->addStretch();
    fightersLayout->addLayout(f2Layout);
    
    combatLayout->addLayout(fightersLayout);
    
    combatLog = new QTextEdit();
    combatLog->setReadOnly(true);
    combatLayout->addWidget(combatLog);
    
    QPushButton* backButton = new QPushButton("Retour à la sélection");
    combatLayout->addWidget(backButton);
    
    mainLayout->addWidget(combatWidget);
    combatWidget->setVisible(false); // Hide combat UI initially
    mainLayout->addStretch();

    // --- Connections ---
    connect(runButton, &QPushButton::clicked, this, &RunPage::startCombat);
    connect(backButton, &QPushButton::clicked, [this]() {
        combatWidget->setVisible(false);
        selectionWidget->setVisible(true);
    });
    
    connect(p1AttackBtn, &QPushButton::clicked, [this]() {
        p1QueuedAttacks++;
        updateCombatUI();
    });
    connect(p1CancelBtn, &QPushButton::clicked, [this]() {
        if (p1QueuedAttacks > 0) p1QueuedAttacks--;
        updateCombatUI();
    });
    connect(p2AttackBtn, &QPushButton::clicked, [this]() {
        p2QueuedAttacks++;
        updateCombatUI();
    });
    connect(p2CancelBtn, &QPushButton::clicked, [this]() {
        if (p2QueuedAttacks > 0) p2QueuedAttacks--;
        updateCombatUI();
    });
    connect(endTurnBtn, &QPushButton::clicked, this, &RunPage::resolveTurn);

    loadEntities();
}

void RunPage::loadEntities() {
    // For testing, let's inject two dummy entities if the datastore is empty
    auto names = DataStore::getInstance().getAvailableEntityNames();
    if (names.empty()) {
        char1Combo->addItem("Guerrier (Test)");
        char2Combo->addItem("Mage (Test)");
    } else {
        for (const auto& name : names) {
            char1Combo->addItem(QString::fromStdString(name));
            char2Combo->addItem(QString::fromStdString(name));
        }
    }
}

void RunPage::startCombat() {
    QString n1 = char1Combo->currentText();
    QString n2 = char2Combo->currentText();
    
    auto opt1 = DataStore::getInstance().getEntityTemplate(n1.toStdString());
    auto opt2 = DataStore::getInstance().getEntityTemplate(n2.toStdString());
    
    fighter1 = opt1 ? opt1 : Entity(n1.toStdString());
    fighter2 = opt2 ? opt2 : Entity(n2.toStdString());
    
    // In RunPage::startCombat()
    // Give them some default stats for testing if they are new
    if (!opt1) { fighter1->force = 10.0f; fighter1->blood = 32.0f; fighter1->stade = 1; }
    if (!opt2) { fighter2->force = 8.0f; fighter2->blood = 32.0f; fighter2->stade = 1; }
    
    // Reset damage (wounds vector)
    fighter1->wounds.clear();
    fighter2->wounds.clear();
    
    p1QueuedAttacks = 0;
    p2QueuedAttacks = 0;
    
    float v1 = fighter1->vitesse;
    float v2 = fighter2->vitesse;
    p1FreeActions = 2 + (v1 >= v2 && v2 > 0 ? static_cast<int>(v1 / v2) : 0);
    p2FreeActions = 2 + (v2 >= v1 && v1 > 0 ? static_cast<int>(v2 / v1) : 0);

    currentTurn = 1;
    
    // Set buttons ready
    p1AttackBtn->setEnabled(true);
    p2AttackBtn->setEnabled(true);
    p1CancelBtn->setEnabled(false);
    p2CancelBtn->setEnabled(false);
    endTurnBtn->setEnabled(true);

    selectionWidget->setVisible(false);
    combatWidget->setVisible(true);
    combatLog->clear();
    combatLog->append("Le combat commence");
    
    updateCombatUI();
}

void RunPage::updateCombatUI() {
    if (fighter1 && fighter2) {
        auto formatWounds = [](const std::vector<Wound>& w) {
            if (w.empty()) return QString("Aucune");
            QString res = "[";
            for (size_t i = 0; i < w.size(); ++i) {
                res += getStageName(w[i].effectiveness);
                if (w[i].damageType == PhysicalDamageType::Tranchant) {
                    res += " (Tranchante)";
                } else if (w[i].damageType == PhysicalDamageType::Contondant) {
                    res += " (Contondante)";
                } else {
                    res += " (Neutre)";
                }
                if (i < w.size() - 1) res += ", ";
            }
            res += "]";
            return res;
        };
        
        p1NameLabel->setText(QString::fromStdString(fighter1->getName()));
        if (fighter1) {
            QString armorInfo = "Armure : Aucune";
            if (fighter1->armor.has_value()) {
                const auto& armor = fighter1->armor.value();
                armorInfo = QString("Armure : %1 (Durabilité : %2/%3)")
                            .arg(QString::fromStdString(armor.name))
                            .arg(armor.durability)
                            .arg(armor.maxDurability);
                if (armor.durability <= 0) {
                    armorInfo += " [ROMPUE]";
                }
            }
            p1HpLabel->setText("Blessures : " + formatWounds(fighter1->wounds) + "\n" +
                               "Saignement : " + QString::fromStdString(fighter1->getBleedingState()) + " (" + QString::number(fighter1->getBleedingRate()) + " tics/tour)\n" +
                               "Sang : " + QString::number(fighter1->blood, 'f', 1) + " / 32.0 tics\n" +
                               "Endurance : " + QString::number(fighter1->physicalReserve) + " / " + QString::number(fighter1->maxPhysicalReserve) + 
                               " (" + QString::fromStdString(fighter1->getPhysicalState()) + ")\n" +
                               armorInfo);
        }
        
        p2NameLabel->setText(QString::fromStdString(fighter2->getName()));
        if (fighter2) {
            QString armorInfo = "Armure : Aucune";
            if (fighter2->armor.has_value()) {
                const auto& armor = fighter2->armor.value();
                armorInfo = QString("Armure : %1 (Durabilité : %2/%3)")
                            .arg(QString::fromStdString(armor.name))
                            .arg(armor.durability)
                            .arg(armor.maxDurability);
                if (armor.durability <= 0) {
                    armorInfo += " [ROMPUE]";
                }
            }
            p2HpLabel->setText("Blessures : " + formatWounds(fighter2->wounds) + "\n" +
                               "Saignement : " + QString::fromStdString(fighter2->getBleedingState()) + " (" + QString::number(fighter2->getBleedingRate()) + " tics/tour)\n" +
                               "Sang : " + QString::number(fighter2->blood, 'f', 1) + " / 32.0 tics\n" +
                               "Endurance : " + QString::number(fighter2->physicalReserve) + " / " + QString::number(fighter2->maxPhysicalReserve) + 
                               " (" + QString::fromStdString(fighter2->getPhysicalState()) + ")\n" +
                               armorInfo);
        }
        
        if (fighter1->isDead() || fighter2->isDead()) {
            p1AttackBtn->setEnabled(false);
            p1CancelBtn->setEnabled(false);
            p2AttackBtn->setEnabled(false);
            p2CancelBtn->setEnabled(false);
            endTurnBtn->setEnabled(false);
        } else {
            endTurnBtn->setEnabled(p1QueuedAttacks > 0 || p2QueuedAttacks > 0);
            p1AttackBtn->setEnabled(true);
            p2AttackBtn->setEnabled(true);
            p1CancelBtn->setEnabled(p1QueuedAttacks > 0);
            p2CancelBtn->setEnabled(p2QueuedAttacks > 0);
            
            auto setBtnText = [](QPushButton* btn, int queued, int freeCount) {
                float multipliers[] = {2.0f, 2.3f, 2.6f, 3.4f, 5.0f, 7.0f};
                float nextMult = 1.0f;
                if (queued >= freeCount) {
                    int idx = queued - freeCount;
                    if (idx > 5) idx = 5;
                    nextMult = multipliers[idx];
                }
                
                if (queued >= freeCount) {
                    btn->setText("Préparer (Surcad. x" + QString::number(nextMult, 'f', 1) + ") [" + QString::number(queued) + " en file]");
                } else {
                    btn->setText("Préparer (" + QString::number(freeCount - queued) + " restants)");
                }
            };
            setBtnText(p1AttackBtn, p1QueuedAttacks, p1FreeActions);
            setBtnText(p2AttackBtn, p2QueuedAttacks, p2FreeActions);
        }
    }
}

void RunPage::resolveTurn() {
    if (!fighter1 || !fighter2) return;

    QString msg = "\n--- Résolution du Tour " + QString::number(currentTurn) + " ---";
    combatLog->append(msg);
    
    auto applyAttack = [this](Entity* attacker, Entity* defender, int actionIndex, int freeActions) {
        float multipliers[] = {2.0f, 2.3f, 2.6f, 3.4f, 5.0f, 7.0f};
        float multiplier = 1.0f;
        if (actionIndex >= freeActions) {
            int idx = actionIndex - freeActions;
            if (idx > 5) idx = 5;
            multiplier = multipliers[idx];
        }
        
        QString logMsg = QString::fromStdString(attacker->getName()) + " attaque (Action " + QString::number(actionIndex + 1) + ") : ";
        if (multiplier > 1.0f) logMsg += "[Surcadençage x" + QString::number(multiplier, 'f', 1) + "] ";
        
        std::optional<int> preArmor;
        if (defender->armor.has_value()) preArmor = defender->armor->durability;

        int eff = CombatSystem::executeAttack(*attacker, *defender, multiplier);
        logMsg += "inflige un " + getStageName(eff);
        
        if (defender->armor.has_value() && preArmor.has_value()) {
            int currentDur = defender->armor->durability;
            int diff = preArmor.value() - currentDur;
            if (diff > 0) {
                logMsg += QString("\n  [Armure] %1 subit -%2 de durabilité (%3/%4)")
                          .arg(QString::fromStdString(defender->armor->name))
                          .arg(diff).arg(currentDur).arg(defender->armor->maxDurability);
                if (currentDur == 0 && preArmor.value() > 0) logMsg += QString("\n  [Armure] %1 est rompue !").arg(QString::fromStdString(defender->armor->name));
            }
        }

        if (defender->isDead()) {
            if (defender->physicalReserve <= 0) logMsg += "\n" + QString::fromStdString(defender->getName()) + " s'effondre de fatigue (K.O) !";
            else logMsg += "\n" + QString::fromStdString(defender->getName()) + " est K.O !";
        }
        if (attacker->isDead() && attacker->physicalReserve <= 0) {
            logMsg += "\n" + QString::fromStdString(attacker->getName()) + " s'effondre de fatigue suite à son effort (K.O) !";
        }
        combatLog->append(logMsg);
    };

    Entity* first = &(*fighter1);
    Entity* second = &(*fighter2);
    int firstQueued = p1QueuedAttacks;
    int secondQueued = p2QueuedAttacks;
    int firstFree = p1FreeActions;
    int secondFree = p2FreeActions;
    
    if (fighter2->vitesse > fighter1->vitesse) {
        first = &(*fighter2);
        second = &(*fighter1);
        firstQueued = p2QueuedAttacks;
        secondQueued = p1QueuedAttacks;
        firstFree = p2FreeActions;
        secondFree = p1FreeActions;
    }

    int firstDone = 0;
    int secondDone = 0;

    while ((firstDone < firstQueued || secondDone < secondQueued) && !first->isDead() && !second->isDead()) {
        if (firstDone < firstQueued) {
            applyAttack(first, second, firstDone, firstFree);
            firstDone++;
        }
        if (second->isDead() || first->isDead()) break;
        
        if (secondDone < secondQueued) {
            applyAttack(second, first, secondDone, secondFree);
            secondDone++;
        }
        if (first->isDead() || second->isDead()) break;
    }

    // Apply bleeding at turn end
    QString bleedMsg = "\n--- Effets de Saignement ---";
    bool bleedingHappened = false;
    for (Entity* entity : { &(*fighter1), &(*fighter2) }) {
        if (!entity->isDead()) {
            int rate = entity->getBleedingRate();
            if (rate > 0) {
                entity->applyBleeding(rate);
                bleedMsg += "\n" + QString::fromStdString(entity->getName()) + " perd " + QString::number(rate) + " tic(s) de sang (Sang restant : " + QString::number(entity->blood, 'f', 1) + "/32.0).";
                bleedingHappened = true;
                if (entity->isDead()) {
                    bleedMsg += "\n" + QString::fromStdString(entity->getName()) + " succombe à l'hémorragie (K.O) !";
                }
            }
        }
    }
    if (!bleedingHappened) bleedMsg += "\nAucun saignement actif.";
    combatLog->append(bleedMsg + "\n");
    
    p1QueuedAttacks = 0;
    p2QueuedAttacks = 0;
    currentTurn++;
    
    combatLog->append("--- Préparation du Tour " + QString::number(currentTurn) + " ---\n");
    
    updateCombatUI();

    if (fighter1->isDead() || fighter2->isDead()) {
        saveCombatLog();
    }
}

void RunPage::saveCombatLog() {
    QDir dir("logs");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QString filename = "logs/Combat_" + 
                       QString::fromStdString(fighter1->getName()).replace(" ", "_") + "_vs_" + 
                       QString::fromStdString(fighter2->getName()).replace(" ", "_") + "_" + 
                       QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".log";
                       
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << combatLog->toPlainText();
        file.close();
    }
}
