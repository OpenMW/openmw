#ifndef GAME_MWWORLD_MANUALREF_H
#define GAME_MWWORLD_MANUALREF_H

#include <boost/any.hpp>

#include <components/esm_store/store.hpp>

#include "ptr.hpp"
#include "cellstore.hpp"

namespace MWWorld
{
    /// \brief Manually constructed live cell ref
    class ManualRef
    {
            boost::any mRef;
            Ptr mPtr;

            ManualRef (const ManualRef&);
            ManualRef& operator= (const ManualRef&);

            template<typename T>
            bool create (const ESMS::RecListT<T>& list, const std::string& name)
            {
                if (const T *instance = list.search (name))
                {
                    LiveCellRef<T> ref;
                    ref.base = instance;

                    mRef = ref;
                    mPtr = Ptr (&boost::any_cast<LiveCellRef<T>&> (mRef), 0);

                    return true;
                }

                return false;
            }

            template<typename T>
            bool create (const ESMS::RecListWithIDT<T>& list, const std::string& name)
            {
                if (const T *instance = list.search (name))
                {
                    LiveCellRef<T> ref;
                    ref.base = instance;

                    mRef = ref;
                    mPtr = Ptr (&boost::any_cast<LiveCellRef<T>&> (mRef), 0);

                    return true;
                }

                return false;
            }

        public:

            ManualRef (const ESMS::ESMStore& store, const std::string& name)
            {
                // create
                if (!create (store.activators, name) &&
                    !create (store.potions, name) &&
                    !create (store.appas, name) &&
                    !create (store.armors, name) &&
                    !create (store.books, name) &&
                    !create (store.clothes, name) &&
                    !create (store.containers, name) &&
                    !create (store.creatures, name) &&
                    !create (store.doors, name) &&
                    !create (store.ingreds, name) &&
                    !create (store.creatureLists, name) &&
                    !create (store.itemLists, name) &&
                    !create (store.lights, name) &&
                    !create (store.lockpicks, name) &&
                    !create (store.miscItems, name) &&
                    !create (store.npcs, name) &&
                    !create (store.probes, name) &&
                    !create (store.repairs, name) &&
                    !create (store.statics, name) &&
                    !create (store.weapons, name))
                    throw std::logic_error ("failed to create manual cell ref for " + name);

                // initialise
                ESM::CellRef& cellRef = mPtr.getCellRef();
                cellRef.mRefID = name;
                cellRef.mRefnum = -1;
                cellRef.mScale = 1;
                cellRef.mFactIndex = 0;
                cellRef.mCharge = 0;
                cellRef.mIntv = 0;
                cellRef.mNam9 = 0;
                cellRef.mTeleport = false;
                cellRef.mLockLevel = 0;
                cellRef.mUnam = 0;
            }

            const Ptr& getPtr() const
            {
                return mPtr;
            }
    };
}

#endif
