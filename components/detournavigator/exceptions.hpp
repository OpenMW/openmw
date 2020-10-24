#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_EXCEPTIONS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_EXCEPTIONS_H

#include <stdexcept>

namespace DetourNavigator
{
    struct NavigatorException : std::runtime_error
    {
        explicit NavigatorException(const std::string& message) : std::runtime_error(message) {}
        explicit NavigatorException(const char* message) : std::runtime_error(message) {}
    };

    struct InvalidArgument : std::invalid_argument
    {
        explicit InvalidArgument(const std::string& message) : std::invalid_argument(message) {}
        explicit InvalidArgument(const char* message) : std::invalid_argument(message) {}
    };
}

#endif
