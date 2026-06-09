#include <print>
#include <cstdio>

#define RUN_TEST(test_func) \
    do { \
        std::println("[RUN] {}...", #test_func); \
        test_func(); \
        std::println("[SUCCESS] {}", #test_func); \
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
void test_catalyst_loading();


// Simulator & overclock tests (test_simulator.cpp)
void test_simulator();
void test_overclock_simulation();

// Advanced wear and magic tests (test_magic_durability.cpp)
void test_advanced_wear_and_magic();
void test_eaux_maternelles();
void test_magic_catalyst_selection();
void test_bal_des_lucioles();
void test_generic_duration_heal_and_boost();
void test_races();
void test_strict_json_validation();


#include "../back/data/DataStore.hpp"

int main() {
    // 1. Initialiser le DataStore avec les fichiers JSON
    if (!DataStore::getInstance().loadSystemData("data/diff_stats.json")) {
        std::println(stderr, "Failed to load system data");
        return 1;
    }
    if (!DataStore::getInstance().loadEnergySystem("data/energy_system.json")) {
        std::println(stderr, "Failed to load energy system");
        return 1;
    }
    if (!DataStore::getInstance().loadCatalysts("data/catalysts.json")) {
        std::println(stderr, "Failed to load catalysts");
        return 1;
    }
    if (!DataStore::getInstance().loadSpells("data/magies")) {
        std::println(stderr, "Failed to load spells");
        return 1;
    }
    if (!DataStore::getInstance().loadRaces("data/races")) {
        std::println(stderr, "Failed to load races");
        return 1;
    }

    std::println("Starting unit tests suite...");

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
    RUN_TEST(test_catalyst_loading);


    // Simulator / Overclock tests
    RUN_TEST(test_simulator);
    RUN_TEST(test_overclock_simulation);

    // Advanced wear and magic tests
    RUN_TEST(test_advanced_wear_and_magic);
    RUN_TEST(test_eaux_maternelles);
    RUN_TEST(test_magic_catalyst_selection);
    RUN_TEST(test_bal_des_lucioles);
    RUN_TEST(test_generic_duration_heal_and_boost);
    RUN_TEST(test_races);
    RUN_TEST(test_strict_json_validation);


    std::println("All unit tests completed successfully!");
    return 0;
}
