#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_EXCEPTIONS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_EXCEPTIONS_H

#include <stdexcept>

namespace DetourNavigator
{
    struct NavigatorException : std::runtime_error
    {
        NavigatorException(const std::string& message) : std::runtime_error(message) {}
        NavigatorException(const char* message) : std::runtime_error(message) {}
    };

    struct InvalidArgument : std::invalid_argument
    {
        InvalidArgument(const std::string& message) : std::invalid_argument(message) {}
        InvalidArgument(const char* message) : std::invalid_argument(message) {}
    };
}

#endif
