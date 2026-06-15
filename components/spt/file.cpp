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

        {
            int32_t section = stream.readSection();
            if (section != 1000)
                throw std::runtime_error(std::format("Expected SPT section 1000, got {}", section));
            file->mVersionId = stream.readIntString();
            if (file->mVersionId != "__IdvSpt_02_")
                throw std::runtime_error(std::format("Unsupported SPT version '{}'", file->mVersionId));
        }

        {
            int32_t section = stream.readSection();
            if (section != 1002)
                throw std::runtime_error(std::format("Expected SPT section 1002, got {}", section));
        }

        while (true)
        {
            int32_t section = stream.readSection();
            switch (section)
            {
                case 2000:
                    file->mTextureMap = stream.readIntString();
                    break;
                case 2001:
                    file->mLodFarDistance = stream.readFloat();
                    break;
                case 2002:
                    file->mUnknown2002 = stream.readByte();
                    break;
                default:
                    throw std::runtime_error(
                        std::format("Unsupported section {} in SPT file", section));
            }
        }
    }
}