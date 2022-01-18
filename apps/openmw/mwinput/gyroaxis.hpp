#ifndef MWINPUT_GYROAXIS
#define MWINPUT_GYROAXIS

#include <string_view>

namespace MWInput
{
    enum GyroscopeAxis
    {
        Unknown = 0,
        X = 1,
        Y = 2,
        Z = 3,
        Minus_X = -1,
        Minus_Y = -2,
        Minus_Z = -3
    };

    GyroscopeAxis gyroscopeAxisFromString(std::string_view s);
}

#endif // !MWINPUT_GYROAXIS
