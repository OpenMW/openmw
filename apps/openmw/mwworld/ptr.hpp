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
            
            Ptr::CellStore *getCell() const
            {
                assert (mCell);
                return mCell;
            }
    };
}

#endif

