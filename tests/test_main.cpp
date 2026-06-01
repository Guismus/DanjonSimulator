#include <iostream>

#define RUN_TEST(test_func) \
    do { \
        std::cout << "[RUN] " << #test_func << "..." << std::endl; \
        test_func(); \
        std::cout << "[SUCCESS] " << #test_func << std::endl; \
    } while (0)

// Wound tests (test_wound.cpp)
void test_wound_stacking();
void test_death_rules();
void test_bleeding_rates();

// Combat tests (test_combat.cpp)
void test_speed_free_actions();
void test_stamina_costs();
void test_parry_dodge();
void test_stat_penalties_rounding();

// Loader / static data tests (test_loader.cpp)
void test_equipment_loading();
void test_class_immunities();
void test_aegis_delayed_wound_debuff();

// Simulator & overclock tests (test_simulator.cpp)
void test_simulator();
void test_overclock_simulation();

// Advanced wear and magic tests (test_magic_durability.cpp)
void test_advanced_wear_and_magic();

#include "../back/data/DataStore.hpp"

int main() {
    // 1. Initialiser le DataStore avec les fichiers JSON
    if (!DataStore::getInstance().loadSystemData("data/diff_stats.json")) {
        std::cerr << "Failed to load system data" << std::endl;
        return 1;
    }
    if (!DataStore::getInstance().loadEnergySystem("data/energy_system.json")) {
        std::cerr << "Failed to load energy system" << std::endl;
        return 1;
    }

    std::cout << "Starting unit tests suite..." << std::endl;

    // Wound tests
    RUN_TEST(test_wound_stacking);
    RUN_TEST(test_death_rules);
    RUN_TEST(test_bleeding_rates);

    // Combat tests
    RUN_TEST(test_speed_free_actions);
    RUN_TEST(test_stamina_costs);
    RUN_TEST(test_parry_dodge);
    RUN_TEST(test_stat_penalties_rounding);

    // Loader tests
    RUN_TEST(test_equipment_loading);
    RUN_TEST(test_class_immunities);
    RUN_TEST(test_aegis_delayed_wound_debuff);

    // Simulator / Overclock tests
    RUN_TEST(test_simulator);
    RUN_TEST(test_overclock_simulation);

    // Advanced wear and magic tests
    RUN_TEST(test_advanced_wear_and_magic);

    std::cout << "All unit tests completed successfully!" << std::endl;
    return 0;
}
