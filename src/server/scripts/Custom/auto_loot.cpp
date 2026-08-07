#include "Chat.h"
#include "Creature.h"
#include "Loot.h"
#include "LootMgr.h"
#include "LootPackets.h"
#include "Item.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"

class AutoLoot : public PlayerScript
{
public:
    AutoLoot() : PlayerScript("AutoLoot") { }

    void OnCreatureKill(Player* player, Creature* creature) override
    {
        ChatHandler(player->GetSession()).PSendSysMessage("Creature Kill");

        if (!player || !creature)
            return;

        if (creature->IsAlive())
            return;

        Loot& loot = creature->loot;

        if (loot.isLooted())
            return;

        AutoLootCreature(player, creature, loot);
    }

private:
    static void AutoLootCreature(Player* player, Creature* creature, Loot& loot) 
    {
        if (loot.gold > 0)
        {
            uint64 gold = loot.gold;
            loot.gold = 0;

            uint64 goldMod = CalculatePct(gold, player->GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_MONEY_GAIN, 1));

            player->ModifyMoney(gold);
            player->UpdateCriteria(CRITERIA_TYPE_LOOT_MONEY, gold);

            WorldPackets::Loot::LootMoneyNotify packet;
            packet.Money = gold;
            packet.MoneyMod = goldMod;
            packet.SoleLooter = true; // "You loot..."
            player->GetSession()->SendPacket(packet.Write());

            loot.NotifyMoneyRemoved();
        }

        // Loot normal items.
        uint32 i = 0;

        for (auto itr = loot.items.begin(); itr != loot.items.end(); ++itr)
        {
            ObjectGuid const& ownerGuid = itr->first;
            LootItemList& itemList = itr->second;

            // Optional: skip loot lists that do not belong to this player
            if (!ownerGuid.IsEmpty() && ownerGuid != player->GetGUID())
                continue;

            for (auto itemItr = itemList.begin(); itemItr != itemList.end(); ++itemItr, ++i)
            {
                LootItem& item = *itemItr;

                if (item.is_looted || item.count == 0)
                    continue;

                if (!item.AllowedForPlayer(player))
                    continue;

                ItemTemplate const* proto = sObjectMgr->GetItemTemplate(item.itemid);
                if (!proto)
                    continue;

                ItemPosCountVec dest;
                InventoryResult msg = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, item.itemid, item.count);

                if (msg == EQUIP_ERR_OK)
                {
                    Item* newItem = player->StoreNewItem(dest, item.itemid, true, item.randomBonusListId);
                    player->SendNewItem(newItem, item.count, false, false, true);

                    item.count = 0;
                    item.is_looted = true;

                    loot.NotifyItemRemoved(i);

                    if (loot.unlootedCount > 0)
                        --loot.unlootedCount;
                }
                else
                {
                    player->SendEquipError(msg, nullptr, nullptr, item.itemid);
                    return;
                }
            }
        }
    
        if (loot.isLooted())
        {
            creature->RemoveUnitFlag(UNIT_FLAG_SKINNABLE);
            creature->AllLootRemovedFromCorpse();
            creature->RemoveDynamicFlag(UNIT_DYNFLAG_LOOTABLE);
            loot.clear();
        }

        loot.RemoveLooter(player->GetGUID());
        player->RemoveAELootedObject(loot.GetGUID());
    }
};

void AddSC_AutoLoot()
{
    new AutoLoot();
}