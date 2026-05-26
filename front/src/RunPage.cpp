#include "../include/RunPage.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QProcess>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QElapsedTimer>
#include <QTimer>
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
    
    QGridLayout* grid = new QGridLayout();
    
    // Fighter 1 config
    grid->addWidget(new QLabel("Combattant 1 :"), 0, 0);
    char1Combo = new QComboBox();
    grid->addWidget(char1Combo, 0, 1);
    
    grid->addWidget(new QLabel("Contrôle 1 :"), 1, 0);
    char1ModeCombo = new QComboBox();
    char1ModeCombo->addItem("Manuel");
    char1ModeCombo->addItem("Script (Python)");
    char1ModeCombo->addItem("Externe (TCP)");
    grid->addWidget(char1ModeCombo, 1, 1);
    
    grid->addWidget(new QLabel("Arme 1 :"), 2, 0);
    char1WeaponCombo = new QComboBox();
    grid->addWidget(char1WeaponCombo, 2, 1);
    
    grid->addWidget(new QLabel("Armure 1 :"), 3, 0);
    char1ArmorCombo = new QComboBox();
    grid->addWidget(char1ArmorCombo, 3, 1);
    
    QLabel* scriptPathLabel1 = new QLabel("Script 1 :");
    grid->addWidget(scriptPathLabel1, 4, 0);
    scriptPathEdit1 = new QLineEdit();
    scriptPathEdit1->setText("scripts/ai_agent.py");
    grid->addWidget(scriptPathEdit1, 4, 1);
    
    QLabel* tcpHostLabel1 = new QLabel("TCP Hôte 1 :");
    grid->addWidget(tcpHostLabel1, 5, 0);
    tcpHostEdit1 = new QLineEdit();
    tcpHostEdit1->setText("127.0.0.1");
    grid->addWidget(tcpHostEdit1, 5, 1);
    
    QLabel* tcpPortLabel1 = new QLabel("TCP Port 1 :");
    grid->addWidget(tcpPortLabel1, 6, 0);
    tcpPortEdit1 = new QSpinBox();
    tcpPortEdit1->setRange(1, 65535);
    tcpPortEdit1->setValue(8080);
    grid->addWidget(tcpPortEdit1, 6, 1);

    // Fighter 2 config
    grid->addWidget(new QLabel("Combattant 2 :"), 0, 2);
    char2Combo = new QComboBox();
    grid->addWidget(char2Combo, 0, 3);
    
    grid->addWidget(new QLabel("Contrôle 2 :"), 1, 2);
    char2ModeCombo = new QComboBox();
    char2ModeCombo->addItem("Manuel");
    char2ModeCombo->addItem("Script (Python)");
    char2ModeCombo->addItem("Externe (TCP)");
    grid->addWidget(char2ModeCombo, 1, 3);
    
    grid->addWidget(new QLabel("Arme 2 :"), 2, 2);
    char2WeaponCombo = new QComboBox();
    grid->addWidget(char2WeaponCombo, 2, 3);
    
    grid->addWidget(new QLabel("Armure 2 :"), 3, 2);
    char2ArmorCombo = new QComboBox();
    grid->addWidget(char2ArmorCombo, 3, 3);
    
    QLabel* scriptPathLabel2 = new QLabel("Script 2 :");
    grid->addWidget(scriptPathLabel2, 4, 2);
    scriptPathEdit2 = new QLineEdit();
    scriptPathEdit2->setText("scripts/ai_agent.py");
    grid->addWidget(scriptPathEdit2, 4, 3);
    
    QLabel* tcpHostLabel2 = new QLabel("TCP Hôte 2 :");
    grid->addWidget(tcpHostLabel2, 5, 2);
    tcpHostEdit2 = new QLineEdit();
    tcpHostEdit2->setText("127.0.0.1");
    grid->addWidget(tcpHostEdit2, 5, 3);
    
    QLabel* tcpPortLabel2 = new QLabel("TCP Port 2 : ");
    grid->addWidget(tcpPortLabel2, 6, 2);
    tcpPortEdit2 = new QSpinBox();
    tcpPortEdit2->setRange(1, 65535);
    tcpPortEdit2->setValue(8080);
    grid->addWidget(tcpPortEdit2, 6, 3);

    // Connect mode toggles
    auto updateP1ModeVisibility = [this, scriptPathLabel1, tcpHostLabel1, tcpPortLabel1](int index) {
        bool isScript = (index == 1);
        bool isTcp = (index == 2);
        scriptPathLabel1->setVisible(isScript);
        scriptPathEdit1->setVisible(isScript);
        tcpHostLabel1->setVisible(isTcp);
        tcpHostEdit1->setVisible(isTcp);
        tcpPortLabel1->setVisible(isTcp);
        tcpPortEdit1->setVisible(isTcp);
    };
    connect(char1ModeCombo, &QComboBox::currentIndexChanged, updateP1ModeVisibility);
    updateP1ModeVisibility(char1ModeCombo->currentIndex());

    auto updateP2ModeVisibility = [this, scriptPathLabel2, tcpHostLabel2, tcpPortLabel2](int index) {
        bool isScript = (index == 1);
        bool isTcp = (index == 2);
        scriptPathLabel2->setVisible(isScript);
        scriptPathEdit2->setVisible(isScript);
        tcpHostLabel2->setVisible(isTcp);
        tcpHostEdit2->setVisible(isTcp);
        tcpPortLabel2->setVisible(isTcp);
        tcpPortEdit2->setVisible(isTcp);
    };
    connect(char2ModeCombo, &QComboBox::currentIndexChanged, updateP2ModeVisibility);
    updateP2ModeVisibility(char2ModeCombo->currentIndex());
    
    selLayout->addLayout(grid);
    
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
    p1ParryBtn = new QPushButton("Préparer Parade");
    p1DodgeBtn = new QPushButton("Préparer Esquive");
    p1PassBtn = new QPushButton("Finir le Tour");
    p1CancelBtn = new QPushButton("Annuler Action");
    f1Layout->addWidget(p1NameLabel);
    f1Layout->addWidget(p1HpLabel);
    f1Layout->addWidget(p1AttackBtn);
    f1Layout->addWidget(p1ParryBtn);
    f1Layout->addWidget(p1DodgeBtn);
    f1Layout->addWidget(p1PassBtn);
    f1Layout->addWidget(p1CancelBtn);
    
    // Fighter 2 UI
    QVBoxLayout* f2Layout = new QVBoxLayout();
    p2NameLabel = new QLabel("P2");
    p2NameLabel->setStyleSheet("font-weight: bold; font-size: 16px;");
    p2HpLabel = new QLabel("HP: ");
    p2AttackBtn = new QPushButton("Préparer Attaque");
    p2ParryBtn = new QPushButton("Préparer Parade");
    p2DodgeBtn = new QPushButton("Préparer Esquive");
    p2PassBtn = new QPushButton("Finir le Tour");
    p2CancelBtn = new QPushButton("Annuler Action");
    f2Layout->addWidget(p2NameLabel);
    f2Layout->addWidget(p2HpLabel);
    f2Layout->addWidget(p2AttackBtn);
    f2Layout->addWidget(p2ParryBtn);
    f2Layout->addWidget(p2DodgeBtn);
    f2Layout->addWidget(p2PassBtn);
    f2Layout->addWidget(p2CancelBtn);
    
    fightersLayout->addLayout(f1Layout);
    fightersLayout->addStretch();
    
    QVBoxLayout* centerLayout = new QVBoxLayout();
    QLabel* vsLabel = new QLabel("VS");
    vsLabel->setAlignment(Qt::AlignCenter);
    centerLayout->addWidget(vsLabel);
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
    
    auto addActionP1 = [this](ActionType type) {
        float multipliers[] = {2.0f, 2.3f, 2.6f, 3.4f, 5.0f, 7.0f};
        float multiplier = 1.0f;
        int queued = p1Actions.size();
        if (queued >= p1FreeActions) {
            int idx = queued - p1FreeActions;
            if (idx > 5) idx = 5;
            multiplier = multipliers[idx];
        }
        p1Actions.push_back({type, multiplier});
        updateCombatUI();
    };

    auto addActionP2 = [this](ActionType type) {
        float multipliers[] = {2.0f, 2.3f, 2.6f, 3.4f, 5.0f, 7.0f};
        float multiplier = 1.0f;
        int queued = p2Actions.size();
        if (queued >= p2FreeActions) {
            int idx = queued - p2FreeActions;
            if (idx > 5) idx = 5;
            multiplier = multipliers[idx];
        }
        p2Actions.push_back({type, multiplier});
        updateCombatUI();
    };

    connect(p1AttackBtn, &QPushButton::clicked, [this, addActionP1]() {
        addActionP1(ActionType::Attack);
    });
    connect(p1ParryBtn, &QPushButton::clicked, [this, addActionP1]() {
        addActionP1(ActionType::Parry);
    });
    connect(p1DodgeBtn, &QPushButton::clicked, [this, addActionP1]() {
        addActionP1(ActionType::Dodge);
    });
    connect(p1CancelBtn, &QPushButton::clicked, [this]() {
        p1Finished = false;
        if (!p1Actions.empty()) p1Actions.pop_back();
        updateCombatUI();
    });

    connect(p2AttackBtn, &QPushButton::clicked, [this, addActionP2]() {
        addActionP2(ActionType::Attack);
    });
    connect(p2ParryBtn, &QPushButton::clicked, [this, addActionP2]() {
        addActionP2(ActionType::Parry);
    });
    connect(p2DodgeBtn, &QPushButton::clicked, [this, addActionP2]() {
        addActionP2(ActionType::Dodge);
    });
    connect(p2CancelBtn, &QPushButton::clicked, [this]() {
        p2Finished = false;
        if (!p2Actions.empty()) p2Actions.pop_back();
        updateCombatUI();
    });

    connect(p1PassBtn, &QPushButton::clicked, [this]() {
        p1Finished = true;
        updateCombatUI();
        checkResolve();
    });
    connect(p2PassBtn, &QPushButton::clicked, [this]() {
        p2Finished = true;
        updateCombatUI();
        checkResolve();
    });

    loadEntities();
}

void RunPage::loadEntities() {
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

    // Populate weapons
    char1WeaponCombo->addItem("Aucune");
    char2WeaponCombo->addItem("Aucune");
    auto weaponNames = DataStore::getInstance().getAvailableWeaponNames();
    for (const auto& wName : weaponNames) {
        char1WeaponCombo->addItem(QString::fromStdString(wName));
        char2WeaponCombo->addItem(QString::fromStdString(wName));
    }

    // Populate armors
    char1ArmorCombo->addItem("Aucune");
    char2ArmorCombo->addItem("Aucune");
    auto armorNames = DataStore::getInstance().getAvailableArmorNames();
    for (const auto& aName : armorNames) {
        char1ArmorCombo->addItem(QString::fromStdString(aName));
        char2ArmorCombo->addItem(QString::fromStdString(aName));
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

    // Assign Weapon 1
    QString w1 = char1WeaponCombo->currentText();
    if (w1 != "Aucune") {
        auto wOpt = DataStore::getInstance().getWeaponTemplate(w1.toStdString());
        if (wOpt) fighter1->weapon = wOpt;
    } else {
        fighter1->weapon = std::nullopt;
    }

    // Assign Armor 1
    QString a1 = char1ArmorCombo->currentText();
    if (a1 != "Aucune") {
        auto aOpt = DataStore::getInstance().getArmorTemplate(a1.toStdString());
        if (aOpt) fighter1->armor = aOpt;
    } else {
        fighter1->armor = std::nullopt;
    }

    // Assign Weapon 2
    QString w2 = char2WeaponCombo->currentText();
    if (w2 != "Aucune") {
        auto wOpt = DataStore::getInstance().getWeaponTemplate(w2.toStdString());
        if (wOpt) fighter2->weapon = wOpt;
    } else {
        fighter2->weapon = std::nullopt;
    }

    // Assign Armor 2
    QString a2 = char2ArmorCombo->currentText();
    if (a2 != "Aucune") {
        auto aOpt = DataStore::getInstance().getArmorTemplate(a2.toStdString());
        if (aOpt) fighter2->armor = aOpt;
    } else {
        fighter2->armor = std::nullopt;
    }
    
    // Reset damage (wounds vector)
    fighter1->wounds.clear();
    fighter2->wounds.clear();
    
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
    
    // Set buttons ready
    p1AttackBtn->setEnabled(true);
    p1ParryBtn->setEnabled(true);
    p1DodgeBtn->setEnabled(true);
    p1PassBtn->setEnabled(true);
    p2AttackBtn->setEnabled(true);
    p2ParryBtn->setEnabled(true);
    p2DodgeBtn->setEnabled(true);
    p2PassBtn->setEnabled(true);
    p1CancelBtn->setEnabled(false);
    p2CancelBtn->setEnabled(false);

    selectionWidget->setVisible(false);
    combatWidget->setVisible(true);
    combatLog->clear();
    combatLog->append("Le combat commence");
    
    updateCombatUI();
    checkResolve();
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
            QString weaponInfo = "Arme : Aucune";
            if (fighter1->weapon.has_value()) {
                const auto& weapon = fighter1->weapon.value();
                weaponInfo = QString("Arme : %1 (Durabilité : %2/%3)")
                             .arg(QString::fromStdString(weapon.name))
                             .arg(weapon.durability)
                             .arg(weapon.maxDurability);
                if (weapon.durability <= 0) {
                    weaponInfo += " [ROMPUE]";
                }
            }
            auto formatActions = [](const std::vector<QueuedAction>& actions) {
                if (actions.empty()) return QString("Aucune action préparée");
                QStringList list;
                for (size_t i = 0; i < actions.size(); ++i) {
                    QString name;
                    if (actions[i].type == ActionType::Attack) name = "Attaque";
                    else if (actions[i].type == ActionType::Parry) name = "Parade";
                    else if (actions[i].type == ActionType::Dodge) name = "Esquive";
                    
                    if (actions[i].overclockMultiplier > 1.0f) {
                        name += " [Surcad. x" + QString::number(actions[i].overclockMultiplier, 'f', 1) + "]";
                    } else {
                        name += " [Gratuite]";
                    }
                    list << QString::number(i + 1) + ". " + name;
                }
                return list.join("\n");
            };

            p1HpLabel->setText("Blessures : " + formatWounds(fighter1->wounds) + "\n" +
                               "Saignement : " + QString::fromStdString(fighter1->getBleedingState()) + " (" + QString::number(fighter1->getBleedingRate()) + " tics/tour)\n" +
                               "Sang : " + QString::number(fighter1->blood, 'f', 1) + " / 32.0 tics\n" +
                               "Endurance : " + QString::number(fighter1->physicalReserve) + " / " + QString::number(fighter1->maxPhysicalReserve) + 
                               " (" + QString::fromStdString(fighter1->getPhysicalState()) + ")\n" +
                               armorInfo + "\n" +
                               weaponInfo + "\n\nActions préparées :\n" + formatActions(p1Actions));
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
            QString weaponInfo = "Arme : Aucune";
            if (fighter2->weapon.has_value()) {
                const auto& weapon = fighter2->weapon.value();
                weaponInfo = QString("Arme : %1 (Durabilité : %2/%3)")
                             .arg(QString::fromStdString(weapon.name))
                             .arg(weapon.durability)
                             .arg(weapon.maxDurability);
                if (weapon.durability <= 0) {
                    weaponInfo += " [ROMPUE]";
                }
            }
            auto formatActions = [](const std::vector<QueuedAction>& actions) {
                if (actions.empty()) return QString("Aucune action préparée");
                QStringList list;
                for (size_t i = 0; i < actions.size(); ++i) {
                    QString name;
                    if (actions[i].type == ActionType::Attack) name = "Attaque";
                    else if (actions[i].type == ActionType::Parry) name = "Parade";
                    else if (actions[i].type == ActionType::Dodge) name = "Esquive";
                    
                    if (actions[i].overclockMultiplier > 1.0f) {
                        name += " [Surcad. x" + QString::number(actions[i].overclockMultiplier, 'f', 1) + "]";
                    } else {
                        name += " [Gratuite]";
                    }
                    list << QString::number(i + 1) + ". " + name;
                }
                return list.join("\n");
            };

            p2HpLabel->setText("Blessures : " + formatWounds(fighter2->wounds) + "\n" +
                               "Saignement : " + QString::fromStdString(fighter2->getBleedingState()) + " (" + QString::number(fighter2->getBleedingRate()) + " tics/tour)\n" +
                               "Sang : " + QString::number(fighter2->blood, 'f', 1) + " / 32.0 tics\n" +
                               "Endurance : " + QString::number(fighter2->physicalReserve) + " / " + QString::number(fighter2->maxPhysicalReserve) + 
                               " (" + QString::fromStdString(fighter2->getPhysicalState()) + ")\n" +
                               armorInfo + "\n" +
                               weaponInfo + "\n\nActions préparées :\n" + formatActions(p2Actions));
        }
        
        if (fighter1->isDead() || fighter2->isDead() || fighter1->physicalReserve <= 0 || fighter2->physicalReserve <= 0) {
            p1AttackBtn->setEnabled(false);
            p1ParryBtn->setEnabled(false);
            p1DodgeBtn->setEnabled(false);
            p1PassBtn->setEnabled(false);
            p1CancelBtn->setEnabled(false);
            p2AttackBtn->setEnabled(false);
            p2ParryBtn->setEnabled(false);
            p2DodgeBtn->setEnabled(false);
            p2PassBtn->setEnabled(false);
            p2CancelBtn->setEnabled(false);
        } else {
            bool p1Manual = (char1ModeCombo->currentIndex() == 0);
            p1AttackBtn->setEnabled(p1Manual && !p1Finished);
            p1ParryBtn->setEnabled(p1Manual && !p1Finished);
            p1DodgeBtn->setEnabled(p1Manual && !p1Finished);
            p1PassBtn->setEnabled(p1Manual && !p1Finished);
            p1CancelBtn->setEnabled(p1Manual && (p1Finished || !p1Actions.empty()));

            bool p2Manual = (char2ModeCombo->currentIndex() == 0);
            p2AttackBtn->setEnabled(p2Manual && !p2Finished);
            p2ParryBtn->setEnabled(p2Manual && !p2Finished);
            p2DodgeBtn->setEnabled(p2Manual && !p2Finished);
            p2PassBtn->setEnabled(p2Manual && !p2Finished);
            p2CancelBtn->setEnabled(p2Manual && (p2Finished || !p2Actions.empty()));
            
            auto setBtnText = [](QPushButton* btnAttack, QPushButton* btnParry, QPushButton* btnDodge, const std::vector<QueuedAction>& actions, int freeCount) {
                float multipliers[] = {2.0f, 2.3f, 2.6f, 3.4f, 5.0f, 7.0f};
                int queued = actions.size();
                float nextMult = 1.0f;
                if (queued >= freeCount) {
                    int idx = queued - freeCount;
                    if (idx > 5) idx = 5;
                    nextMult = multipliers[idx];
                }
                
                QString suffix = queued >= freeCount ? " (Surcad. x" + QString::number(nextMult, 'f', 1) + ")" : " (Gratuit)";
                btnAttack->setText("Attaquer" + suffix);
                btnParry->setText("Parer" + suffix);
                btnDodge->setText("Esquiver" + suffix);
            };
            setBtnText(p1AttackBtn, p1ParryBtn, p1DodgeBtn, p1Actions, p1FreeActions);
            setBtnText(p2AttackBtn, p2ParryBtn, p2DodgeBtn, p2Actions, p2FreeActions);
        }
    }
}

void RunPage::resolveTurn() {
    if (!fighter1 || !fighter2) return;

    // Récupérer les modes de contrôle
    ControlMode p1Mode = static_cast<ControlMode>(char1ModeCombo->currentIndex());
    ControlMode p2Mode = static_cast<ControlMode>(char2ModeCombo->currentIndex());

    // Déterminer l'ordre pour requêter les IA/scripts (la plus rapide décide en premier)
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

    // Mettre à jour l'UI après que les IA ont choisi
    updateCombatUI();

    QString msg = "\n--- Résolution du Tour " + QString::number(currentTurn) + " ---";
    combatLog->append(msg);

    // Reset temporary combat states
    fighter1->activeParries = 0;
    fighter1->activeDodges = 0;
    fighter2->activeParries = 0;
    fighter2->activeDodges = 0;
    
    auto executeSingleAction = [this](Entity* attacker, Entity* defender, const QueuedAction& action, int actionIndex) {
        if (attacker->isDead() || attacker->physicalReserve <= 0) return;
        
        QString logMsg = QString::fromStdString(attacker->getName()) + " : ";
        
        if (action.type == ActionType::Attack) {
            logMsg += "Attaque (Action " + QString::number(actionIndex + 1) + ") ";
            if (action.overclockMultiplier > 1.0f) logMsg += "[Surcadençage x" + QString::number(action.overclockMultiplier, 'f', 1) + "] ";
            
            std::optional<int> preArmor;
            if (defender->armor.has_value()) preArmor = defender->armor->durability;

            std::optional<int> preWeapon;
            if (attacker->weapon.has_value()) preWeapon = attacker->weapon->durability;

            bool wasParrying = (defender->activeParries > 0);
            int eff = CombatSystem::executeAttack(*attacker, *defender, action.overclockMultiplier);
            
            if (eff == -98) {
                logMsg += "-> L'attaque est esquivée !";
            } else if (eff == -97) {
                logMsg += "-> L'attaque est bloquée par la parade (Bouclier en métal) !";
            } else if (eff == -99) {
                logMsg += "-> L'attaque est bloquée par l'armure !";
            } else {
                logMsg += "inflige un " + getStageName(eff);
                if (wasParrying) {
                    logMsg += " (paré, efficacité de l'attaque réduite de 10%)";
                }
            }

            // Log weapon durability loss
            if (attacker->weapon.has_value() && preWeapon.has_value()) {
                int currentDur = attacker->weapon->durability;
                int diff = preWeapon.value() - currentDur;
                if (diff > 0) {
                    logMsg += QString("\n  [Arme] %1 subit -%2 de durabilité (%3/%4)")
                              .arg(QString::fromStdString(attacker->weapon->name))
                              .arg(diff).arg(currentDur).arg(attacker->weapon->maxDurability);
                    if (currentDur == 0 && preWeapon.value() > 0) {
                        logMsg += QString("\n  [Arme] %1 est rompue !").arg(QString::fromStdString(attacker->weapon->name));
                    }
                }
            }

            // Log armor durability loss
            if (defender->armor.has_value() && preArmor.has_value()) {
                int currentDur = defender->armor->durability;
                int diff = preArmor.value() - currentDur;
                if (diff > 0) {
                    logMsg += QString("\n  [Armure] %1 subit -%2 de durabilité (%3/%4)")
                              .arg(QString::fromStdString(defender->armor->name))
                              .arg(diff).arg(currentDur).arg(defender->armor->maxDurability);
                    if (currentDur == 0 && preArmor.value() > 0) {
                        logMsg += QString("\n  [Armure] %1 est rompue !").arg(QString::fromStdString(defender->armor->name));
                    }
                }
            }

            if (defender->isDead()) {
                logMsg += "\n" + QString::fromStdString(defender->getName()) + " est K.O !";
            }
        } else if (action.type == ActionType::Parry) {
            logMsg += "Parade (Action " + QString::number(actionIndex + 1) + ") ";
            if (action.overclockMultiplier > 1.0f) logMsg += "[Surcadençage x" + QString::number(action.overclockMultiplier, 'f', 1) + "] ";
            CombatSystem::executeParry(*attacker, action.overclockMultiplier);
            logMsg += "-> Prépare une parade (dégâts de la prochaine attaque réduits de 10%).";
        } else if (action.type == ActionType::Dodge) {
            logMsg += "Esquive (Action " + QString::number(actionIndex + 1) + ") ";
            if (action.overclockMultiplier > 1.0f) logMsg += "[Surcadençage x" + QString::number(action.overclockMultiplier, 'f', 1) + "] ";
            CombatSystem::executeDodge(*attacker, action.overclockMultiplier);
            logMsg += "-> Prépare une esquive (évitera la prochaine attaque).";
        } else if (action.type == ActionType::Magic) {
            logMsg += "Magie (Action " + QString::number(actionIndex + 1) + ") ";
            if (action.overclockMultiplier > 1.0f) logMsg += "[Surcadençage x" + QString::number(action.overclockMultiplier, 'f', 1) + "] ";
            logMsg += "-> Tente de lancer une magie (Non implémenté dans le moteur de combat).";
        }
        
        combatLog->append(logMsg);
    };

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
                    executeSingleAction(first, second, action, i);
                }
            }
        }
        if (second->isDead() || second->physicalReserve <= 0 || first->isDead() || first->physicalReserve <= 0) break;
        
        if (i < secondQueued) {
            if (!second->isDead() && second->physicalReserve > 0) {
                const auto& action = secondActions->at(i);
                if (action.type == ActionType::Parry || action.type == ActionType::Dodge) {
                    executeSingleAction(second, first, action, i);
                }
            }
        }
        if (first->isDead() || first->physicalReserve <= 0 || second->isDead() || second->physicalReserve <= 0) break;
    }

    // Phase 2 : Résolution de toutes les attaques et magies (dans l'ordre de vitesse/alternance)
    for (size_t i = 0; i < maxActions; ++i) {
        if (i < firstQueued) {
            if (!first->isDead() && first->physicalReserve > 0) {
                const auto& action = firstActions->at(i);
                if (action.type == ActionType::Attack || action.type == ActionType::Magic) {
                    executeSingleAction(first, second, action, i);
                }
            }
        }
        if (second->isDead() || second->physicalReserve <= 0 || first->isDead() || first->physicalReserve <= 0) break;
        
        if (i < secondQueued) {
            if (!second->isDead() && second->physicalReserve > 0) {
                const auto& action = secondActions->at(i);
                if (action.type == ActionType::Attack || action.type == ActionType::Magic) {
                    executeSingleAction(second, first, action, i);
                }
            }
        }
        if (first->isDead() || first->physicalReserve <= 0 || second->isDead() || second->physicalReserve <= 0) break;
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
    
    p1Actions.clear();
    p2Actions.clear();
    
    p1Finished = false;
    p2Finished = false;
    
    bool combatFinished = fighter1->isDead() || fighter2->isDead() || fighter1->physicalReserve <= 0 || fighter2->physicalReserve <= 0;
    if (combatFinished) {
        QString endMsg = "\n======================================";
        endMsg += "\n           FIN DU COMBAT";
        endMsg += "\n======================================";
        
        bool f1_out = fighter1->isDead() || fighter1->physicalReserve <= 0;
        bool f2_out = fighter2->isDead() || fighter2->physicalReserve <= 0;
        
        if (f1_out && f2_out) {
            endMsg += "\nMatch nul ! Les deux combattants sont hors de combat.";
        } else if (f1_out) {
            QString reason = fighter1->isDead() ? "mort" : "épuisement";
            endMsg += "\n" + QString::fromStdString(fighter1->getName()) + " est hors de combat (" + reason + ").";
            endMsg += "\nVictoire de " + QString::fromStdString(fighter2->getName()) + " !";
        } else {
            QString reason = fighter2->isDead() ? "mort" : "épuisement";
            endMsg += "\n" + QString::fromStdString(fighter2->getName()) + " est hors de combat (" + reason + ").";
            endMsg += "\nVictoire de " + QString::fromStdString(fighter1->getName()) + " !";
        }
        endMsg += "\n======================================\n";
        combatLog->append(endMsg);
        
        saveCombatLog();
    } else {
        currentTurn++;
        combatLog->append("--- Préparation du Tour " + QString::number(currentTurn) + " ---\n");
    }
    
    updateCombatUI();

    if (!combatFinished) {
        QTimer::singleShot(200, this, [this]() {
            checkResolve();
        });
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

QJsonObject RunPage::serializeEntity(const Entity& entity, const std::vector<QueuedAction>& queuedActions, int freeActions) {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(entity.getName());
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
    
    QJsonArray actionsArr;
    for (const auto& act : queuedActions) {
        if (act.type == ActionType::Attack) actionsArr.append("Attaquer");
        else if (act.type == ActionType::Parry) actionsArr.append("Parer");
        else if (act.type == ActionType::Dodge) actionsArr.append("Esquiver");
        else if (act.type == ActionType::Magic) actionsArr.append("Magie");
    }
    obj["queued_actions"] = actionsArr;
    obj["free_actions"] = freeActions;
    return obj;
}

QJsonObject RunPage::serializeState(const Entity& active, const std::vector<QueuedAction>& activeActions,
                                     const Entity& opponent, const std::vector<QueuedAction>& opponentActions) {
    QJsonObject state;
    
    int activeFree = (&active == &(*fighter1)) ? p1FreeActions : p2FreeActions;
    int oppFree = (&opponent == &(*fighter1)) ? p1FreeActions : p2FreeActions;

    state["active_character"] = serializeEntity(active, activeActions, activeFree);
    state["opponent_character"] = serializeEntity(opponent, opponentActions, oppFree);
    return state;
}

QString RunPage::queryScript(const QJsonObject& state, int playerNum) {
    QString scriptPath = (playerNum == 1) ? scriptPathEdit1->text() : scriptPathEdit2->text();
    if (scriptPath.isEmpty()) {
        scriptPath = "scripts/ai_agent.py";
    }
    
    QJsonDocument doc(state);
    QString jsonStr(doc.toJson(QJsonDocument::Compact));
    
    QProcess process;
    QStringList arguments;
    arguments << scriptPath << jsonStr;
    
    process.start("python3", arguments);
    if (!process.waitForFinished(5000)) { // 5 seconds timeout for python script
        process.kill();
        combatLog->append("❌ [Script] Timeout lors de l'exécution du script Python.");
        return "Passer";
    }
    
    QByteArray output = process.readAllStandardOutput().trimmed();
    QByteArray errOutput = process.readAllStandardError().trimmed();
    
    if (process.exitCode() != 0) {
        combatLog->append("❌ [Script] Le script a échoué avec le code " + QString::number(process.exitCode()) + " : " + QString::fromUtf8(errOutput));
        return "Passer";
    }
    
    QJsonParseError parseError;
    QJsonDocument respDoc = QJsonDocument::fromJson(output, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        combatLog->append("❌ [Script] Erreur de parsing JSON de la réponse : " + parseError.errorString() + "\nStdout: " + QString::fromUtf8(output));
        return "Passer";
    }
    
    QJsonObject respObj = respDoc.object();
    return respObj["action"].toString("Passer");
}

QString RunPage::queryTCP(const QJsonObject& state, int playerNum) {
    QString host = (playerNum == 1) ? tcpHostEdit1->text() : tcpHostEdit2->text();
    int port = (playerNum == 1) ? tcpPortEdit1->value() : tcpPortEdit2->value();
    if (host.isEmpty()) host = "127.0.0.1";
    
    QTcpSocket socket;
    socket.connectToHost(host, port);
    if (!socket.waitForConnected(3000)) { // 3 seconds timeout to connect
        combatLog->append("❌ [TCP] Impossible de se connecter au serveur " + host + ":" + QString::number(port));
        return "Passer";
    }
    
    QJsonDocument doc(state);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";
    socket.write(data);
    if (!socket.waitForBytesWritten(2000)) {
        combatLog->append("❌ [TCP] Erreur d'écriture sur la socket.");
        return "Passer";
    }
    
    // Wait for response (up to 10 seconds total)
    QByteArray responseData;
    int remainingTimeMs = 10000;
    QElapsedTimer timer;
    timer.start();
    
    while (remainingTimeMs > 0) {
        if (socket.waitForReadyRead(remainingTimeMs)) {
            responseData += socket.readAll();
            if (responseData.contains('\n')) {
                break; // Received complete line
            }
        } else {
            break; // Timeout or error
        }
        remainingTimeMs = 10000 - timer.elapsed();
    }
    
    if (responseData.isEmpty()) {
        combatLog->append("❌ [TCP] Timeout de 10 secondes dépassé ou aucune réponse reçue.");
        return "Passer";
    }
    
    // Clean and parse response
    QByteArray line = responseData.split('\n').first().trimmed();
    QJsonParseError parseError;
    QJsonDocument respDoc = QJsonDocument::fromJson(line, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        combatLog->append("❌ [TCP] Erreur de parsing JSON du serveur : " + parseError.errorString() + "\nBrut: " + QString::fromUtf8(line));
        return "Passer";
    }
    
    QJsonObject respObj = respDoc.object();
    return respObj["action"].toString("Passer");
}

void RunPage::fetchAutomatedActions(Entity& entity, std::vector<QueuedAction>& actions, int freeActions, ControlMode mode, int playerNum) {
    if (mode == ControlMode::Manual) return;
    
    actions.clear();
    
    // Get reference to opponent
    Entity& opponent = (&entity == &(*fighter1)) ? (*fighter2) : (*fighter1);
    const std::vector<QueuedAction>& opponentActions = (&entity == &(*fighter1)) ? p2Actions : p1Actions;
    
    float multipliers[] = {2.0f, 2.3f, 2.6f, 3.4f, 5.0f, 7.0f};
    int maxTries = 10; // Prevent infinite loops
    
    for (int i = 0; i < maxTries; ++i) {
        if (entity.isDead() || entity.physicalReserve <= 0) break;
        
        QJsonObject state = serializeState(entity, actions, opponent, opponentActions);
        
        QString actionStr;
        if (mode == ControlMode::Script) {
            actionStr = queryScript(state, playerNum);
        } else if (mode == ControlMode::TCP) {
            actionStr = queryTCP(state, playerNum);
        }
        
        actionStr = actionStr.trimmed();
        if (actionStr == "Passer" || actionStr == "Finir le tour" || actionStr.isEmpty()) {
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
            break; // Unrecognized action
        }
        
        float multiplier = 1.0f;
        int queued = actions.size();
        if (queued >= freeActions) {
            int idx = queued - freeActions;
            if (idx > 5) idx = 5;
            multiplier = multipliers[idx];
        }
        
        actions.push_back({type, multiplier});
    }
}

void RunPage::checkResolve() {
    bool p1FinishedActual = p1Finished || (char1ModeCombo->currentIndex() != 0);
    bool p2FinishedActual = p2Finished || (char2ModeCombo->currentIndex() != 0);
    if (p1FinishedActual && p2FinishedActual) {
        resolveTurn();
    }
}
