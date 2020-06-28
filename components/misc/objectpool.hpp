#ifndef OPENMW_COMPONENTS_MISC_OBJECTPOOL_H
#define OPENMW_COMPONENTS_MISC_OBJECTPOOL_H

#include <deque>
#include <memory>
#include <vector>

namespace Misc
{
    template <class T>
    class ObjectPool;

    template <class T>
    class ObjectPtrDeleter
    {
        public:
            ObjectPtrDeleter(std::nullptr_t)
                : mPool(nullptr) {}

            ObjectPtrDeleter(ObjectPool<T>& pool)
                : mPool(&pool) {}

            void operator()(T* object) const
            {
                mPool->recycle(object);
            }

        private:
            ObjectPool<T>* mPool;
    };

    template <class T>
    struct ObjectPtr final : std::unique_ptr<T, ObjectPtrDeleter<T>>
    {
        using std::unique_ptr<T, ObjectPtrDeleter<T>>::unique_ptr;
        using std::unique_ptr<T, ObjectPtrDeleter<T>>::operator=;

        ObjectPtr()
            : ObjectPtr(nullptr) {}

        ObjectPtr(std::nullptr_t)
            : std::unique_ptr<T, ObjectPtrDeleter<T>>(nullptr, nullptr) {}
    };

    template <class T>
    class ObjectPool
    {
        friend class ObjectPtrDeleter<T>;

        public:
            ObjectPool()
                : mObjects(std::make_unique<std::deque<T>>()) {}

            ObjectPtr<T> get()
            {
                T* object;

                if (!mUnused.empty())
                {
                    object = mUnused.back();
                    mUnused.pop_back();
                }
                else
                {
                    mObjects->emplace_back();
                    object = &mObjects->back();
                }

                return ObjectPtr<T>(object, ObjectPtrDeleter<T>(*this));
            }

        private:
            std::unique_ptr<std::deque<T>> mObjects;
            std::vector<T*> mUnused;

            void recycle(T* object)
            {
                mUnused.push_back(object);
            }
    };
}

#endif
