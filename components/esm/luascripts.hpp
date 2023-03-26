#ifndef OPENMW_ESM_LUASCRIPTS_H
#define OPENMW_ESM_LUASCRIPTS_H

#include <cstdint>
#include <vector>
#include <string>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // LuaScriptCfg, LuaScriptsCfg are used in content files.

    struct LuaScriptCfg
    {
        using Flags = uint32_t;
        static constexpr Flags sGlobal = 1ull << 0;  // start as a global script
        static constexpr Flags sCustom = 1ull << 1;  // local; can be attached/detached by a global script
        static constexpr Flags sPlayer = 1ull << 2;  // auto attach to players

        static constexpr Flags sMerge = 1ull << 3;  // merge with configuration for this script from previous content files.

        std::string mScriptPath;  // VFS path to the script.
        std::string mInitializationData;  // Serialized Lua table. It is a binary data. Can contain '\0'.
        Flags mFlags;  // bitwise OR of Flags.

        // Auto attach as a local script to objects of specific types (i.e. Container, Door, Activator, etc.)
        std::vector<uint32_t> mTypes;  // values are ESM::RecNameInts

        // Auto attach as a local script to objects with specific recordIds (i.e. specific door type, or an unique NPC)
        struct PerRecordCfg
        {
            bool mAttach;  // true - attach, false - don't attach (overrides previous attach)
            std::string mRecordId;
            // Initialization data for this specific record. If empty than LuaScriptCfg::mInitializationData is used.
            std::string mInitializationData;
        };
        std::vector<PerRecordCfg> mRecords;

        // Auto attach as a local script to specific objects by their Refnums. The reference must be defined in the same
        // content file as this LuaScriptCfg or in one of its deps.
        struct PerRefCfg
        {
            bool mAttach;  // true - attach, false - don't attach (overrides previous attach)
            uint32_t mRefnumIndex;
            int32_t mRefnumContentFile;
            // Initialization data for this specific refnum. If empty than LuaScriptCfg::mInitializationData is used.
            std::string mInitializationData;
        };
        std::vector<PerRefCfg> mRefs;
    };

    struct LuaScriptsCfg
    {
        std::vector<LuaScriptCfg> mScripts;

        void load(ESMReader &esm);
        void adjustRefNums(const ESMReader &esm);

        void save(ESMWriter &esm) const;
    };

    // LuaTimer, LuaScript, LuaScripts are used in saved game files.
    // Storage structure for LuaUtil::ScriptsContainer. These are not top-level records.
    // Used either for global scripts or for local scripts on a specific object.

    struct LuaTimer
    {
        enum class Type : bool
        {
            SIMULATION_TIME = 0,
            GAME_TIME = 1,
        };

        Type mType;
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

