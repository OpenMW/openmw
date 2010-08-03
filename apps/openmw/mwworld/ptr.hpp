#ifndef GAME_MWWORLD_PTR_H
#define GAME_MWWORLD_PTR_H

#include <cassert>

#include <boost/any.hpp>

#include <components/esm/loadcrea.hpp>
#include <components/esm/loadnpc.hpp>
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
            std::string mTypeName;

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

            const std::string& getTypeName() const
            {
                return mTypeName;
            }

            template<typename T>
            Ptr (ESMS::LiveCellRef<T, RefData> *liveCellRef, CellStore *cell)
            {
                mPtr = liveCellRef;
                mCellRef = &liveCellRef->ref;
                mRefData = &liveCellRef->mData;
                mCell = cell;
                mTypeName = typeid (T).name();
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
