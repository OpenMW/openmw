#ifndef GAME_MWWORLD_MANUALREF_H
#define GAME_MWWORLD_MANUALREF_H

#include <boost/any.hpp>

#include "esmstore.hpp"
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
            bool create (const MWWorld::Store<T>& list, const std::string& name)
            {
                if (const T *instance = list.search (name))
                {
                    LiveCellRef<T> ref;
                    ref.mBase = instance;

                    mRef = ref;
                    mPtr = Ptr (&boost::any_cast<LiveCellRef<T>&> (mRef), 0);

                    return true;
                }

                return false;
            }

        public:

            ManualRef (const MWWorld::ESMStore& store, const std::string& name)
            {
                // create
                if (!create (store.get<ESM::Activator>(), name) &&
                    !create (store.get<ESM::Potion>(), name) &&
                    !create (store.get<ESM::Apparatus>(), name) &&
                    !create (store.get<ESM::Armor>(), name) &&
                    !create (store.get<ESM::Book>(), name) &&
                    !create (store.get<ESM::Clothing>(), name) &&
                    !create (store.get<ESM::Container>(), name) &&
                    !create (store.get<ESM::Creature>(), name) &&
                    !create (store.get<ESM::Door>(), name) &&
                    !create (store.get<ESM::Ingredient>(), name) &&
                    !create (store.get<ESM::CreatureLevList>(), name) &&
                    !create (store.get<ESM::ItemLevList>(), name) &&
                    !create (store.get<ESM::Light>(), name) &&
                    !create (store.get<ESM::Lockpick>(), name) &&
                    !create (store.get<ESM::Miscellaneous>(), name) &&
                    !create (store.get<ESM::NPC>(), name) &&
                    !create (store.get<ESM::Probe>(), name) &&
                    !create (store.get<ESM::Repair>(), name) &&
                    !create (store.get<ESM::Static>(), name) &&
                    !create (store.get<ESM::Weapon>(), name))
                    throw std::logic_error ("failed to create manual cell ref for " + name);

                // initialise
                ESM::CellRef& cellRef = mPtr.getCellRef();
                cellRef.mRefID = name;
                cellRef.mRefnum = -1;
                cellRef.mScale = 1;
                cellRef.mFactIndex = 0;
                cellRef.mCharge = -1;
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
