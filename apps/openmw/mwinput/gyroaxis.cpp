#include "gyroaxis.hpp"

namespace MWInput
{
    GyroscopeAxis gyroscopeAxisFromString(std::string_view s)
    {
        if (s == "x")
            return GyroscopeAxis::X;
        else if (s == "y")
            return GyroscopeAxis::Y;
        else if (s == "z")
            return GyroscopeAxis::Z;
        else if (s == "-x")
            return GyroscopeAxis::Minus_X;
        else if (s == "-y")
            return GyroscopeAxis::Minus_Y;
        else if (s == "-z")
            return GyroscopeAxis::Minus_Z;

        return GyroscopeAxis::Unknown;
    }
}
