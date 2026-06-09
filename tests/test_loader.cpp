#include "../back/core/entity.hpp"
#include "../back/data/DataStore.hpp"
#include <cassert>
#include <cmath>

void test_equipment_loading() {
    assert(DataStore::getInstance().loadWeapons("data/Equipement/Arme"));
    assert(DataStore::getInstance().loadArmors("data/Equipement/Armure"));

    auto weapons = DataStore::getInstance().getAvailableWeaponNames();
    assert(!weapons.empty());
    
    auto espadon = DataStore::getInstance().getWeaponTemplate("Espadon");
    assert(espadon.has_value());
    assert(espadon->name == "Espadon");
    assert(espadon->type == WeaponWeight::Lourd);
    assert(espadon->damageType == DamageType::Tranchant);
    assert(espadon->durability == 150);

    auto armors = DataStore::getInstance().getAvailableArmorNames();
    assert(!armors.empty());
    auto maille = DataStore::getInstance().getArmorTemplate("cote de mailles en parbélienne");
    assert(maille.has_value());
    assert(maille->name == "cote de mailles en parbélienne");
    assert(maille->material == ArmorMaterial::Mineral);
    assert(maille->durability == 36);
}

void test_class_immunities() {
    Entity grimm("Grimm Entity");
    grimm.setClass("Grimm");
    assert(grimm.isImmuneToPoison());
    assert(grimm.isImmuneToCharm());
    assert(!grimm.isImmuneToStun());
    assert(std::abs(grimm.getFireDamageResistanceBonus() - 0.0f) < 0.001f);

    Entity aegis("Aegis Entity");
    aegis.setClass("AEGIS");
    assert(!aegis.isImmuneToPoison());
    assert(!aegis.isImmuneToCharm());
    assert(aegis.isImmuneToStun());

    Entity forgemaster("Forgemaster Entity");
    forgemaster.setClass("FORGEMAITRE");
    assert(std::abs(forgemaster.getFireDamageResistanceBonus() - 0.10f) < 0.001f);

    Entity arachnee("Arachnee Entity");
    arachnee.setClass("ARACHNEE");
    assert(arachnee.isImmuneToPoison());
    assert(arachnee.isImmuneToCharm());
}

void test_aegis_delayed_wound_debuff() {
    Entity fighter("Aegis Fighter");
    fighter.setClass("AEGIS");
    fighter.force = 10.0f;
    fighter.currentTurn = 1;

    // Apply Stade 3 wound at turn 1
    fighter.applyWound(3, DamageType::Contondant);

    // At turn 1, debuff should be delayed (0 turns elapsed)
    assert(std::abs(fighter.getEffectiveForce() - 10.0f) < 0.001f);

    // At turn 2, debuff should still be delayed (1 turn elapsed)
    fighter.currentTurn = 2;
    assert(std::abs(fighter.getEffectiveForce() - 10.0f) < 0.001f);

    // At turn 3, debuff should apply (2 turns elapsed)
    fighter.currentTurn = 3;
    assert(std::abs(fighter.getEffectiveForce() - 8.5f) < 0.001f);
}

void test_catalyst_loading() {
    assert(DataStore::getInstance().loadCatalysts("data/catalysts.json"));
    auto temp = DataStore::getInstance().getCatalystTemplate("F");
    assert(temp.has_value());
    assert(temp->reserve == 100);
    assert(temp->power == 5);
    
    // Also load entities to check Haru
    assert(DataStore::getInstance().loadEntities("data/entities"));
    auto haruOpt = DataStore::getInstance().getEntityTemplate("Haru Dahrendorf");
    assert(haruOpt.has_value());
    assert(!haruOpt->catalysts.empty());
    assert(haruOpt->catalysts[0].magicType == "Eaux maternelles");
    assert(haruOpt->catalysts[0].reserve == 100);
    assert(haruOpt->catalysts[0].power == 5);
    assert(haruOpt->catalyst.has_value());
    assert(haruOpt->catalyst->reserve == 100);
    assert(haruOpt->catalyst->power == 5);
}
