#ifndef GAME_MWWORLD_PTR_H
#define GAME_MWWORLD_PTR_H

#include <cassert>

#include <boost/any.hpp>

#include <components/esm/loadcrea.hpp>
#include <components/esm/loadnpc.hpp>
#include <components/esm_store/cell_store.hpp>

#include "../mwmechanics/creaturestats.hpp"

#include "refdata.hpp"

namespace MWWorld
{
    /// \brief Pointer to a LiveCellRef
    
    class Ptr
    {
        public:
        
            typedef ESMS::CellStore<RefData> CellStore;
    
            boost::any mPtr;
            ESM::CellRef *mCellRef;
            RefData *mRefData;
            CellStore *mCell;
    
        public:
        
            Ptr() : mCellRef (0), mRefData (0), mCell (0) {}
            
            bool isEmpty() const
            {
                return mPtr.empty();
            }
            
            const std::type_info& getType()
            {
                assert (!mPtr.empty());
                return mPtr.type();
            }
            
            template<typename T>
            Ptr (ESMS::LiveCellRef<T, RefData> *liveCellRef, CellStore *cell)
            {
                mPtr = liveCellRef;
                mCellRef = &liveCellRef->ref;
                mRefData = &liveCellRef->mData;
                mCell = cell;
            }
            
            template<typename T>
            ESMS::LiveCellRef<T, RefData> *get() const
            {
                return boost::any_cast<ESMS::LiveCellRef<T, RefData>*> (mPtr);
            }
    
            ESM::CellRef& getCellRef() const
            {
                assert (mCellRef);
                return *mCellRef;
            }
    
            RefData& getRefData() const
            {
                assert (mRefData);
                return *mRefData;
            }
            
            Ptr::CellStore *getCell() const
            {
                assert (mCell);
                return mCell;
            }
            
            /// Throws an exception, if the ID type does not support creature stats.
            MWMechanics::CreatureStats& getCreatureStats() const
            {
                RefData& data = getRefData();
                
                if (!data.getCreatureStats().get())
                {
                    if (mPtr.type()==typeid (ESMS::LiveCellRef<ESM::Creature, RefData> *))
                    {
                        boost::shared_ptr<MWMechanics::CreatureStats> stats (
                            new MWMechanics::CreatureStats);
                    
                        ESMS::LiveCellRef<ESM::Creature, RefData> *ref = get<ESM::Creature>();
                        
                        stats->mAttributes[0].set (ref->base->data.strength);
                        stats->mAttributes[1].set (ref->base->data.intelligence);
                        stats->mAttributes[2].set (ref->base->data.willpower);
                        stats->mAttributes[3].set (ref->base->data.agility);
                        stats->mAttributes[4].set (ref->base->data.speed);
                        stats->mAttributes[5].set (ref->base->data.endurance);
                        stats->mAttributes[6].set (ref->base->data.personality);
                        stats->mAttributes[7].set (ref->base->data.luck);

                        data.getCreatureStats() = stats;
                    }
                    else if (mPtr.type()==typeid (ESMS::LiveCellRef<ESM::NPC, RefData> *))
                    {
                        boost::shared_ptr<MWMechanics::CreatureStats> stats (
                            new MWMechanics::CreatureStats);
                    
                        ESMS::LiveCellRef<ESM::NPC, RefData> *ref = get<ESM::NPC>();
                        
                        stats->mAttributes[0].set (ref->base->npdt52.strength);
                        stats->mAttributes[1].set (ref->base->npdt52.intelligence);
                        stats->mAttributes[2].set (ref->base->npdt52.willpower);
                        stats->mAttributes[3].set (ref->base->npdt52.agility);
                        stats->mAttributes[4].set (ref->base->npdt52.speed);
                        stats->mAttributes[5].set (ref->base->npdt52.endurance);
                        stats->mAttributes[6].set (ref->base->npdt52.personality);
                        stats->mAttributes[7].set (ref->base->npdt52.luck);

                        data.getCreatureStats() = stats;
                    }
                    else
                        throw std::runtime_error (
                            "CreatureStats not available for this ID type");                
                }
            
                return *data.getCreatureStats();
            }            
    };
    
    inline bool operator== (const Ptr& left, const Ptr& right)
    {
        return left.mRefData==right.mRefData;
    }
    
    inline bool operator!= (const Ptr& left, const Ptr& right)
    {
        return !(left==right);
    }

    inline bool operator< (const Ptr& left, const Ptr& right)
    {
        return left.mRefData<right.mRefData;
    }

    inline bool operator>= (const Ptr& left, const Ptr& right)
    {
        return !(left<right);
    }

    inline bool operator> (const Ptr& left, const Ptr& right)
    {
        return right<left;
    }

    inline bool operator<= (const Ptr& left, const Ptr& right)
    {
        return !(left>right);
    }
}

#endif

