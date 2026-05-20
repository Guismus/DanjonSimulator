#include "../include/MainWindow.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Danjon Simulator");
    setupUi();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // Error banner (hidden by default)
    errorBanner = new QLabel(this);
    errorBanner->setStyleSheet("background-color: red; color: white; font-weight: bold; padding: 10px;");
    errorBanner->setVisible(false);
    mainLayout->addWidget(errorBanner);

    // Navigation
    QHBoxLayout* navLayout = new QHBoxLayout();
    QPushButton* btnRun = new QPushButton("Run Page");
    QPushButton* btnHistory = new QPushButton("History Page");
    navLayout->addWidget(btnRun);
    navLayout->addWidget(btnHistory);
    mainLayout->addLayout(navLayout);

    // Stacked widget for pages
    stackedWidget = new QStackedWidget();
    runPage = new RunPage();
    historyPage = new HistoryPage();
    
    stackedWidget->addWidget(runPage);
    stackedWidget->addWidget(historyPage);
    
    mainLayout->addWidget(stackedWidget);

    // Connections
    connect(btnRun, &QPushButton::clicked, [this]() { stackedWidget->setCurrentWidget(runPage); });
    connect(btnHistory, &QPushButton::clicked, [this]() { stackedWidget->setCurrentWidget(historyPage); });
}

void MainWindow::showErrorBanner(const QString& message) {
    errorBanner->setText("JSON Error: " + message);
    errorBanner->setVisible(true);
}

void MainWindow::hideErrorBanner() {
    errorBanner->setVisible(false);
}
