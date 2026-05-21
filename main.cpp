#include <QApplication>
#include "front/include/MainWindow.hpp"
#include "back/data/DataStore.hpp"
#include <print>
#include <QFile>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    std::println("--- Danjon Simulator v1.0.0 (C++26) ---");

    QFile styleFile("front/resources/style.qss");
    if(styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        app.setStyleSheet(styleSheet);
    }

    DataStore::getInstance().loadSystemData("data/diff_stats.json");
    DataStore::getInstance().loadEnergySystem("data/energy_system.json");
    DataStore::getInstance().loadArmors("data/Equipement/Armure");
    DataStore::getInstance().loadEntities("data/entities");

    MainWindow window;
    window.resize(1024, 768);
    window.show();

    return app.exec();
}