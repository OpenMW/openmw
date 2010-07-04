#ifndef GAME_MWWORLD_PTR_H
#define GAME_MWWORLD_PTR_H

#include <cassert>

#include <boost/any.hpp>

#include <components/esm_store/cell_store.hpp>

#include "refdata.hpp"

namespace MWWorld
{
    /// \brief Pointer to a LiveCellRef
    
    class Ptr
    {
            boost::any mPtr;
            ESM::CellRef *mCellRef;
            RefData *mRefData;
    
        public:
        
            Ptr() : mCellRef (0), mRefData (0) {}
            
            template<typename T>
            Ptr (ESMS::LiveCellRef<T, RefData> *liveCellRef)
            {
                mPtr = liveCellRef;
                mCellRef = &liveCellRef->ref;
                mRefData = &liveCellRef->mData;
            }
            
            template<typename T>
            ESMS::LiveCellRef<T, RefData> *get() const
            {
                return boost::any_cast<const ESMS::LiveCellRef<T, RefData>*> (mPtr);
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
    };
}

#endif

