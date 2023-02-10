#ifndef OPENMW_COMPONENTS_ESM3_FORMATVERSION_H
#define OPENMW_COMPONENTS_ESM3_FORMATVERSION_H

#include <cstdint>

namespace ESM
{
    using FormatVersion = std::uint32_t;

    inline constexpr FormatVersion DefaultFormatVersion = 0;
    inline constexpr FormatVersion CurrentContentFormatVersion = 1;
    inline constexpr FormatVersion MaxOldWeatherFormatVersion = 1;
    inline constexpr FormatVersion MaxOldDeathAnimationFormatVersion = 2;
    inline constexpr FormatVersion MaxOldForOfWarFormatVersion = 6;
    inline constexpr FormatVersion MaxWerewolfDeprecatedDataFormatVersion = 7;
    inline constexpr FormatVersion MaxOldTimeLeftFormatVersion = 8;
    inline constexpr FormatVersion MaxIntFallbackFormatVersion = 10;
    inline constexpr FormatVersion MaxClearModifiersFormatVersion = 16;
    inline constexpr FormatVersion MaxOldAiPackageFormatVersion = 17;
    inline constexpr FormatVersion MaxOldSkillsAndAttributesFormatVersion = 18;
    inline constexpr FormatVersion MaxOldCreatureStatsFormatVersion = 19;
    inline constexpr FormatVersion CurrentSaveGameFormatVersion = 22;
}

#endif
