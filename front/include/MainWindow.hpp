#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>
#include "RunPage.hpp"
#include "HistoryPage.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void showErrorBanner(const QString& message);
    void hideErrorBanner();

private:
    QStackedWidget* stackedWidget;
    RunPage* runPage;
    HistoryPage* historyPage;
    
    QLabel* errorBanner;

    void setupUi();
};
