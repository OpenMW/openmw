#ifndef GAME_MWCLASS_ITEMLEVLIST_H
#define GAME_MWCLASS_ITEMLEVLIST_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class ItemLevList : public MWWorld::Class
    {
        public:

            virtual std::string getName (const MWWorld::ConstPtr& ptr) const;
            ///< \return name (the one that is to be presented to the user; not the internal one);
            /// can return an empty string.

            static void registerSelf();
    };
}

#endif
