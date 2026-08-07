#include "Player.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "GameTables.h"
#include "Unit.h"

class Attune : public PlayerScript
{
public:
    Attune() : PlayerScript("Attune") {}

    void OnLogin(Player* player, bool firstLogin) override
    {
        QueryResult result = CharacterDatabase.Query(
            "SELECT itemEntry, forgeLevel, bonusListIDs FROM custom_attuned_items WHERE accountId = 1 AND complete = 1"
        );

        if (result)
        {
            do
            {
                Field* fields = result->Fetch();

                uint32 itemEntry = fields[0].GetUInt32();
                uint8 forgeLevel = fields[1].GetUInt8();
                std::string bonusListIDs = fields[2].GetString();

                TC_LOG_INFO("server.loading", "Attune item: %u forge: %u", itemEntry, uint32(forgeLevel));

                ApplyAttuneStats(player, itemEntry, forgeLevel);

            } while (result->NextRow());
        }
    } 

    void ApplyAttuneStats(Player* player, uint32 item, uint8 forgeLevel)
    {
        if (!player)
            return;

        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(item);

        if (!itemTemplate)
        {
            TC_LOG_INFO("server.loading", "No Attune ItemTemplate found for item entry %u", item);
            return;
        }

        bool apply = true;
        uint32 itemLevel = itemTemplate->GetBaseItemLevel();
        float combatRatingMultiplier = 1.0f;
        if (GtCombatRatingsMultByILvl const* ratingMult = sCombatRatingsMultByILvlGameTable.GetRow(itemLevel))
            combatRatingMultiplier = GetIlvlStatMultiplier(ratingMult, itemTemplate->GetInventoryType());

        for (uint8 i = 0; i < MAX_ITEM_PROTO_STATS; ++i)
        {
            int32 statType = itemTemplate->GetItemStatType(i);
            if (statType == -1)
                continue;

            int32 val = itemTemplate->GetItemStatAllocation(i);
            if (val == 0)
                continue;

            switch (statType)
            {
            case ITEM_MOD_MANA:
                player->HandleStatModifier(UNIT_MOD_MANA, BASE_VALUE, float(val), apply);
                break;
            case ITEM_MOD_HEALTH:                           // modify HP
                player->HandleStatModifier(UNIT_MOD_HEALTH, BASE_VALUE, float(val), apply);
                break;
            case ITEM_MOD_AGILITY:                          // modify agility
                player->HandleStatModifier(UNIT_MOD_STAT_AGILITY, BASE_VALUE, float(val), apply);
                player->ApplyStatBuffMod(STAT_AGILITY, CalculatePct(val, player->GetModifierValue(UNIT_MOD_STAT_AGILITY, BASE_PCT_EXCLUDE_CREATE)), apply);
                break;
            case ITEM_MOD_STRENGTH:                         //modify strength
                player->HandleStatModifier(UNIT_MOD_STAT_STRENGTH, BASE_VALUE, float(val), apply);
                player->ApplyStatBuffMod(STAT_STRENGTH, CalculatePct(val, player->GetModifierValue(UNIT_MOD_STAT_STRENGTH, BASE_PCT_EXCLUDE_CREATE)), apply);
                break;
            case ITEM_MOD_INTELLECT:                        //modify intellect
                player->HandleStatModifier(UNIT_MOD_STAT_INTELLECT, BASE_VALUE, float(val), apply);
                player->ApplyStatBuffMod(STAT_INTELLECT, CalculatePct(val, player->GetModifierValue(UNIT_MOD_STAT_INTELLECT, BASE_PCT_EXCLUDE_CREATE)), apply);
                break;
                // case ITEM_MOD_SPIRIT:                           //modify spirit
                //     player->HandleStatModifier(UNIT_MOD_STAT_SPIRIT, BASE_VALUE, float(val), apply);
                //     player->ApplyStatBuffMod(STAT_SPIRIT, CalculatePct(val, player->GetModifierValue(UNIT_MOD_STAT_SPIRIT, BASE_PCT_EXCLUDE_CREATE)), apply);
                //     break;
            case ITEM_MOD_STAMINA:                          //modify stamina
            {
                if (GtStaminaMultByILvl const* staminaMult = sStaminaMultByILvlGameTable.GetRow(itemLevel))
                    val = int32(val * GetIlvlStatMultiplier(staminaMult, itemTemplate->GetInventoryType()));
                player->HandleStatModifier(UNIT_MOD_STAT_STAMINA, BASE_VALUE, float(val), apply);
                player->ApplyStatBuffMod(STAT_STAMINA, CalculatePct(val, player->GetModifierValue(UNIT_MOD_STAT_STAMINA, BASE_PCT_EXCLUDE_CREATE)), apply);
                break;
            }
            case ITEM_MOD_DEFENSE_SKILL_RATING:
                player->ApplyRatingMod(CR_DEFENSE_SKILL, int32(val * combatRatingMultiplier), apply);
                break;
            case ITEM_MOD_DODGE_RATING:
                player->ApplyRatingMod(CR_DODGE, int32(val * combatRatingMultiplier), apply);
                break;
            case ITEM_MOD_PARRY_RATING:
                player->ApplyRatingMod(CR_PARRY, int32(val * combatRatingMultiplier), apply);
                break;
            case ITEM_MOD_BLOCK_RATING:
                player->ApplyRatingMod(CR_BLOCK, int32(val * combatRatingMultiplier), apply);
                break;
            case ITEM_MOD_HIT_MELEE_RATING:
                player->ApplyRatingMod(CR_HIT_MELEE, int32(val * combatRatingMultiplier), apply);
                break;
            case ITEM_MOD_HIT_RANGED_RATING:
                player->ApplyRatingMod(CR_HIT_RANGED, int32(val * combatRatingMultiplier), apply);
                break;
            case ITEM_MOD_HIT_SPELL_RATING:
                player->ApplyRatingMod(CR_HIT_SPELL, int32(val * combatRatingMultiplier), apply);
                break;
            case ITEM_MOD_CRIT_MELEE_RATING:
                player->ApplyRatingMod(CR_CRIT_MELEE, int32(val * combatRatingMultiplier), apply);
                break;
            case ITEM_MOD_CRIT_RANGED_RATING:
                player->ApplyRatingMod(CR_CRIT_RANGED, int32(val * combatRatingMultiplier), apply);
                break;
            case ITEM_MOD_CRIT_SPELL_RATING:
                player->ApplyRatingMod(CR_CRIT_SPELL, int32(val * combatRatingMultiplier), apply);
                break;
                // case ITEM_MOD_HIT_TAKEN_MELEE_RATING:
                //     player->ApplyRatingMod(CR_HIT_TAKEN_MELEE, int32(val), apply);
                //     break;
                // case ITEM_MOD_HIT_TAKEN_RANGED_RATING:
                //     player->ApplyRatingMod(CR_HIT_TAKEN_RANGED, int32(val), apply);
                //     break;
                // case ITEM_MOD_HIT_TAKEN_SPELL_RATING:
                //     player->ApplyRatingMod(CR_HIT_TAKEN_SPELL, int32(val), apply);
                //     break;
                // case ITEM_MOD_CRIT_TAKEN_MELEE_RATING:
                //     player->ApplyRatingMod(CR_CRIT_TAKEN_MELEE, int32(val), apply);
                //     break;
            case ITEM_MOD_CRIT_TAKEN_RANGED_RATING:
                player->ApplyRatingMod(CR_RESILIENCE_PLAYER_DAMAGE, int32(val), apply);
                break;
                // case ITEM_MOD_CRIT_TAKEN_SPELL_RATING:
                //     player->ApplyRatingMod(CR_CRIT_TAKEN_SPELL, int32(val), apply);
                //     break;
            case ITEM_MOD_HASTE_MELEE_RATING:
                player->ApplyRatingMod(CR_HASTE_MELEE, int32(val), apply);
                break;
            case ITEM_MOD_HASTE_RANGED_RATING:
                player->ApplyRatingMod(CR_HASTE_RANGED, int32(val), apply);
                break;
            case ITEM_MOD_HASTE_SPELL_RATING:
                player->ApplyRatingMod(CR_HASTE_SPELL, int32(val), apply);
                break;
            case ITEM_MOD_HIT_RATING:
                player->ApplyRatingMod(CR_HIT_MELEE, int32(val * combatRatingMultiplier), apply);
                player->ApplyRatingMod(CR_HIT_RANGED, int32(val * combatRatingMultiplier), apply);
                player->ApplyRatingMod(CR_HIT_SPELL, int32(val * combatRatingMultiplier), apply);
                break;
            case ITEM_MOD_CRIT_RATING:
                player->ApplyRatingMod(CR_CRIT_MELEE, int32(val * combatRatingMultiplier), apply);
                player->ApplyRatingMod(CR_CRIT_RANGED, int32(val * combatRatingMultiplier), apply);
                player->ApplyRatingMod(CR_CRIT_SPELL, int32(val * combatRatingMultiplier), apply);
                break;
                // case ITEM_MOD_HIT_TAKEN_RATING: // Unused since 3.3.5
                //     player->ApplyRatingMod(CR_HIT_TAKEN_MELEE, int32(val), apply);
                //     player->ApplyRatingMod(CR_HIT_TAKEN_RANGED, int32(val), apply);
                //     player->ApplyRatingMod(CR_HIT_TAKEN_SPELL, int32(val), apply);
                //     break;
                // case ITEM_MOD_CRIT_TAKEN_RATING: // Unused since 3.3.5
                //     player->ApplyRatingMod(CR_CRIT_TAKEN_MELEE, int32(val), apply);
                //     player->ApplyRatingMod(CR_CRIT_TAKEN_RANGED, int32(val), apply);
                //     player->ApplyRatingMod(CR_CRIT_TAKEN_SPELL, int32(val), apply);
                //     break;
            case ITEM_MOD_RESILIENCE_RATING:
                player->ApplyRatingMod(CR_RESILIENCE_PLAYER_DAMAGE, int32(val * combatRatingMultiplier), apply);
                break;
            case ITEM_MOD_HASTE_RATING:
                player->ApplyRatingMod(CR_HASTE_MELEE, int32(val * combatRatingMultiplier), apply);
                player->ApplyRatingMod(CR_HASTE_RANGED, int32(val * combatRatingMultiplier), apply);
                player->ApplyRatingMod(CR_HASTE_SPELL, int32(val * combatRatingMultiplier), apply);
                break;
            case ITEM_MOD_EXPERTISE_RATING:
                player->ApplyRatingMod(CR_EXPERTISE, int32(val * combatRatingMultiplier), apply);
                break;
            case ITEM_MOD_ATTACK_POWER:
                player->HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE, float(val), apply);
                player->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(val), apply);
                break;
            case ITEM_MOD_RANGED_ATTACK_POWER:
                player->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(val), apply);
                break;
            case ITEM_MOD_VERSATILITY:
                player->ApplyRatingMod(CR_VERSATILITY_DAMAGE_DONE, int32(val * combatRatingMultiplier), apply);
                player->ApplyRatingMod(CR_VERSATILITY_DAMAGE_TAKEN, int32(val * combatRatingMultiplier), apply);
                player->ApplyRatingMod(CR_VERSATILITY_HEALING_DONE, int32(val * combatRatingMultiplier), apply);
                break;
            case ITEM_MOD_MANA_REGENERATION:
                player->ApplyManaRegenBonus(int32(val), apply);
                break;
            case ITEM_MOD_ARMOR_PENETRATION_RATING:
                player->ApplyRatingMod(CR_ARMOR_PENETRATION, int32(val), apply);
                break;
            case ITEM_MOD_SPELL_POWER:
                player->ApplySpellPowerBonus(int32(val), apply);
                break;
            case ITEM_MOD_HEALTH_REGEN:
                player->ApplyHealthRegenBonus(int32(val), apply);
                break;
            case ITEM_MOD_SPELL_PENETRATION:
                player->ApplySpellPenetrationBonus(val, apply);
                break;
            case ITEM_MOD_MASTERY_RATING:
                player->ApplyRatingMod(CR_MASTERY, int32(val * combatRatingMultiplier), apply);
                break;
            case ITEM_MOD_EXTRA_ARMOR:
                player->HandleStatModifier(UNIT_MOD_ARMOR, TOTAL_VALUE, float(val), apply);
                break;
            case ITEM_MOD_FIRE_RESISTANCE:
                player->HandleStatModifier(UNIT_MOD_RESISTANCE_FIRE, BASE_VALUE, float(val), apply);
                break;
            case ITEM_MOD_FROST_RESISTANCE:
                player->HandleStatModifier(UNIT_MOD_RESISTANCE_FROST, BASE_VALUE, float(val), apply);
                break;
            case ITEM_MOD_HOLY_RESISTANCE:
                player->HandleStatModifier(UNIT_MOD_RESISTANCE_HOLY, BASE_VALUE, float(val), apply);
                break;
            case ITEM_MOD_SHADOW_RESISTANCE:
                player->HandleStatModifier(UNIT_MOD_RESISTANCE_SHADOW, BASE_VALUE, float(val), apply);
                break;
            case ITEM_MOD_NATURE_RESISTANCE:
                player->HandleStatModifier(UNIT_MOD_RESISTANCE_NATURE, BASE_VALUE, float(val), apply);
                break;
            case ITEM_MOD_ARCANE_RESISTANCE:
                player->HandleStatModifier(UNIT_MOD_RESISTANCE_ARCANE, BASE_VALUE, float(val), apply);
                break;
            case ITEM_MOD_PVP_POWER:
                player->ApplyRatingMod(CR_PVP_POWER, int32(val), apply);
                break;
            case ITEM_MOD_CORRUPTION:
                player->ApplyRatingMod(CR_CORRUPTION, int32(val), apply);
                break;
            case ITEM_MOD_CORRUPTION_RESISTANCE:
                player->ApplyRatingMod(CR_CORRUPTION_RESISTANCE, int32(val), apply);
                break;
            case ITEM_MOD_CR_SPEED:
                player->ApplyRatingMod(CR_SPEED, int32(val * combatRatingMultiplier), apply);
                break;
            case ITEM_MOD_CR_LIFESTEAL:
                player->ApplyRatingMod(CR_LIFESTEAL, int32(val * combatRatingMultiplier), apply);
                break;
            case ITEM_MOD_CR_AVOIDANCE:
                player->ApplyRatingMod(CR_AVOIDANCE, int32(val * combatRatingMultiplier), apply);
                break;
            case ITEM_MOD_CR_STURDINESS:
                player->ApplyRatingMod(CR_STURDINESS, int32(val * combatRatingMultiplier), apply);
                break;
            case ITEM_MOD_AGI_STR_INT:
                player->HandleStatModifier(UNIT_MOD_STAT_AGILITY, BASE_VALUE, float(val), apply);
                player->HandleStatModifier(UNIT_MOD_STAT_STRENGTH, BASE_VALUE, float(val), apply);
                player->HandleStatModifier(UNIT_MOD_STAT_INTELLECT, BASE_VALUE, float(val), apply);
                player->ApplyStatBuffMod(STAT_AGILITY, CalculatePct(val, player->GetModifierValue(UNIT_MOD_STAT_AGILITY, BASE_PCT_EXCLUDE_CREATE)), apply);
                player->ApplyStatBuffMod(STAT_STRENGTH, CalculatePct(val, player->GetModifierValue(UNIT_MOD_STAT_STRENGTH, BASE_PCT_EXCLUDE_CREATE)), apply);
                player->ApplyStatBuffMod(STAT_INTELLECT, CalculatePct(val, player->GetModifierValue(UNIT_MOD_STAT_INTELLECT, BASE_PCT_EXCLUDE_CREATE)), apply);
                break;
            case ITEM_MOD_AGI_STR:
                player->HandleStatModifier(UNIT_MOD_STAT_AGILITY, BASE_VALUE, float(val), apply);
                player->HandleStatModifier(UNIT_MOD_STAT_STRENGTH, BASE_VALUE, float(val), apply);
                player->ApplyStatBuffMod(STAT_AGILITY, CalculatePct(val, player->GetModifierValue(UNIT_MOD_STAT_AGILITY, BASE_PCT_EXCLUDE_CREATE)), apply);
                player->ApplyStatBuffMod(STAT_STRENGTH, CalculatePct(val, player->GetModifierValue(UNIT_MOD_STAT_STRENGTH, BASE_PCT_EXCLUDE_CREATE)), apply);
                break;
            case ITEM_MOD_AGI_INT:
                player->HandleStatModifier(UNIT_MOD_STAT_AGILITY, BASE_VALUE, float(val), apply);
                player->HandleStatModifier(UNIT_MOD_STAT_INTELLECT, BASE_VALUE, float(val), apply);
                player->ApplyStatBuffMod(STAT_AGILITY, CalculatePct(val, player->GetModifierValue(UNIT_MOD_STAT_AGILITY, BASE_PCT_EXCLUDE_CREATE)), apply);
                player->ApplyStatBuffMod(STAT_INTELLECT, CalculatePct(val, player->GetModifierValue(UNIT_MOD_STAT_INTELLECT, BASE_PCT_EXCLUDE_CREATE)), apply);
                break;
            case ITEM_MOD_STR_INT:
                player->HandleStatModifier(UNIT_MOD_STAT_STRENGTH, BASE_VALUE, float(val), apply);
                player->HandleStatModifier(UNIT_MOD_STAT_INTELLECT, BASE_VALUE, float(val), apply);
                player->ApplyStatBuffMod(STAT_STRENGTH, CalculatePct(val, player->GetModifierValue(UNIT_MOD_STAT_STRENGTH, BASE_PCT_EXCLUDE_CREATE)), apply);
                player->ApplyStatBuffMod(STAT_INTELLECT, CalculatePct(val, player->GetModifierValue(UNIT_MOD_STAT_INTELLECT, BASE_PCT_EXCLUDE_CREATE)), apply);
                break;
            }
        }
    }
};

void AddSC_Attune()
{
    new Attune();
}