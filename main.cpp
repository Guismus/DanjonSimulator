#include <print>
#include <filesystem>
#include <thread>
#include "back/core/Simulator.hpp"

int main(int argc, char* argv[]) {
    std::println("--- Danjon Simulator v1.0.0 (C++26) ---");

    Simulator* sim = new Simulator();
    while (true) {
        std::println("Running main loop...");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    delete sim;
    return 0;
}