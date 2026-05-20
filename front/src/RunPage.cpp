#include "../include/RunPage.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include "../../back/data/DataStore.hpp"
#include "../../back/systems/CombatSystem.hpp"

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
    p1AttackBtn = new QPushButton("Attaquer");
    f1Layout->addWidget(p1NameLabel);
    f1Layout->addWidget(p1HpLabel);
    f1Layout->addWidget(p1AttackBtn);
    
    // Fighter 2 UI
    QVBoxLayout* f2Layout = new QVBoxLayout();
    p2NameLabel = new QLabel("P2");
    p2NameLabel->setStyleSheet("font-weight: bold; font-size: 16px;");
    p2HpLabel = new QLabel("HP: ");
    p2AttackBtn = new QPushButton("Attaquer");
    f2Layout->addWidget(p2NameLabel);
    f2Layout->addWidget(p2HpLabel);
    f2Layout->addWidget(p2AttackBtn);
    
    fightersLayout->addLayout(f1Layout);
    fightersLayout->addStretch();
    fightersLayout->addWidget(new QLabel("VS"));
    fightersLayout->addStretch();
    fightersLayout->addLayout(f2Layout);
    
    combatLayout->addLayout(fightersLayout);
    
    combatLog = new QLabel("Combat Started!");
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
        if (fighter1 && fighter2) performAttack(*fighter1, *fighter2);
    });
    connect(p2AttackBtn, &QPushButton::clicked, [this]() {
        if (fighter1 && fighter2) performAttack(*fighter2, *fighter1);
    });

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
    if (!opt1) { fighter1->force = 10.0f; fighter1->blood = 0.0f; fighter1->stade = 1; }
    if (!opt2) { fighter2->force = 8.0f; fighter2->blood = 0.0f; fighter2->stade = 1; }
    
    // Reset damage (blood acts as 0 to 5 damage track)
    fighter1->blood = 0.0f;
    fighter2->blood = 0.0f;
    
    // Set first turn to P1
    p1AttackBtn->setEnabled(true);
    p2AttackBtn->setEnabled(false);

    selectionWidget->setVisible(false);
    combatWidget->setVisible(true);
    combatLog->setText("Le combat commence ! Tour de " + QString::fromStdString(fighter1->getName()));
    
    updateCombatUI();
}

void RunPage::updateCombatUI() {
    if (fighter1 && fighter2) {
        p1NameLabel->setText(QString::fromStdString(fighter1->getName()));
        p1HpLabel->setText("Blessures: " + QString::number(fighter1->blood) + " / 5");
        
        p2NameLabel->setText(QString::fromStdString(fighter2->getName()));
        p2HpLabel->setText("Blessures: " + QString::number(fighter2->blood) + " / 5");
        
        // If someone is dead, disable both buttons
        if (fighter1->blood >= 5.0f || fighter2->blood >= 5.0f) {
            p1AttackBtn->setEnabled(false);
            p2AttackBtn->setEnabled(false);
        }
    }
}

void RunPage::performAttack(Entity& attacker, Entity& defender) {
    float oldBlood = defender.blood;
    CombatSystem::executeAttack(attacker, defender);
    float damage = defender.blood - oldBlood;
    
    QString msg = QString::fromStdString(attacker.getName()) + " attaque " + QString::fromStdString(defender.getName()) + " !\n" 
                + "Résultat : +" + QString::number(damage) + " Dégâts !";
                
    if (defender.blood >= 5.0f) {
        msg += "\n" + QString::fromStdString(defender.getName()) + " est K.O !";
    } else {
        // Toggle turns
        bool isP1Attacking = (attacker.getName() == fighter1->getName());
        p1AttackBtn->setEnabled(!isP1Attacking);
        p2AttackBtn->setEnabled(isP1Attacking);
        msg += "\nAu tour de " + QString::fromStdString(defender.getName()) + " de jouer.";
    }
    
    combatLog->setText(msg);
    updateCombatUI();
}
