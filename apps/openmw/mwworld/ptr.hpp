#ifndef GAME_MWWORLD_PTR_H
#define GAME_MWWORLD_PTR_H

#include <cassert>
#include <type_traits>
#include <string>
#include <sstream>

#include "livecellref.hpp"

namespace MWWorld
{
    class ContainerStore;
    class CellStore;
    struct LiveCellRefBase;

    /// \brief Pointer to a LiveCellRef
    template <template<class> class TypeTransform>
    class PtrBase
    {
        public:

            typedef TypeTransform<MWWorld::LiveCellRefBase> LiveCellRefBaseType;
            typedef TypeTransform<CellStore> CellStoreType;
            typedef TypeTransform<ContainerStore> ContainerStoreType;

            LiveCellRefBaseType *mRef;
            CellStoreType *mCell;
            ContainerStoreType *mContainerStore;

            PtrBase(LiveCellRefBaseType *liveCellRef=nullptr, CellStoreType *cell=nullptr)
              : mRef(liveCellRef), mCell(cell), mContainerStore(nullptr)
            {
            }

            bool isEmpty() const
            {
                return mRef == nullptr;
            }

            unsigned int getType() const
            {
                if(mRef != nullptr)
                    return mRef->getType();
                throw std::runtime_error("Can't get type name from an empty object.");
            }

            std::string getTypeDescription() const
            {
                return mRef ? mRef->getTypeDescription() : "nullptr";
            }

            const Class& getClass() const
            {
                if(mRef != nullptr)
                    return *(mRef->mClass);
                throw std::runtime_error("Cannot get class of an empty object");
            }

            template<typename T>
            TypeTransform<MWWorld::LiveCellRef<T>> *get() const
            {
                TypeTransform<MWWorld::LiveCellRef<T>> *ref = dynamic_cast<TypeTransform<MWWorld::LiveCellRef<T>>*>(mRef);
                if(ref) return ref;

                std::stringstream str;
                str<< "Bad LiveCellRef cast to "<<T::getRecordType()<<" from ";
                if(mRef != nullptr) str<< getTypeDescription();
                else str<< "an empty object";

                throw std::runtime_error(str.str());
            }

            LiveCellRefBaseType *getBase() const
            {
                if (!mRef)
                    throw std::runtime_error ("Can't access cell ref pointed to by null Ptr");
                return mRef;
            }

            TypeTransform<MWWorld::CellRef>& getCellRef() const
            {
                assert(mRef);
                return mRef->mRef;
            }

            TypeTransform<RefData>& getRefData() const
            {
                assert(mRef);
                return mRef->mData;
            }

            CellStoreType *getCell() const
            {
                assert(mCell);
                return mCell;
            }

            bool isInCell() const
            {
                return (mContainerStore == nullptr) && (mCell != nullptr);
            }

            void setContainerStore (ContainerStoreType *store)
            ///< Must not be called on references that are in a cell.
            {
                assert (store);
                assert (!mCell);
                mContainerStore = store;
            }

            ContainerStoreType *getContainerStore() const
            ///< May return a 0-pointer, if reference is not in a container.
            {
                return mContainerStore;
            }

            operator const void *()
            ///< Return a 0-pointer, if Ptr is empty; return a non-0-pointer, if Ptr is not empty
            {
                return mRef;
            }

    };

    class Ptr : public PtrBase<std::remove_const_t> {};
    class ConstPtr : public PtrBase<std::add_const_t>
    {
    public:
        ConstPtr(const Ptr& ptr)
              : PtrBase<std::add_const_t>(ptr.mRef, ptr.mCell)
        {}
    };

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
