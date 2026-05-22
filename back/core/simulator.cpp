#include "simulator.hpp"
#include <print>

Simulator::Simulator() {
    std::println("Simulator initialized. Status: IDLE");
}

Simulator::~Simulator() {
    std::println("Simulator destroyed.");
}