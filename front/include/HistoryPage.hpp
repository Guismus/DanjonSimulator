#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QListWidget>

class HistoryPage : public QWidget {
    Q_OBJECT

public:
    explicit HistoryPage(QWidget *parent = nullptr);

private:
    QListWidget* combatList;
    QTextEdit* logView;
    QPushButton* refreshButton;

    void loadLogs();
    void onCombatSelected();
};
