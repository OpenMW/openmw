#include "hash.hpp"

#include <components/misc/hash.hpp>

#include <cstdint>
#include <functional>
#include <istream>
#include <string>

namespace Files
{
    std::uint64_t getHash(const std::string& fileName, std::istream& stream)
    {
        std::uint64_t hash = std::hash<std::string> {}(fileName);
        try
        {
            const auto start = stream.tellg();
            const auto exceptions = stream.exceptions();
            stream.exceptions(std::ios_base::badbit);
            while (stream)
            {
                std::uint64_t value = 0;
                stream.read(reinterpret_cast<char*>(&value), sizeof(value));
                Misc::hashCombine(hash, value);
            }
            stream.exceptions(exceptions);
            stream.clear();
            stream.seekg(start);
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error("Error while reading \"" + fileName + "\" to get hash: " + std::string(e.what()));
        }
        return hash;
    }
}
