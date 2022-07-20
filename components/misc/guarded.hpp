#ifndef OPENMW_COMPONENTS_MISC_GUARDED_H
#define OPENMW_COMPONENTS_MISC_GUARDED_H

#include <mutex>
#include <memory>
#include <condition_variable>
#include <type_traits>

namespace Misc
{
    template <class T>
    class Locked
    {
        public:
            Locked(std::mutex& mutex, std::remove_reference_t<T>& value)
                : mLock(mutex), mValue(value)
            {}

            std::remove_reference_t<T>& get() const
            {
                return mValue.get();
            }

            std::remove_reference_t<T>* operator ->() const
            {
                return &get();
            }

            std::remove_reference_t<T>& operator *() const
            {
                return get();
            }

        private:
            std::unique_lock<std::mutex> mLock;
            std::reference_wrapper<std::remove_reference_t<T>> mValue;
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

            Locked<const T> lockConst() const
            {
                return Locked<const T>(mMutex, mValue);
            }

            template <class Predicate>
            void wait(std::condition_variable& cv, Predicate&& predicate)
            {
                std::unique_lock<std::mutex> lock(mMutex);
                cv.wait(lock, [&] { return predicate(mValue); });
            }

        private:
            mutable std::mutex mMutex;
            T mValue;
    };
}

#endif
