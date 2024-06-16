#ifndef OPENMW_MWWORLD_MAGICEFFECTS_H
#define OPENMW_MWWORLD_MAGICEFFECTS_H

namespace ESM
{
    struct CreatureStats;
    struct InventoryState;
    struct NpcStats;
}

namespace MWWorld
{
    void convertMagicEffects(
        ESM::CreatureStats& creatureStats, ESM::InventoryState& inventory, ESM::NpcStats* npcStats = nullptr);

    void convertStats(ESM::CreatureStats& creatureStats);

    void convertEnchantmentSlots(ESM::CreatureStats& creatureStats, ESM::InventoryState& inventory);
}

#endif
