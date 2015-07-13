#include "util.hpp"

#include <cstdint>

namespace ESM
{
    bool readDeleSubRecord(ESMReader &esm)
    {
        if (esm.isNextSub("DELE"))
        {
            esm.skipHSub();
            return true;
        }
        return false;
    }

    void writeDeleSubRecord(ESMWriter &esm)
    {
        esm.writeHNT("DELE", static_cast<int32_t>(0));
    }

    template <>
    bool isRecordDeleted<StartScript>(const StartScript &script)
    {
        return false;
    }

    template <>
    bool isRecordDeleted<Race>(const Race &race)
    {
        return false;
    }

    template <>
    bool isRecordDeleted<GameSetting>(const GameSetting &gmst)
    {
        return false;
    }

    template <>
    bool isRecordDeleted<Skill>(const Skill &skill)
    {
        return false;
    }

    template <>
    bool isRecordDeleted<MagicEffect>(const MagicEffect &mgef)
    {
        return false;
    }

    template <>
    bool isRecordDeleted<Pathgrid>(const Pathgrid &pgrd)
    {
        return false;
    }

    template <>
    bool isRecordDeleted<Land>(const Land &land)
    {
        return false;
    }

    template <>
    bool isRecordDeleted<DebugProfile>(const DebugProfile &profile)
    {
        return false;
    }

    template <>
    bool isRecordDeleted<Filter>(const Filter &filter)
    {
        return false;
    }
}
