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
        p1Ready = true;
        p1AttackBtn->setEnabled(false);
        p1AttackBtn->setText("Prêt !");
        if (p1Ready && p2Ready) resolveTurn();
    });
    connect(p2AttackBtn, &QPushButton::clicked, [this]() {
        p2Ready = true;
        p2AttackBtn->setEnabled(false);
        p2AttackBtn->setText("Prêt !");
        if (p1Ready && p2Ready) resolveTurn();
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
    
    // Reset damage (wounds vector)
    fighter1->wounds.clear();
    fighter2->wounds.clear();
    
    p1Ready = false;
    p2Ready = false;
    
    // Set both buttons ready
    p1AttackBtn->setEnabled(true);
    p2AttackBtn->setEnabled(true);
    p1AttackBtn->setText("Attaquer");
    p2AttackBtn->setText("Attaquer");

    selectionWidget->setVisible(false);
    combatWidget->setVisible(true);
    combatLog->clear();
    combatLog->append("Le combat commence");
    
    updateCombatUI();
}

void RunPage::updateCombatUI() {
    if (fighter1 && fighter2) {
        auto formatWounds = [](const std::vector<int>& w) {
            if (w.empty()) return QString("Aucune");
            QString res = "[";
            for (size_t i = 0; i < w.size(); ++i) {
                res += getStageName(w[i]);
                if (i < w.size() - 1) res += ", ";
            }
            res += "]";
            return res;
        };
        
        p1NameLabel->setText(QString::fromStdString(fighter1->getName()));
        if (fighter1) {
            p1HpLabel->setText("Blessures : " + formatWounds(fighter1->wounds) + "\n" +
                               "Endurance : " + QString::number(fighter1->physicalReserve) + " / " + QString::number(fighter1->maxPhysicalReserve) + 
                               " (" + QString::fromStdString(fighter1->getPhysicalState()) + ")");
        }
        
        p2NameLabel->setText(QString::fromStdString(fighter2->getName()));
        if (fighter2) {
            p2HpLabel->setText("Blessures : " + formatWounds(fighter2->wounds) + "\n" +
                               "Endurance : " + QString::number(fighter2->physicalReserve) + " / " + QString::number(fighter2->maxPhysicalReserve) + 
                               " (" + QString::fromStdString(fighter2->getPhysicalState()) + ")");
        }
        
        // If someone is dead, disable both buttons
        if (fighter1->isDead() || fighter2->isDead()) {
            p1AttackBtn->setEnabled(false);
            p2AttackBtn->setEnabled(false);
        }
    }
}

void RunPage::resolveTurn() {
    if (!fighter1 || !fighter2) return;

    QString msg = "--- Résolution du Tour ---\n";

    Entity* first = &(*fighter1);
    Entity* second = &(*fighter2);

    if (fighter2->vitesse > fighter1->vitesse) {
        first = &(*fighter2);
        second = &(*fighter1);
    }

    // First attacker hits
    int eff1 = CombatSystem::executeAttack(*first, *second);
    msg += QString::fromStdString(first->getName()) + " tape (Vit: " + QString::number(first->vitesse) + ") et inflige un " + getStageName(eff1);

    if (second->isDead()) {
        msg += "\n" + QString::fromStdString(second->getName()) + " est K.O !";
    } else {
        // Second attacker hits back
        int eff2 = CombatSystem::executeAttack(*second, *first);
        msg += "\n" + QString::fromStdString(second->getName()) + " tape (Vit: " + QString::number(second->vitesse) + ") et inflige un " + getStageName(eff2);

        if (first->isDead()) {
            msg += "\n" + QString::fromStdString(first->getName()) + " est K.O !";
        }
    }

    combatLog->append(msg + "\n");

    // Reset state
    p1Ready = false;
    p2Ready = false;
    p1AttackBtn->setText("Attaquer");
    p2AttackBtn->setText("Attaquer");
    p1AttackBtn->setEnabled(true);
    p2AttackBtn->setEnabled(true);
    
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
