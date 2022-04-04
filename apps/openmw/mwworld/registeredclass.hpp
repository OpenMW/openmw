#ifndef GAME_MWWORLD_REGISTEREDCLASS_H
#define GAME_MWWORLD_REGISTEREDCLASS_H

#include "class.hpp"

namespace MWWorld
{
    template <class Derived, class Base = Class>
    class RegisteredClass : public Base
    {
        public:
            static void registerSelf()
            {
                static Derived instance;
                Base::registerClass(instance);
            }

        protected:
            explicit RegisteredClass(unsigned type) : Base(type) {}
    };
}

#endif
