#include "../include/HistoryPage.hpp"
#include <QVBoxLayout>
#include <QSplitter>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QListWidgetItem>
#include <QVariant>

HistoryPage::HistoryPage(QWidget *parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    
    combatList = new QListWidget();
    splitter->addWidget(combatList);
    
    logView = new QTextEdit();
    logView->setReadOnly(true);
    splitter->addWidget(logView);
    
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    
    layout->addWidget(splitter);
    
    refreshButton = new QPushButton("Rafraîchir les logs");
    layout->addWidget(refreshButton);
    
    connect(refreshButton, &QPushButton::clicked, this, &HistoryPage::loadLogs);
    connect(combatList, &QListWidget::itemSelectionChanged, this, &HistoryPage::onCombatSelected);
    
    loadLogs();
}

void HistoryPage::loadLogs() {
    combatList->clear();
    logView->clear();
    QDir logDir("logs");
    if (!logDir.exists()) return;
    
    QStringList filters;
    filters << "*.log" << "*.json";
    logDir.setNameFilters(filters);
    
    for (const auto& fileInfo : logDir.entryInfoList(QDir::Files, QDir::Time)) {
        QListWidgetItem* item = new QListWidgetItem(fileInfo.fileName());
        item->setData(Qt::UserRole, fileInfo.absoluteFilePath());
        combatList->addItem(item);
    }
}

void HistoryPage::onCombatSelected() {
    logView->clear();
    QListWidgetItem* item = combatList->currentItem();
    if (!item) return;
    
    QString filePath = item->data(Qt::UserRole).toString();
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        logView->setText(in.readAll());
    }
}
