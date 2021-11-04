#ifndef COMPONENTS_LUA_CONFIGURATION_H
#define COMPONENTS_LUA_CONFIGURATION_H

#include <map>
#include <optional>

#include <components/esm/luascripts.hpp>

namespace LuaUtil
{

    class ScriptsConfiguration
    {
    public:
        void init(ESM::LuaScriptsCfg);

        size_t size() const { return mScripts.size(); }
        const ESM::LuaScriptCfg& operator[](int id) const { return mScripts[id]; }

        std::optional<int> findId(std::string_view path) const;
        const std::vector<int>& getListByFlag(ESM::LuaScriptCfg::Flags type) const;

    private:
        std::vector<ESM::LuaScriptCfg> mScripts;
        std::map<std::string, int, std::less<>> mPathToIndex;
        std::map<ESM::LuaScriptCfg::Flags, std::vector<int>> mScriptsByFlag;
        static const std::vector<int> sEmpty;
    };

    // Parse ESM::LuaScriptsCfg from text and add to `cfg`.
    void parseOMWScripts(ESM::LuaScriptsCfg& cfg, std::string_view data);

    std::string scriptCfgToString(const ESM::LuaScriptCfg& script);

}

#endif // COMPONENTS_LUA_CONFIGURATION_H
