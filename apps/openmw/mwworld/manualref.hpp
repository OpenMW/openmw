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
            void create (const MWWorld::Store<T>& list, const std::string& name)
            {
                const T* base = list.find(name);

                ESM::CellRef cellRef;
                cellRef.mRefNum.mIndex = 0;
                cellRef.mRefNum.mContentFile = -1;
                cellRef.mRefID = name;
                cellRef.mScale = 1;
                cellRef.mFactionRank = 0;
                cellRef.mCharge = -1;
                cellRef.mGoldValue = 1;
                cellRef.mEnchantmentCharge = -1;
                cellRef.mTeleport = false;
                cellRef.mLockLevel = 0;
                cellRef.mReferenceBlocked = 0;

                LiveCellRef<T> ref(cellRef, base);

                mRef = ref;
                mPtr = Ptr (&boost::any_cast<LiveCellRef<T>&> (mRef), 0);
            }

        public:

            ManualRef (const MWWorld::ESMStore& store, const std::string& name, const int count=1)
            {
                std::string lowerName = Misc::StringUtils::lowerCase (name);
                switch (store.find (lowerName))
                {
                    case ESM::REC_ACTI: create (store.get<ESM::Activator>(), lowerName); break;
                    case ESM::REC_ALCH: create (store.get<ESM::Potion>(), lowerName); break;
                    case ESM::REC_APPA: create (store.get<ESM::Apparatus>(), lowerName); break;
                    case ESM::REC_ARMO: create (store.get<ESM::Armor>(), lowerName); break;
                    case ESM::REC_BOOK: create (store.get<ESM::Book>(), lowerName); break;
                    case ESM::REC_CLOT: create (store.get<ESM::Clothing>(), lowerName); break;
                    case ESM::REC_CONT: create (store.get<ESM::Container>(), lowerName); break;
                    case ESM::REC_CREA: create (store.get<ESM::Creature>(), lowerName); break;
                    case ESM::REC_DOOR: create (store.get<ESM::Door>(), lowerName); break;
                    case ESM::REC_INGR: create (store.get<ESM::Ingredient>(), lowerName); break;
                    case ESM::REC_LEVC: create (store.get<ESM::CreatureLevList>(), lowerName); break;
                    case ESM::REC_LEVI: create (store.get<ESM::ItemLevList>(), lowerName); break;
                    case ESM::REC_LIGH: create (store.get<ESM::Light>(), lowerName); break;
                    case ESM::REC_LOCK: create (store.get<ESM::Lockpick>(), lowerName); break;
                    case ESM::REC_MISC: create (store.get<ESM::Miscellaneous>(), lowerName); break;
                    case ESM::REC_NPC_: create (store.get<ESM::NPC>(), lowerName); break;
                    case ESM::REC_PROB: create (store.get<ESM::Probe>(), lowerName); break;
                    case ESM::REC_REPA: create (store.get<ESM::Repair>(), lowerName); break;
                    case ESM::REC_STAT: create (store.get<ESM::Static>(), lowerName); break;
                    case ESM::REC_WEAP: create (store.get<ESM::Weapon>(), lowerName); break;

                    case 0:
                        throw std::logic_error ("failed to create manual cell ref for " + lowerName + " (unknown ID)");

                    default:
                        throw std::logic_error ("failed to create manual cell ref for " + lowerName + " (unknown type)");
                }

                mPtr.getRefData().setCount(count);
            }

            const Ptr& getPtr() const
            {
                return mPtr;
            }
    };
}

#endif
