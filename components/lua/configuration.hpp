#ifndef COMPONENTS_LUA_CONFIGURATION_H
#define COMPONENTS_LUA_CONFIGURATION_H

#include <map>
#include <optional>

#include "components/esm/luascripts.hpp"
#include "components/esm3/cellref.hpp"

namespace LuaUtil
{
    using ScriptIdsWithInitializationData = std::map<int, std::string_view>;

    class ScriptsConfiguration
    {
    public:
        void init(ESM::LuaScriptsCfg);

        size_t size() const { return mScripts.size(); }
        const ESM::LuaScriptCfg& operator[](int id) const { return mScripts[id]; }

        std::optional<int> findId(std::string_view path) const;
        bool isCustomScript(int id) const { return mScripts[id].mFlags & ESM::LuaScriptCfg::sCustom; }

        ScriptIdsWithInitializationData getGlobalConf() const { return getConfByFlag(ESM::LuaScriptCfg::sGlobal); }
        ScriptIdsWithInitializationData getPlayerConf() const { return getConfByFlag(ESM::LuaScriptCfg::sPlayer); }
        ScriptIdsWithInitializationData getLocalConf(uint32_t type, std::string_view recordId, ESM::RefNum refnum) const;

    private:
        ScriptIdsWithInitializationData getConfByFlag(ESM::LuaScriptCfg::Flags flag) const;

        std::vector<ESM::LuaScriptCfg> mScripts;
        std::map<std::string, int, std::less<>> mPathToIndex;

        struct DetailedConf
        {
            int mScriptId;
            bool mAttach;
            std::string_view mInitializationData;
        };
        std::map<uint32_t, std::vector<int>> mScriptsPerType;
        std::map<std::string, std::vector<DetailedConf>, std::less<>> mScriptsPerRecordId;
        std::map<ESM::RefNum, std::vector<DetailedConf>> mScriptsPerRefNum;
    };

    // Parse ESM::LuaScriptsCfg from text and add to `cfg`.
    void parseOMWScripts(ESM::LuaScriptsCfg& cfg, std::string_view data);

    std::string scriptCfgToString(const ESM::LuaScriptCfg& script);

}

#endif // COMPONENTS_LUA_CONFIGURATION_H
