#ifndef OPENMW_ESM_LUASCRIPTS_H
#define OPENMW_ESM_LUASCRIPTS_H

#include <vector>
#include <string>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // LuaScriptCfg, LuaScriptsCfg are used in content files.

    struct LuaScriptCfg
    {
        using Flags = uint64_t;
        static constexpr Flags sGlobal = 1ull << 0;
        static constexpr Flags sCustom = 1ull << 1;  // local; can be attached/detached by a global script
        static constexpr Flags sPlayer = 1ull << 2;  // auto attach to players
        // auto attach for other classes:
        static constexpr Flags sActivator = 1ull << 3;
        static constexpr Flags sArmor = 1ull << 4;
        static constexpr Flags sBook = 1ull << 5;
        static constexpr Flags sClothing = 1ull << 6;
        static constexpr Flags sContainer = 1ull << 7;
        static constexpr Flags sCreature = 1ull << 8;
        static constexpr Flags sDoor = 1ull << 9;
        static constexpr Flags sIngredient = 1ull << 10;
        static constexpr Flags sLight = 1ull << 11;
        static constexpr Flags sMiscItem = 1ull << 12;
        static constexpr Flags sNPC = 1ull << 13;
        static constexpr Flags sPotion = 1ull << 14;
        static constexpr Flags sWeapon = 1ull << 15;

        std::string mScriptPath;
        std::string mInitializationData;  // Serialized Lua table. It is a binary data. Can contain '\0'.
        Flags mFlags;  // bitwise OR of Flags.
    };

    struct LuaScriptsCfg
    {
        std::vector<LuaScriptCfg> mScripts;

        void load(ESMReader &esm);
        void save(ESMWriter &esm) const;
    };

    // LuaTimer, LuaScript, LuaScripts are used in saved game files.
    // Storage structure for LuaUtil::ScriptsContainer. These are not top-level records.
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

        void load(ESMReader &esm);
        void save(ESMWriter &esm) const;
    };

    // Saves binary string `data` (can contain '\0') as LUAD record.
    void saveLuaBinaryData(ESM::ESMWriter& esm, const std::string& data);

    // Loads LUAD as binary string. If next subrecord is not LUAD, then returns an empty string.
    std::string loadLuaBinaryData(ESM::ESMReader& esm);

}

#endif

