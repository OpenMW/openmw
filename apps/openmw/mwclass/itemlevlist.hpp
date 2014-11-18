#ifndef GAME_MWCLASS_ITEMLEVLIST_H
#define GAME_MWCLASS_ITEMLEVLIST_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class ItemLevList : public MWWorld::Class
    {
        public:

            /// Return ID of \a ptr
            virtual std::string getId (const MWWorld::Ptr& ptr) const;

            virtual std::string getName (const MWWorld::Ptr& ptr) const;
            ///< \return name (the one that is to be presented to the user; not the internal one);
            /// can return an empty string.

            static void registerSelf();
    };
}

#endif
