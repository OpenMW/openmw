#include "file.hpp"

#include <format>
#include <stdexcept>

#include "stream.hpp"

namespace Spt
{
    SPTFilePtr parse(Files::IStreamPtr&& inputStream)
    {
        auto file = std::make_shared<SPTFile>();
        SPTStream stream(std::move(inputStream));

        int32_t section = stream.readSection();
        if (section != 1000)
            throw std::runtime_error(std::format("Expected SPT section 1000, got {}", section));
        file->mVersionId = stream.readIntString();
        if (file->mVersionId != "__IdvSpt_02_")
            throw std::runtime_error(std::format("Unsupported SPT version '{}'", file->mVersionId));

        section = stream.readSection();
        throw std::runtime_error(std::format("Unsupported section {} in SPT file", section));
    }
}
