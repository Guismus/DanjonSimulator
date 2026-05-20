#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>

class HistoryPage : public QWidget {
    Q_OBJECT

public:
    explicit HistoryPage(QWidget *parent = nullptr);

private:
    QTextEdit* logView;
    QPushButton* refreshButton;

    void loadLogs();
};
