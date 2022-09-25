#ifndef OPENMW_COMPONENTS_NIF_EXCEPTION_HPP
#define OPENMW_COMPONENTS_NIF_EXCEPTION_HPP

#include <filesystem>
#include <stdexcept>
#include <string>

#include <components/files/conversion.hpp>

namespace Nif
{
    struct Exception : std::runtime_error
    {
        explicit Exception(const std::string& message, const std::filesystem::path& path)
            : std::runtime_error("NIFFile Error: " + message + " when reading " + Files::pathToUnicodeString(path))
        {
        }
    };
}

#endif
