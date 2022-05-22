#ifndef GAME_MWWORLD_MANUALREF_H
#define GAME_MWWORLD_MANUALREF_H

#include <any>

#include "ptr.hpp"

namespace MWWorld
{
    /// \brief Manually constructed live cell ref
    class ManualRef
    {
            std::any mRef;
            Ptr mPtr;

            ManualRef (const ManualRef&);
            ManualRef& operator= (const ManualRef&);

        public:
            ManualRef(const MWWorld::ESMStore& store, std::string_view name, const int count = 1);

            const Ptr& getPtr() const
            {
                return mPtr;
            }
    };
}

#endif
