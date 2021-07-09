#ifndef OPENMW_ESM_LUASCRIPTS_H
#define OPENMW_ESM_LUASCRIPTS_H

#include <vector>
#include <string>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // Storage structure for LuaUtil::ScriptsContainer. This is not a top-level record.
    // Used either for global scripts or for local scripts on a specific object.

    struct LuaTimer
    {
        enum class TimeUnit : bool
        {
            SECONDS = 0,
            HOURS = 1,
        };

        TimeUnit mUnit;
        double mTime;
        std::string mCallbackName;
        std::string mCallbackArgument;  // Serialized Lua table. It is a binary data. Can contain '\0'.
    };

    struct LuaScript
    {
        std::string mScriptPath;
        std::string mData;  // Serialized Lua table. It is a binary data. Can contain '\0'.
        std::vector<LuaTimer> mTimers;
    };

    struct LuaScripts
    {
        std::vector<LuaScript> mScripts;

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };

    // Saves binary string `data` (can contain '\0') as record LUAD.
    void saveLuaBinaryData(ESM::ESMWriter& esm, const std::string& data);

    // Loads LUAD as binary string. If next subrecord is not LUAD, then returns an empty string.
    std::string loadLuaBinaryData(ESM::ESMReader& esm);

}

#endif

