#ifndef OPENMW_COMPONENTS_MISC_GUARDED_H
#define OPENMW_COMPONENTS_MISC_GUARDED_H

#include <mutex>
#include <memory>

namespace Misc
{
    template <class T>
    class Locked
    {
        public:
            Locked(std::mutex& mutex, T& value)
                : mLock(mutex), mValue(value)
            {}

            T& get() const
            {
                return mValue.get();
            }

            T* operator ->() const
            {
                return std::addressof(get());
            }

            T& operator *() const
            {
                return get();
            }

        private:
            std::unique_lock<std::mutex> mLock;
            std::reference_wrapper<T> mValue;
    };

    template <class T>
    class ScopeGuarded
    {
        public:
            ScopeGuarded()
                : mMutex()
                , mValue()
            {}

            ScopeGuarded(const T& value)
                : mMutex()
                , mValue(value)
            {}

            ScopeGuarded(T&& value)
                : mMutex()
                , mValue(std::move(value))
            {}

            template <class ... Args>
            ScopeGuarded(Args&& ... args)
                : mMutex()
                , mValue(std::forward<Args>(args) ...)
            {}

            ScopeGuarded(const ScopeGuarded& other)
                : mMutex()
                , mValue(other.lock().get())
            {}

            ScopeGuarded(ScopeGuarded&& other)
                : mMutex()
                , mValue(std::move(other.lock().get()))
            {}

            Locked<T> lock()
            {
                return Locked<T>(mMutex, mValue);
            }

            Locked<const T> lockConst()
            {
                return Locked<const T>(mMutex, mValue);
            }

        private:
            std::mutex mMutex;
            T mValue;
    };
}

#endif
