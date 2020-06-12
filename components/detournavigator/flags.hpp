#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_FLAGS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_FLAGS_H

#include <ostream>

namespace DetourNavigator
{
    using Flags = unsigned short;

    enum Flag : Flags
    {
        Flag_none = 0,
        Flag_walk = 1 << 0,
        Flag_swim = 1 << 1,
        Flag_openDoor = 1 << 2,
        Flag_usePathgrid = 1 << 3,
    };

    inline std::ostream& operator <<(std::ostream& stream, const Flag value)
    {
        switch (value)
        {
            case Flag_none:
                return stream << "none";
            case Flag_walk:
                return stream << "walk";
            case Flag_swim:
                return stream << "swim";
            case Flag_openDoor:
                return stream << "openDoor";
            case Flag_usePathgrid:
                return stream << "usePathgrid";
        }

        return stream;
    }

    struct WriteFlags
    {
        Flags mValue;

        friend inline std::ostream& operator <<(std::ostream& stream, const WriteFlags& value)
        {
            if (value.mValue == Flag_none)
            {
                return stream << Flag_none;
            }
            else
            {
                bool first = true;
                for (const auto flag : {Flag_walk, Flag_swim, Flag_openDoor, Flag_usePathgrid})
                {
                    if (value.mValue & flag)
                    {
                        if (!first)
                            stream << " | ";
                        first = false;
                        stream << flag;
                    }
                }

                return stream;
            }
        }
    };
}

#endif
