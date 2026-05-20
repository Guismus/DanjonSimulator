#include "../include/HistoryPage.hpp"
#include <QVBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QDir>

HistoryPage::HistoryPage(QWidget *parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    logView = new QTextEdit();
    logView->setReadOnly(true);
    layout->addWidget(logView);
    
    refreshButton = new QPushButton("Refresh Logs");
    layout->addWidget(refreshButton);
    
    connect(refreshButton, &QPushButton::clicked, this, &HistoryPage::loadLogs);
    
    loadLogs();
}

void HistoryPage::loadLogs() {
    logView->clear();
    QDir logDir("logs");
    if (!logDir.exists()) return;
    
    QStringList filters;
    filters << "*.log" << "*.json";
    logDir.setNameFilters(filters);
    
    for (const auto& fileInfo : logDir.entryInfoList(QDir::Files)) {
        QFile file(fileInfo.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            logView->append("--- " + fileInfo.fileName() + " ---");
            logView->append(in.readAll());
            logView->append("\n");
        }
    }
}
