#ifndef GAME_MWWORLD_MANUALREF_H
#define GAME_MWWORLD_MANUALREF_H

#include <boost/any.hpp>

#include <components/esm_store/cell_store.hpp>
#include <components/esm_store/store.hpp>

#include "ptr.hpp"

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
                    ESMS::LiveCellRef<T, RefData> ref;
                    ref.base = instance;

                    mRef = ref;
                    mPtr = Ptr (&boost::any_cast<ESMS::LiveCellRef<T, RefData>&> (mRef), 0);

                    return true;
                }

                return false;
            }

            template<typename T>
            bool create (const ESMS::RecListWithIDT<T>& list, const std::string& name)
            {
                if (const T *instance = list.search (name))
                {
                    ESMS::LiveCellRef<T, RefData> ref;
                    ref.base = instance;

                    mRef = ref;
                    mPtr = Ptr (&boost::any_cast<ESMS::LiveCellRef<T, RefData>&> (mRef), 0);

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
                cellRef.refnum = -1;
                cellRef.scale = 1;
                cellRef.factIndex = 0;
                cellRef.charge = 0;
                cellRef.intv = 0;
                cellRef.nam9 = 0;
                cellRef.teleport = false;
                cellRef.lockLevel = 0;
                cellRef.unam = 0;
            }

            const Ptr& getPtr() const
            {
                return mPtr;
            }
    };
}

#endif
