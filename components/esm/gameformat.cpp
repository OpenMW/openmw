#include "gameformat.hpp"

#include <algorithm>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include <components/misc/strings/algorithm.hpp>

namespace ESM
{
    namespace
    {
        constexpr std::pair<std::string_view, GameFormat> gameFormatNames[] = {
            { "TES3", GameFormat::TES3 },
            { "TES4", GameFormat::TES4 },
            { "FO3", GameFormat::FO3 },
            { "FONV", GameFormat::FONV },
            { "TES5", GameFormat::TES5 },
            { "FO4", GameFormat::FO4 },
        };
    }

    std::istream& operator>>(std::istream& stream, GameFormat& format)
    {
        std::string name;
        stream >> name;
        const auto it = std::ranges::find_if(
            gameFormatNames, [&](const auto& entry) { return Misc::StringUtils::ciEqual(entry.first, name); });
        if (it == std::end(gameFormatNames))
            stream.setstate(std::ios::failbit);
        else
            format = it->second;
        return stream;
    }

    std::ostream& operator<<(std::ostream& stream, GameFormat format)
    {
        const auto it = std::ranges::find(gameFormatNames, format, &std::pair<std::string_view, GameFormat>::second);
        return stream << (it != std::end(gameFormatNames) ? it->first : "Unknown");
    }

    std::optional<GameFormat> getGameFormatFromMasters(
        std::span<const MasterData> masters, std::span<const GameFormat> masterFormats)
    {
        static constexpr std::pair<std::string_view, GameFormat> ambiguousMainFiles[] = {
            { "Fallout3.esm", GameFormat::FO3 },
            { "Skyrim.esm", GameFormat::TES5 },
        };

        // Search through previously-loaded masters
        for (GameFormat format : masterFormats)
            if (format != GameFormat::Unknown)
                return format;

        // Otherwise, find a matching name. Still useful for the ESM tool, which doesn't load masters.
        for (const MasterData& master : masters)
            for (const auto& [name, format] : ambiguousMainFiles)
                if (Misc::StringUtils::ciEqual(master.name, name))
                    return format;
        return std::nullopt;
    }

    GameFormat getGameFormat(std::uint32_t version, std::optional<std::uint16_t> headerFormVersion,
        std::span<const MasterData> masters, std::span<const GameFormat> masterFormats)
    {
        switch (version)
        {
            case VER_120:
            case VER_130:
                return GameFormat::TES3;
            case VER_080:
                return GameFormat::TES4;
            case VER_100:
                return headerFormVersion.has_value() ? GameFormat::FO4 : GameFormat::TES4;
            case VER_094:
                if (const std::optional<GameFormat> format = getGameFormatFromMasters(masters, masterFormats))
                    return *format;
                return headerFormVersion.value_or(0) <= 15 ? GameFormat::FO3 : GameFormat::TES5;
            case VER_132:
            case VER_133:
            case VER_134:
                return GameFormat::FONV;
            case VER_170:
            case VER_171:
                return GameFormat::TES5;
            case VER_095:
                return GameFormat::FO4;
        }
        return GameFormat::Unknown;
    }
}
