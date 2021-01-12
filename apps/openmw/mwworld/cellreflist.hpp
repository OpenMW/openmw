#ifndef GAME_MWWORLD_CELLREFLIST_H
#define GAME_MWWORLD_CELLREFLIST_H

#include <list>

#include "livecellref.hpp"

namespace MWWorld
{
    /// \brief Collection of references of one type
    template <typename X>
    struct CellRefList
    {
        typedef LiveCellRef<X> LiveRef;
        typedef std::list<LiveRef> List;
        List mList;

        /// Search for the given reference in the given reclist from
        /// ESMStore. Insert the reference into the list if a match is
        /// found. If not, throw an exception.
        /// Moved to cpp file, as we require a custom compare operator for it,
        /// and the build will fail with an ugly three-way cyclic header dependence
        /// so we need to pass the instantiation of the method to the linker, when
        /// all methods are known.
        void load (ESM::CellRef &ref, bool deleted, const MWWorld::ESMStore &esmStore);

        LiveRef &insert (const LiveRef &item)
        {
            mList.push_back(item);
            return mList.back();
        }

        /// Remove all references with the given refNum from this list.
        void remove (const ESM::RefNum &refNum)
        {
            for (typename List::iterator it = mList.begin(); it != mList.end();)
            {
                if (*it == refNum)
                    mList.erase(it++);
                else
                    ++it;
            }
        }
    };
}

#endif
