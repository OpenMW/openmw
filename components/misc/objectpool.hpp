#ifndef OPENMW_COMPONENTS_MISC_OBJECTPOOL_H
#define OPENMW_COMPONENTS_MISC_OBJECTPOOL_H

#include <deque>
#include <memory>
#include <vector>

namespace Misc
{
    template <class T>
    class ObjectPool
    {
        public:
            ObjectPool()
                : mObjects(std::make_unique<std::deque<T>>()) {}

            T* get()
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

                return object;
            }

            void recycle(T* object)
            {
                mUnused.push_back(object);
            }

        private:
            std::unique_ptr<std::deque<T>> mObjects;
            std::vector<T*> mUnused;
    };
}

#endif
