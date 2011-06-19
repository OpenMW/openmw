#ifndef GAME_MWWORLD_CONTAINERSTORE_H
#define GAME_MWWORLD_CONTAINERSTORE_H

#include <components/esm_store/cell_store.hpp>

namespace MWWorld
{
    template<typename D>
    struct ContainerStore
    {
            ESMS::CellRefList<ESM::Potion, D>            potions;
            ESMS::CellRefList<ESM::Apparatus, D>         appas;
            ESMS::CellRefList<ESM::Armor, D>             armors;
            ESMS::CellRefList<ESM::Book, D>              books;
            ESMS::CellRefList<ESM::Clothing, D>          clothes;
            ESMS::CellRefList<ESM::Ingredient, D>        ingreds;
            ESMS::CellRefList<ESM::Light, D>             lights;
            ESMS::CellRefList<ESM::Tool, D>              lockpicks;
            ESMS::CellRefList<ESM::Miscellaneous, D>     miscItems;
            ESMS::CellRefList<ESM::Probe, D>             probes;
            ESMS::CellRefList<ESM::Repair, D>            repairs;
            ESMS::CellRefList<ESM::Weapon, D>            weapons;
    };
}

#endif
