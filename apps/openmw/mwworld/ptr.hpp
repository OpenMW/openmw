#ifndef GAME_MWWORLD_PTR_H
#define GAME_MWWORLD_PTR_H

#include "cellstore.hpp"

namespace MWWorld
{
    class ContainerStore;

    /// \brief Pointer to a LiveCellRef

    class Ptr
    {
            static const std::string sEmptyString;

        public:

            typedef MWWorld::CellStore CellStore;
            ///< \deprecated

            MWWorld::LiveCellRefBase *mPtr;
            ESM::CellRef *mCellRef;
            RefData *mRefData;
            CellStore *mCell;
            ContainerStore *mContainerStore;

        public:

            Ptr() : mPtr (0), mCellRef (0), mRefData (0), mCell (0), mContainerStore (0) {}

            bool isEmpty() const
            {
                return mPtr == 0;
            }

            const std::string& getTypeName() const
            {
                return mPtr ? mPtr->mTypeName : sEmptyString;
            }

            template<typename T>
            Ptr (MWWorld::LiveCellRef<T> *liveCellRef, CellStore *cell)
            : mContainerStore (0)
            {
                mPtr = liveCellRef;
                mCellRef = &liveCellRef->mRef;
                mRefData = &liveCellRef->mData;
                mCell = cell;
            }

            template<typename T>
            MWWorld::LiveCellRef<T> *get() const
            {
                if(mPtr && mPtr->mTypeName == typeid(T).name())
                    return static_cast<MWWorld::LiveCellRef<T>*>(mPtr);

                std::stringstream str;
                str<< "Bad LiveCellRef cast to "<<typeid(T).name()<<" from ";
                if(mPtr != 0) str<< mPtr->mTypeName;
                else str<< "an empty object";

                throw std::runtime_error(str.str());
            }

            ESM::CellRef& getCellRef() const;

            RefData& getRefData() const;

            Ptr::CellStore *getCell() const
            {
                assert(mCell);
                return mCell;
            }

            bool isInCell() const
            {
                return (mContainerStore == 0) && (mCell != 0);
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
