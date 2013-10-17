#ifndef GAME_MWWORLD_PTR_H
#define GAME_MWWORLD_PTR_H

#include "cellstore.hpp"
#include "livecellref.hpp"

namespace MWWorld
{
    class ContainerStore;

    /// \brief Pointer to a LiveCellRef

    class Ptr
    {
        public:

            typedef MWWorld::CellStore CellStore;
            ///< \deprecated

            MWWorld::LiveCellRefBase *mRef;
            CellStore *mCell;
            ContainerStore *mContainerStore;

        public:
            Ptr(MWWorld::LiveCellRefBase *liveCellRef=0, CellStore *cell=0)
              : mRef(liveCellRef), mCell(cell), mContainerStore(0)
            {
            }

            bool isEmpty() const
            {
                return mRef == 0;
            }

            const std::string& getTypeName() const;

            const Class& getClass() const
            {
                if(mRef != 0)
                    return *(mRef->mClass);
                throw std::runtime_error("Cannot get class of an empty object");
            }

            template<typename T>
            MWWorld::LiveCellRef<T> *get() const
            {
                MWWorld::LiveCellRef<T> *ref = dynamic_cast<MWWorld::LiveCellRef<T>*>(mRef);
                if(ref) return ref;

                std::stringstream str;
                str<< "Bad LiveCellRef cast to "<<typeid(T).name()<<" from ";
                if(mRef != 0) str<< getTypeName();
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
        return left.mRef==right.mRef;
    }

    inline bool operator!= (const Ptr& left, const Ptr& right)
    {
        return !(left==right);
    }

    inline bool operator< (const Ptr& left, const Ptr& right)
    {
        return left.mRef<right.mRef;
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
