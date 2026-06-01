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
#include "../../back/core/simulator.hpp"

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
    QComboBox* char1WeaponCombo;
    QComboBox* char2WeaponCombo;
    QComboBox* char1ArmorCombo;
    QComboBox* char2ArmorCombo;
    QLineEdit* scriptPathEdit1;
    QLineEdit* scriptPathEdit2;
    QLineEdit* tcpHostEdit1;
    QLineEdit* tcpHostEdit2;
    QSpinBox* tcpPortEdit1;
    QSpinBox* tcpPortEdit2;
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

    Simulator simulator;

    void loadEntities();
    void startCombat();
    void updateCombatUI();
    void resolveTurn();
    void saveCombatLog();
    void checkResolve();

    QString queryScript(const QJsonObject& state, int playerNum);
    QString queryTCP(const QJsonObject& state, int playerNum);
};
