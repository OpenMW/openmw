#ifndef OPENMW_ESMTOOL_ARGUMENTS_H
#define OPENMW_ESMTOOL_ARGUMENTS_H

#include <filesystem>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include <components/esm/common.hpp>
#include <components/esm/format.hpp>

namespace EsmTool
{
    // --game-format values
    inline constexpr std::pair<std::string_view, ESM::GameFormat> gameFormatNames[] = {
        { "TES3", ESM::GameFormat::TES3 },
        { "TES4", ESM::GameFormat::TES4 },
        { "FO3", ESM::GameFormat::FO3 },
        { "FONV", ESM::GameFormat::FONV },
        { "TES5", ESM::GameFormat::TES5 },
        { "FO4", ESM::GameFormat::FO4 },
    };

    struct Arguments
    {
        std::optional<ESM::Format> mRawFormat;
        bool quiet_given = false;
        bool loadcells_given = false;
        bool plain_given = false;

        std::string mode;
        std::string encoding;
        std::filesystem::path filename;
        std::filesystem::path outname;

        std::vector<std::string> types;
        std::string name;

        ESM::GameFormat gameFormat = ESM::GameFormat::Unknown;
    };
}

#endif
