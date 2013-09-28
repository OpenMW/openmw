#ifndef GAME_MWWORLD_PTR_H
#define GAME_MWWORLD_PTR_H

#include <boost/any.hpp>

#include "cellstore.hpp"

namespace MWWorld
{
    class ContainerStore;

    /// \brief Pointer to a LiveCellRef

    class Ptr
    {
        public:

            typedef MWWorld::CellStore CellStore;
            ///< \deprecated

            boost::any mPtr;
            ESM::CellRef *mCellRef;
            RefData *mRefData;
            CellStore *mCell;
            std::string mTypeName;
            ContainerStore *mContainerStore;

        public:

            Ptr() : mCellRef (0), mRefData (0), mCell (0), mContainerStore (0) {}

            bool isEmpty() const
            {
                return mPtr.empty();
            }

            const std::type_info& getType() const
            {
                assert (!mPtr.empty());
                return mPtr.type();
            }

            const std::string& getTypeName() const
            {
                return mTypeName;
            }

            template<typename T>
            Ptr (MWWorld::LiveCellRef<T> *liveCellRef, CellStore *cell)
            : mContainerStore (0)
            {
                mPtr = liveCellRef;
                mCellRef = &liveCellRef->ref;
                mRefData = &liveCellRef->mData;
                mCell = cell;
                mTypeName = typeid (T).name();
            }

            template<typename T>
            MWWorld::LiveCellRef<T> *get() const
            {
                return boost::any_cast<MWWorld::LiveCellRef<T>*> (mPtr);
            }

            ESM::CellRef& getCellRef() const;

            RefData& getRefData() const;

            Ptr::CellStore *getCell() const
            {
                assert (mCell);
                return mCell;
            }

            bool isInCell() const
            {
                return (mCell != 0);
            }

            void setContainerStore (ContainerStore *store);
            ///< Must not be called on references that are in a cell.

            ContainerStore *getContainerStore() const;
            ///< May return a 0-pointer, if reference is not in a container.
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
