#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QSpinBox>
#include <QJsonObject>
#include <optional>
#include "../../back/core/entity.hpp"

enum class ActionType {
    Attack,
    Parry,
    Dodge,
    Magic
};

enum class ControlMode {
    Manual,
    Script,
    TCP
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
    QComboBox* char1ModeCombo;
    QComboBox* char2ModeCombo;
    QLineEdit* scriptPathEdit;
    QLineEdit* tcpHostEdit;
    QSpinBox* tcpPortEdit;
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
    QPushButton* p1PassBtn;
    QPushButton* p2PassBtn;
    QPushButton* p1CancelBtn;
    QPushButton* p2AttackBtn;
    QPushButton* p2ParryBtn;
    QPushButton* p2DodgeBtn;
    QPushButton* p2CancelBtn;
    QTextEdit* combatLog;

    std::optional<Entity> fighter1;
    std::optional<Entity> fighter2;
    
    std::vector<QueuedAction> p1Actions;
    std::vector<QueuedAction> p2Actions;
    int p1FreeActions;
    int p2FreeActions;
    int currentTurn = 1;
    bool p1Finished = false;
    bool p2Finished = false;

    void loadEntities();
    void startCombat();
    void updateCombatUI();
    void resolveTurn();
    void saveCombatLog();
    void checkResolve();

    void fetchAutomatedActions(Entity& entity, std::vector<QueuedAction>& actions, int freeActions, ControlMode mode);
    QString queryScript(const QJsonObject& state);
    QString queryTCP(const QJsonObject& state);
    QJsonObject serializeState(const Entity& active, const std::vector<QueuedAction>& activeActions,
                               const Entity& opponent, const std::vector<QueuedAction>& opponentActions);
    QJsonObject serializeEntity(const Entity& entity, const std::vector<QueuedAction>& queuedActions);
};
