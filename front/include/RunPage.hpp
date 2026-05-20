#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <optional>
#include "../../back/core/entity.hpp"

class RunPage : public QWidget {
    Q_OBJECT

public:
    explicit RunPage(QWidget *parent = nullptr);

signals:
    void runRequested(const QString& char1, const QString& char2);

private:
    // Selection UI
    QWidget* selectionWidget;
    QComboBox* char1Combo;
    QComboBox* char2Combo;
    QPushButton* runButton;
    
    // Combat UI
    QWidget* combatWidget;
    QLabel* p1NameLabel;
    QLabel* p1HpLabel;
    QLabel* p2NameLabel;
    QLabel* p2HpLabel;
    QPushButton* p1AttackBtn;
    QPushButton* p2AttackBtn;
    QTextEdit* combatLog;

    std::optional<Entity> fighter1;
    std::optional<Entity> fighter2;
    
    bool p1Ready;
    bool p2Ready;

    void loadEntities();
    void startCombat();
    void updateCombatUI();
    void resolveTurn();
    void saveCombatLog();
};
