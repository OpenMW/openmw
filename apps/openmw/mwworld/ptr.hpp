#ifndef GAME_MWWORLD_PTR_H
#define GAME_MWWORLD_PTR_H

#include <cassert>

#include <string>
#include <sstream>

#include "livecellref.hpp"

namespace MWWorld
{
    class ContainerStore;
    class CellStore;
    struct LiveCellRefBase;

    /// \brief Pointer to a LiveCellRef

    class Ptr
    {
        public:

            MWWorld::LiveCellRefBase *mRef;
            CellStore *mCell;
            ContainerStore *mContainerStore;

        public:
            Ptr(MWWorld::LiveCellRefBase *liveCellRef=nullptr, CellStore *cell=nullptr)
              : mRef(liveCellRef), mCell(cell), mContainerStore(nullptr)
            {
            }

            bool isEmpty() const
            {
                return mRef == nullptr;
            }

            const std::string& getTypeName() const;

            const Class& getClass() const
            {
                if(mRef != nullptr)
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
                if(mRef != nullptr) str<< getTypeName();
                else str<< "an empty object";

                throw std::runtime_error(str.str());
            }

            MWWorld::LiveCellRefBase *getBase() const;

            MWWorld::CellRef& getCellRef() const;

            RefData& getRefData() const;

            CellStore *getCell() const
            {
                assert(mCell);
                return mCell;
            }

            bool isInCell() const
            {
                return (mContainerStore == nullptr) && (mCell != nullptr);
            }

            void setContainerStore (ContainerStore *store);
            ///< Must not be called on references that are in a cell.

            ContainerStore *getContainerStore() const;
            ///< May return a 0-pointer, if reference is not in a container.

            operator const void *();
            ///< Return a 0-pointer, if Ptr is empty; return a non-0-pointer, if Ptr is not empty
    };

    /// \brief Pointer to a const LiveCellRef
    /// @note a Ptr can be implicitely converted to a ConstPtr, but you can not convert a ConstPtr to a Ptr.
    class ConstPtr
    {
    public:

        const MWWorld::LiveCellRefBase *mRef;
        const CellStore *mCell;
        const ContainerStore *mContainerStore;

    public:
        ConstPtr(const MWWorld::LiveCellRefBase *liveCellRef=nullptr, const CellStore *cell=nullptr)
          : mRef(liveCellRef), mCell(cell), mContainerStore(nullptr)
        {
        }

        ConstPtr(const MWWorld::Ptr& ptr)
            : mRef(ptr.mRef), mCell(ptr.mCell), mContainerStore(ptr.mContainerStore)
        {
        }

        bool isEmpty() const
        {
            return mRef == nullptr;
        }

        const std::string& getTypeName() const;

        const Class& getClass() const
        {
            if(mRef != nullptr)
                return *(mRef->mClass);
            throw std::runtime_error("Cannot get class of an empty object");
        }

        template<typename T>
        const MWWorld::LiveCellRef<T> *get() const
        {
            const MWWorld::LiveCellRef<T> *ref = dynamic_cast<const MWWorld::LiveCellRef<T>*>(mRef);
            if(ref) return ref;

            std::stringstream str;
            str<< "Bad LiveCellRef cast to "<<typeid(T).name()<<" from ";
            if(mRef != nullptr) str<< getTypeName();
            else str<< "an empty object";

            throw std::runtime_error(str.str());
        }

        const MWWorld::LiveCellRefBase *getBase() const;

        const MWWorld::CellRef& getCellRef() const
        {
            assert(mRef);
            return mRef->mRef;
        }

        const RefData& getRefData() const
        {
            assert(mRef);
            return mRef->mData;
        }

        const CellStore *getCell() const
        {
            assert(mCell);
            return mCell;
        }

        bool isInCell() const
        {
            return (mContainerStore == nullptr) && (mCell != nullptr);
        }
        
        void setContainerStore (const ContainerStore *store);
        ///< Must not be called on references that are in a cell.
        
        const ContainerStore *getContainerStore() const;
        ///< May return a 0-pointer, if reference is not in a container.

        operator const void *();
        ///< Return a 0-pointer, if Ptr is empty; return a non-0-pointer, if Ptr is not empty
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

    inline bool operator== (const ConstPtr& left, const ConstPtr& right)
    {
        return left.mRef==right.mRef;
    }

    inline bool operator!= (const ConstPtr& left, const ConstPtr& right)
    {
        return !(left==right);
    }

    inline bool operator< (const ConstPtr& left, const ConstPtr& right)
    {
        return left.mRef<right.mRef;
    }

    inline bool operator>= (const ConstPtr& left, const ConstPtr& right)
    {
        return !(left<right);
    }

    inline bool operator> (const ConstPtr& left, const ConstPtr& right)
    {
        return right<left;
    }

    inline bool operator<= (const ConstPtr& left, const ConstPtr& right)
    {
        return !(left>right);
    }
}

#endif
