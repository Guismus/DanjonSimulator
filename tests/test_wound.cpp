#include "../back/core/entity.hpp"
#include "../back/systems/CombatSystem.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

void test_wound_stacking() {
    Entity fighter("Test Fighter");
    fighter.blood = 32.0f;
    fighter.physicalReserve = 100.0f;

    // Apply first wound
    fighter.applyWound(1, DamageType::Tranchant);
    assert(fighter.wounds.size() == 1);
    assert(fighter.wounds[0].effectiveness == 1);

    // Apply second identical wound, they should combine: 2 Stade 1 -> Stade 2
    fighter.applyWound(1, DamageType::Tranchant);
    assert(fighter.wounds.size() == 1);
    assert(fighter.wounds[0].effectiveness == 2);

    // Apply another Stade 2 wound, they should combine: Stade 2 + Stade 2 -> Stade 3
    fighter.applyWound(2, DamageType::Tranchant);
    assert(fighter.wounds.size() == 1);
    assert(fighter.wounds[0].effectiveness == 3);

    // Apply a Stade 1 wound, should not combine, we should have Stade 3 and Stade 1 sorted descending
    fighter.applyWound(1, DamageType::Tranchant);
    assert(fighter.wounds.size() == 2);
    assert(fighter.wounds[0].effectiveness == 3);
    assert(fighter.wounds[1].effectiveness == 1);
}

void test_death_rules() {
    // 1. Base case: healthy
    Entity fighter("Test Fighter");
    fighter.blood = 32.0f;
    fighter.physicalReserve = 100.0f;
    assert(!fighter.isDead());

    // 2. Stamina <= 0 is unconscious/KO, NOT dead
    fighter.physicalReserve = 0.0f;
    assert(!fighter.isDead());
    fighter.physicalReserve = 100.0f;

    // 3. Stade 5 wound -> dead
    fighter.applyWound(5, DamageType::Neutre);
    assert(fighter.isDead());

    // Reset wounds
    fighter.wounds.clear();
    assert(!fighter.isDead());

    // 4. Blood <= 0 -> dead
    fighter.blood = 0.0f;
    assert(fighter.isDead());
    fighter.blood = 32.0f;
    assert(!fighter.isDead());

    // 5. Combined rule: Stade 4 (Surpuissance) + Stade 2 (Avantage) -> dead
    fighter.applyWound(4, DamageType::Neutre);
    fighter.applyWound(2, DamageType::Neutre);
    assert(fighter.isDead());

    // Reset wounds
    fighter.wounds.clear();

    // 6. Combined rule: Stade 4 (Surpuissance) + Stade 0 (Neutre) -> dead
    fighter.applyWound(4, DamageType::Neutre);
    fighter.applyWound(0, DamageType::Neutre);
    assert(fighter.isDead());

    // Reset wounds
    fighter.wounds.clear();

    // 7. Combined rule: Stade 4 (Surpuissance) + Stade -1 (Sous-Faveur) -> NOT dead
    fighter.applyWound(4, DamageType::Neutre);
    fighter.applyWound(-1, DamageType::Neutre);
    assert(!fighter.isDead());
}

void test_bleeding_rates() {
    Entity fighter("Test Fighter");
    
    // Contondant wounds do not bleed
    fighter.applyWound(2, DamageType::Contondant);
    assert(fighter.getBleedingRate() == 0);
    assert(fighter.getBleedingState() == "Aucun");

    fighter.wounds.clear();

    // Tranchant wounds bleed:
    // Stade <= 0: rate = 1 (Faible)
    fighter.applyWound(0, DamageType::Tranchant);
    assert(fighter.getBleedingRate() == 1);
    assert(fighter.getBleedingState() == "Bénin");

    // Stade 1 or 2: rate = 2 (Moyen)
    fighter.applyWound(2, DamageType::Tranchant); // Note: this does not combine with Stade 0
    assert(fighter.getBleedingRate() == 2); // max rate is 2
    assert(fighter.getBleedingState() == "Violent");

    // Stade >= 3: rate = 3 (Grave)
    fighter.applyWound(3, DamageType::Tranchant);
    assert(fighter.getBleedingRate() == 3);
    assert(fighter.getBleedingState() == "Grave");

    // Test applyBleeding severity reduction
    fighter.blood = 32.0f;
    fighter.applyBleeding(5);
    assert(std::abs(fighter.blood - 27.0f) < 0.001f);
}
