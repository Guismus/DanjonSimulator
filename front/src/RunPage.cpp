#include "../include/RunPage.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTimer>
#include "../../back/data/DataStore.hpp"
#include "../../back/bindings/AgentRunner.hpp"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>



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
        simulator.addActionP1(type);
        updateCombatUI();
    };

    auto addActionP2 = [this](ActionType type) {
        simulator.addActionP2(type);
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
        simulator.popActionP1();
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
        simulator.popActionP2();
        updateCombatUI();
    });

    connect(p1PassBtn, &QPushButton::clicked, [this]() {
        simulator.setP1Finished(true);
        updateCombatUI();
        checkResolve();
    });
    connect(p2PassBtn, &QPushButton::clicked, [this]() {
        simulator.setP2Finished(true);
        updateCombatUI();
        checkResolve();
    });

    agentRunner = new AgentRunner(this);
    agentRunner->setLogCallback([this](const std::string& msg) {
        combatLog->append(QString::fromStdString(msg));
    });

    simulator.setExternalAgentQueryCallback([this](const std::string& stateJson, int playerNum) {
        return agentRunner->query(stateJson, playerNum);
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
    
    Entity f1 = DataStore::getInstance().createFighter(n1.toStdString(),
                                                       char1WeaponCombo->currentText().toStdString(),
                                                       char1ArmorCombo->currentText().toStdString(),
                                                       10.0f);
    Entity f2 = DataStore::getInstance().createFighter(n2.toStdString(),
                                                       char2WeaponCombo->currentText().toStdString(),
                                                       char2ArmorCombo->currentText().toStdString(),
                                                       8.0f);
    
    ControlMode mode1 = static_cast<ControlMode>(char1ModeCombo->currentIndex());
    ControlMode mode2 = static_cast<ControlMode>(char2ModeCombo->currentIndex());
    
    AgentConfig config1;
    config1.mode = mode1;
    config1.scriptPath = scriptPathEdit1->text().toStdString();
    config1.tcpHost = tcpHostEdit1->text().toStdString();
    config1.tcpPort = tcpPortEdit1->value();

    AgentConfig config2;
    config2.mode = mode2;
    config2.scriptPath = scriptPathEdit2->text().toStdString();
    config2.tcpHost = tcpHostEdit2->text().toStdString();
    config2.tcpPort = tcpPortEdit2->value();

    agentRunner->configurePlayer1(config1);
    agentRunner->configurePlayer2(config2);
    
    simulator.startCombat(f1, f2, mode1, mode2);

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
    auto& f1Opt = simulator.getFighter1();
    auto& f2Opt = simulator.getFighter2();
    if (f1Opt.has_value() && f2Opt.has_value()) {
        Entity& f1 = f1Opt.value();
        Entity& f2 = f2Opt.value();
        
        auto formatWounds = [](const std::vector<Wound>& w) {
            if (w.empty()) return QString("Aucune");
            QString res = "[";
            for (size_t i = 0; i < w.size(); ++i) {
                res += QString::fromStdString(getStageName(w[i].effectiveness));
                if (w[i].damageType == DamageType::Tranchant) {
                    res += " (Tranchante)";
                } else if (w[i].damageType == DamageType::Contondant) {
                    res += " (Contondante)";
                } else if (w[i].damageType == DamageType::Feu) {
                    res += " (Feu)";
                } else {
                    res += " (Neutre)";
                }
                if (i < w.size() - 1) res += ", ";
            }
            res += "]";
            return res;
        };
        
        p1NameLabel->setText(QString::fromStdString(f1.getName()));
        
        QString armorInfo1 = "Armure : Aucune";
        if (f1.armor.has_value()) {
            const auto& armor = f1.armor.value();
            armorInfo1 = QString("Armure : %1 (Durabilité : %2/%3)")
                        .arg(QString::fromStdString(armor.name))
                        .arg(armor.durability)
                        .arg(armor.maxDurability);
            if (armor.durability <= 0) {
                armorInfo1 += " [ROMPUE]";
            }
        }
        QString weaponInfo1 = "Arme : Aucune";
        if (f1.weapon.has_value()) {
            const auto& weapon = f1.weapon.value();
            weaponInfo1 = QString("Arme : %1 (Durabilité : %2/%3)")
                         .arg(QString::fromStdString(weapon.name))
                         .arg(weapon.durability)
                         .arg(weapon.maxDurability);
            if (weapon.durability <= 0) {
                weaponInfo1 += " [ROMPUE]";
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
                else if (actions[i].type == ActionType::Magic) name = "Magie";
                
                if (actions[i].overclockMultiplier > 1.0f) {
                    name += " [Surcad. x" + QString::number(actions[i].overclockMultiplier, 'f', 1) + "]";
                } else {
                    name += " [Gratuite]";
                }
                list << QString::number(i + 1) + ". " + name;
            }
            return list.join("\n");
        };

        QString classDetails1 = "";
        if (f1.characterClass.has_value() && !f1.characterClass.value().empty()) {
            classDetails1 = "Classe : " + QString::fromStdString(f1.characterClass.value()) + "\n";
        }

        p1HpLabel->setText(classDetails1 +
                           "Blessures : " + formatWounds(f1.wounds) + "\n" +
                           "Saignement : " + QString::fromStdString(f1.getBleedingState()) + " (" + QString::number(f1.getBleedingRate()) + " tics/tour)\n" +
                           "Sang : " + QString::number(f1.blood, 'f', 1) + " / 32.0 tics\n" +
                           "Endurance : " + QString::number(f1.physicalReserve) + " / " + QString::number(f1.maxPhysicalReserve) + 
                           " (" + QString::fromStdString(f1.getPhysicalState()) + ")\n" +
                           armorInfo1 + "\n" +
                           weaponInfo1 + "\n\nActions préparées :\n" + formatActions(simulator.getP1Actions()));
        
        p2NameLabel->setText(QString::fromStdString(f2.getName()));
        
        QString armorInfo2 = "Armure : Aucune";
        if (f2.armor.has_value()) {
            const auto& armor = f2.armor.value();
            armorInfo2 = QString("Armure : %1 (Durabilité : %2/%3)")
                        .arg(QString::fromStdString(armor.name))
                        .arg(armor.durability)
                        .arg(armor.maxDurability);
            if (armor.durability <= 0) {
                armorInfo2 += " [ROMPUE]";
            }
        }
        QString weaponInfo2 = "Arme : Aucune";
        if (f2.weapon.has_value()) {
            const auto& weapon = f2.weapon.value();
            weaponInfo2 = QString("Arme : %1 (Durabilité : %2/%3)")
                         .arg(QString::fromStdString(weapon.name))
                         .arg(weapon.durability)
                         .arg(weapon.maxDurability);
            if (weapon.durability <= 0) {
                weaponInfo2 += " [ROMPUE]";
            }
        }

        QString classDetails2 = "";
        if (f2.characterClass.has_value() && !f2.characterClass.value().empty()) {
            classDetails2 = "Classe : " + QString::fromStdString(f2.characterClass.value()) + "\n";
        }

        p2HpLabel->setText(classDetails2 +
                           "Blessures : " + formatWounds(f2.wounds) + "\n" +
                           "Saignement : " + QString::fromStdString(f2.getBleedingState()) + " (" + QString::number(f2.getBleedingRate()) + " tics/tour)\n" +
                           "Sang : " + QString::number(f2.blood, 'f', 1) + " / 32.0 tics\n" +
                           "Endurance : " + QString::number(f2.physicalReserve) + " / " + QString::number(f2.maxPhysicalReserve) + 
                           " (" + QString::fromStdString(f2.getPhysicalState()) + ")\n" +
                           armorInfo2 + "\n" +
                           weaponInfo2 + "\n\nActions préparées :\n" + formatActions(simulator.getP2Actions()));
        
        if (f1.isDead() || f2.isDead() || f1.physicalReserve <= 0 || f2.physicalReserve <= 0) {
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
            p1AttackBtn->setEnabled(p1Manual && !simulator.isP1Finished());
            p1ParryBtn->setEnabled(p1Manual && !simulator.isP1Finished());
            p1DodgeBtn->setEnabled(p1Manual && !simulator.isP1Finished());
            p1PassBtn->setEnabled(p1Manual && !simulator.isP1Finished());
            p1CancelBtn->setEnabled(p1Manual && (simulator.isP1Finished() || !simulator.getP1Actions().empty()));

            bool p2Manual = (char2ModeCombo->currentIndex() == 0);
            p2AttackBtn->setEnabled(p2Manual && !simulator.isP2Finished());
            p2ParryBtn->setEnabled(p2Manual && !simulator.isP2Finished());
            p2DodgeBtn->setEnabled(p2Manual && !simulator.isP2Finished());
            p2PassBtn->setEnabled(p2Manual && !simulator.isP2Finished());
            p2CancelBtn->setEnabled(p2Manual && (simulator.isP2Finished() || !simulator.getP2Actions().empty()));
            
            auto setBtnText = [this](QPushButton* btnAttack, QPushButton* btnParry, QPushButton* btnDodge, const Entity& entity, const std::vector<QueuedAction>& actions, int freeCount) {
                float attackMult = simulator.getNextMultiplier(entity, actions, ActionType::Attack, freeCount);
                float parryMult = simulator.getNextMultiplier(entity, actions, ActionType::Parry, freeCount);
                float dodgeMult = simulator.getNextMultiplier(entity, actions, ActionType::Dodge, freeCount);
                
                auto formatSuffix = [](float mult) {
                    return mult > 1.0f ? " (Surcad. x" + QString::number(mult, 'f', 1) + ")" : " (Gratuit)";
                };
                
                btnAttack->setText("Attaquer" + formatSuffix(attackMult));
                btnParry->setText("Parer" + formatSuffix(parryMult));
                btnDodge->setText("Esquiver" + formatSuffix(dodgeMult));
            };
            setBtnText(p1AttackBtn, p1ParryBtn, p1DodgeBtn, f1, simulator.getP1Actions(), simulator.getP1FreeActions());
            setBtnText(p2AttackBtn, p2ParryBtn, p2DodgeBtn, f2, simulator.getP2Actions(), simulator.getP2FreeActions());
        }
    }
}

void RunPage::resolveTurn() {
    auto result = simulator.resolveTurn();
    
    for (const auto& logMsg : result.logs) {
        combatLog->append(QString::fromStdString(logMsg));
    }
    
    updateCombatUI();

    if (result.combatFinished) {
        saveCombatLog();
    } else {
        QTimer::singleShot(200, this, [this]() {
            checkResolve();
        });
    }
}

void RunPage::saveCombatLog() {
    auto& f1Opt = simulator.getFighter1();
    auto& f2Opt = simulator.getFighter2();
    if (!f1Opt.has_value() || !f2Opt.has_value()) return;

    QDir dir("logs");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QString filename = "logs/Combat_" + 
                       QString::fromStdString(f1Opt->getName()).replace(" ", "_") + "_vs_" + 
                       QString::fromStdString(f2Opt->getName()).replace(" ", "_") + "_" + 
                       QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".log";
                       
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << combatLog->toPlainText();
        file.close();
    }
}



void RunPage::checkResolve() {
    bool p1FinishedActual = simulator.isP1Finished() || (char1ModeCombo->currentIndex() != 0);
    bool p2FinishedActual = simulator.isP2Finished() || (char2ModeCombo->currentIndex() != 0);
    if (p1FinishedActual && p2FinishedActual) {
        resolveTurn();
    }
}
