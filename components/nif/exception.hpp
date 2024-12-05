#ifndef OPENMW_COMPONENTS_NIF_EXCEPTION_HPP
#define OPENMW_COMPONENTS_NIF_EXCEPTION_HPP

#include <stdexcept>
#include <string>

namespace Nif
{
    struct Exception : std::runtime_error
    {
        explicit Exception(std::string_view message, std::string_view path)
            : std::runtime_error([&] {
                std::string result = "NIFFile Error: ";
                result += message;
                result += " when reading ";
                result += path;
                return result;
            }())
        {
        }
    };
}

#endif
