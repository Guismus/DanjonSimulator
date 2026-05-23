#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <optional>
#include "../../back/core/entity.hpp"

enum class ActionType {
    Attack,
    Parry,
    Dodge
};

struct QueuedAction {
    ActionType type;
    float overclockMultiplier = 1.0f;
};

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
    QPushButton* p1ParryBtn;
    QPushButton* p1DodgeBtn;
    QPushButton* p1CancelBtn;
    QPushButton* p2AttackBtn;
    QPushButton* p2ParryBtn;
    QPushButton* p2DodgeBtn;
    QPushButton* p2CancelBtn;
    QPushButton* endTurnBtn;
    QTextEdit* combatLog;

    std::optional<Entity> fighter1;
    std::optional<Entity> fighter2;
    
    std::vector<QueuedAction> p1Actions;
    std::vector<QueuedAction> p2Actions;
    int p1FreeActions;
    int p2FreeActions;
    int currentTurn = 1;

    void loadEntities();
    void startCombat();
    void updateCombatUI();
    void resolveTurn();
    void saveCombatLog();
};
