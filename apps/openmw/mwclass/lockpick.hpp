#ifndef GAME_MWCLASS_LOCKPICK_H
#define GAME_MWCLASS_LOCKPICK_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class Lockpick : public MWWorld::Class
    {
        public:

            virtual std::string getName (const MWWorld::Ptr& ptr) const;
            ///< \return name (the one that is to be presented to the user; not the internal one);
            /// can return an empty string.

            virtual std::string getScript (const MWWorld::Ptr& ptr) const;
            ///< Return name of the script attached to ptr

            static void registerSelf();
    };
}

#endif
