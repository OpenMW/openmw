#ifndef GAME_MWCLASS_ITEMLEVLIST_H
#define GAME_MWCLASS_ITEMLEVLIST_H

#include "../mwworld/registeredclass.hpp"

namespace MWClass
{
    class ItemLevList : public MWWorld::RegisteredClass<ItemLevList>
    {
            friend MWWorld::RegisteredClass<ItemLevList>;

            ItemLevList();

        public:

            std::string getName (const MWWorld::ConstPtr& ptr) const override;
            ///< \return name or ID; can return an empty string.

            bool hasToolTip (const MWWorld::ConstPtr& ptr) const override;
            ///< @return true if this object has a tooltip when focused (default implementation: true)
    };
}

#endif
