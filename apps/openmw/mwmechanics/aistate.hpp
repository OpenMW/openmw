#ifndef AISTATE_H
#define AISTATE_H

#include "aistatefwd.hpp"
#include "aitemporarybase.hpp"

#include <memory>

namespace MWMechanics
{

    /** \brief stores one object of any class derived from Base.
     *  Requesting a certain derived class via get() either returns
     * the stored object if it has the correct type or otherwise replaces
     * it with an object of the requested type.
     */
    template <class Base>
    class DerivedClassStorage
    {
    private:
        std::unique_ptr<Base> mStorage;

    public:
        /// \brief returns reference to stored object or deletes it and creates a fitting
        template <class Derived>
        Derived& get()
        {
            Derived* result = dynamic_cast<Derived*>(mStorage.get());

            if (result == nullptr)
            {
                auto storage = std::make_unique<Derived>();
                result = storage.get();
                mStorage = std::move(storage);
            }

            // return a reference to the (new allocated) object
            return *result;
        }

        /// \brief returns pointer to stored object in the desired type
        template <class Derived>
        Derived* getPtr() const
        {
            return dynamic_cast<Derived*>(mStorage.get());
        }

        template <class Derived>
        void copy(DerivedClassStorage& destination) const
        {
            Derived* result = dynamic_cast<Derived*>(mStorage.get());
            if (result != nullptr)
                destination.store<Derived>(*result);
        }

        template <class Derived>
        void store(const Derived& payload)
        {
            mStorage = std::make_unique<Derived>(payload);
        }

        /// \brief takes ownership of the passed object
        template <class Derived>
        void moveIn(std::unique_ptr<Derived>&& storage)
        {
            mStorage = std::move(storage);
        }
    };
}

#endif // AISTATE_H
