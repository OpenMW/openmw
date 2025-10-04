#ifndef OPENMW_COMPONENTS_ESM3_FORMATVERSION_H
#define OPENMW_COMPONENTS_ESM3_FORMATVERSION_H

#include <cstdint>

namespace ESM
{
    using FormatVersion = std::uint32_t;

    inline constexpr FormatVersion DefaultFormatVersion = 0;
    inline constexpr FormatVersion CurrentContentFormatVersion = 1;
    inline constexpr FormatVersion MaxOldGoldValueFormatVersion = 5;
    inline constexpr FormatVersion MaxOldFogOfWarFormatVersion = 6;
    inline constexpr FormatVersion MaxUnoptimizedCharacterDataFormatVersion = 7;
    inline constexpr FormatVersion MaxOldTimeLeftFormatVersion = 8;
    inline constexpr FormatVersion MaxIntFallbackFormatVersion = 10;
    inline constexpr FormatVersion MaxOldRestockingFormatVersion = 14;
    inline constexpr FormatVersion MaxClearModifiersFormatVersion = 16;
    inline constexpr FormatVersion MaxOldAiPackageFormatVersion = 17;
    inline constexpr FormatVersion MaxOldSkillsAndAttributesFormatVersion = 18;
    inline constexpr FormatVersion MaxOldCreatureStatsFormatVersion = 19;
    inline constexpr FormatVersion MaxLimitedSizeStringsFormatVersion = 22;
    inline constexpr FormatVersion MaxStringRefIdFormatVersion = 23;
    inline constexpr FormatVersion MaxSavedGameCellNameAsRefIdFormatVersion = 24;
    inline constexpr FormatVersion MaxNameIsRefIdOnlyFormatVersion = 25;
    inline constexpr FormatVersion MaxUseEsmCellIdFormatVersion = 26;
    inline constexpr FormatVersion MaxActiveSpellSlotIndexFormatVersion = 27;
    inline constexpr FormatVersion MaxOldCountFormatVersion = 30;
    inline constexpr FormatVersion MaxActiveSpellTypeVersion = 31;
    inline constexpr FormatVersion MaxPlayerBeforeCellDataFormatVersion = 32;
    inline constexpr FormatVersion CurrentSaveGameFormatVersion = 34;

    inline constexpr FormatVersion MinSupportedSaveGameFormatVersion = 5;
    inline constexpr FormatVersion OpenMW0_49MinSaveGameFormatVersion = 5;
}

#endif
