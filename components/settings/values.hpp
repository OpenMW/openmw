#ifndef OPENMW_COMPONENTS_SETTINGS_VALUES_H
#define OPENMW_COMPONENTS_SETTINGS_VALUES_H

#include "sanitizerimpl.hpp"
#include "settingvalue.hpp"

namespace Settings
{
    class Values
    {
    public:
        static void init();

    private:
        static Values* sValues;

        friend const Values& values();

        friend Values& valuesMutable();
    };

    inline const Values& values()
    {
        return *Values::sValues;
    }

    inline Values& valuesMutable()
    {
        return *Values::sValues;
    }
}

#endif
