#ifndef COMPONENT_ESM_GAMEFORMAT_H
#define COMPONENT_ESM_GAMEFORMAT_H

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <span>

#include <components/esm/common.hpp>

namespace ESM
{
    enum class GameFormat
    {
        Unknown,
        TES3,
        TES4,
        FO3,
        FONV,
        TES5,
        FO4,
    };

    std::istream& operator>>(std::istream& stream, GameFormat& format);

    std::ostream& operator<<(std::ostream& stream, GameFormat format);

    // The game format associated with a list of masters.
    std::optional<GameFormat> getGameFormatFromMasters(
        std::span<const MasterData> masters, std::span<const GameFormat> masterFormats = {});

    // The game format for a content file. Games weren't meant to coexist, so it's our best educated guess.
    // ESM version -> previous-masters' game format -> matching master names -> header form version
    GameFormat getGameFormat(std::uint32_t version, std::optional<std::uint16_t> headerFormVersion,
        std::span<const MasterData> masters = {}, std::span<const GameFormat> masterFormats = {});
}

#endif // COMPONENT_ESM_GAMEFORMAT_H
