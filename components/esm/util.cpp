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
}
