#include <QApplication>
#include "front/include/MainWindow.hpp"
#include "back/data/DataStore.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    std::cout << "--- Danjon Simulator v1.0.0 (C++26) ---" << std::endl;

    DataStore::getInstance().loadSystemData("data/diff_stats.json");
    DataStore::getInstance().loadEntities("data/entities");

    MainWindow window;
    window.resize(1024, 768);
    window.show();

    return app.exec();
}