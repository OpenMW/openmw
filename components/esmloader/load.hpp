#ifndef OPENMW_COMPONENTS_ESMLOADER_LOAD_H
#define OPENMW_COMPONENTS_ESMLOADER_LOAD_H

#include <components/esm3/esmreader.hpp>

#include <string>
#include <vector>

namespace ToUTF8
{
    class Utf8Encoder;
}

namespace Files
{
    class Collections;
}

namespace Loading
{
    class Listener;
}

namespace EsmLoader
{
    struct EsmData;

    inline constexpr std::size_t fileProgress = 1000;

    struct Query
    {
        bool mLoadActivators = false;
        bool mLoadCells = false;
        bool mLoadContainers = false;
        bool mLoadDoors = false;
        bool mLoadGameSettings = false;
        bool mLoadLands = false;
        bool mLoadStatics = false;
    };

    EsmData loadEsmData(const Query& query, const std::vector<std::string>& contentFiles,
        const Files::Collections& fileCollections, ESM::ReadersCache& readers,
        ToUTF8::Utf8Encoder* encoder, Loading::Listener* listener = nullptr);
}

#endif
