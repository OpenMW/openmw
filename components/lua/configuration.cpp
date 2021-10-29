#include "configuration.hpp"

#include <algorithm>
#include <bitset>
#include <cassert>
#include <sstream>

#include <components/misc/stringops.hpp>

namespace LuaUtil
{

    namespace
    {
        const std::map<std::string, ESM::LuaScriptCfg::Flags, std::less<>> flagsByName{
            {"GLOBAL", ESM::LuaScriptCfg::sGlobal},
            {"CUSTOM", ESM::LuaScriptCfg::sCustom},
            {"PLAYER", ESM::LuaScriptCfg::sPlayer},
            {"ACTIVATOR", ESM::LuaScriptCfg::sActivator},
            {"ARMOR", ESM::LuaScriptCfg::sArmor},
            {"BOOK", ESM::LuaScriptCfg::sBook},
            {"CLOTHING", ESM::LuaScriptCfg::sClothing},
            {"CONTAINER", ESM::LuaScriptCfg::sContainer},
            {"CREATURE", ESM::LuaScriptCfg::sCreature},
            {"DOOR", ESM::LuaScriptCfg::sDoor},
            {"INGREDIENT", ESM::LuaScriptCfg::sIngredient},
            {"LIGHT", ESM::LuaScriptCfg::sLight},
            {"MISC_ITEM", ESM::LuaScriptCfg::sMiscItem},
            {"NPC", ESM::LuaScriptCfg::sNPC},
            {"POTION", ESM::LuaScriptCfg::sPotion},
            {"WEAPON", ESM::LuaScriptCfg::sWeapon},
        };
    }

    const std::vector<int> ScriptsConfiguration::sEmpty;

    void ScriptsConfiguration::init(ESM::LuaScriptsCfg cfg)
    {
        mScripts.clear();
        mScriptsByFlag.clear();
        mPathToIndex.clear();

        // Find duplicates; only the last occurrence will be used.
        // Search for duplicates is case insensitive.
        std::vector<bool> skip(cfg.mScripts.size(), false);
        for (int i = cfg.mScripts.size() - 1; i >= 0; --i)
        {
            auto [_, inserted] = mPathToIndex.insert_or_assign(
                Misc::StringUtils::lowerCase(cfg.mScripts[i].mScriptPath), -1);
            if (!inserted || cfg.mScripts[i].mFlags == 0)
                skip[i] = true;
        }
        mPathToIndex.clear();
        int index = 0;
        for (size_t i = 0; i < cfg.mScripts.size(); ++i)
        {
            if (skip[i])
                continue;
            ESM::LuaScriptCfg& s = cfg.mScripts[i];
            mPathToIndex[s.mScriptPath] = index;  // Stored paths are case sensitive.
            ESM::LuaScriptCfg::Flags flags = s.mFlags;
            ESM::LuaScriptCfg::Flags flag = 1;
            while (flags != 0)
            {
                if (flags & flag)
                    mScriptsByFlag[flag].push_back(index);
                flags &= ~flag;
                flag = flag << 1;
            }
            mScripts.push_back(std::move(s));
            index++;
        }
    }

    std::optional<int> ScriptsConfiguration::findId(std::string_view path) const
    {
        auto it = mPathToIndex.find(path);
        if (it != mPathToIndex.end())
            return it->second;
        else
            return std::nullopt;
    }

    const std::vector<int>& ScriptsConfiguration::getListByFlag(ESM::LuaScriptCfg::Flags type) const
    {
        assert(std::bitset<64>(type).count() <= 1);
        auto it = mScriptsByFlag.find(type);
        if (it != mScriptsByFlag.end())
            return it->second;
        else
            return sEmpty;
    }

    void parseOMWScripts(ESM::LuaScriptsCfg& cfg, std::string_view data)
    {
        while (!data.empty())
        {
            // Get next line
            std::string_view line = data.substr(0, data.find('\n'));
            data = data.substr(std::min(line.size() + 1, data.size()));
            if (!line.empty() && line.back() == '\r')
                line = line.substr(0, line.size() - 1);

            while (!line.empty() && std::isspace(line[0]))
                line = line.substr(1);
            if (line.empty() || line[0] == '#')  // Skip empty lines and comments
                continue;
            while (!line.empty() && std::isspace(line.back()))
                line = line.substr(0, line.size() - 1);

            if (!Misc::StringUtils::ciEndsWith(line, ".lua"))
                throw std::runtime_error(Misc::StringUtils::format(
                    "Lua script should have suffix '.lua', got: %s", std::string(line.substr(0, 300))));

            // Split flags and script path
            size_t semicolonPos = line.find(':');
            if (semicolonPos == std::string::npos)
                throw std::runtime_error(Misc::StringUtils::format("No flags found in: %s", std::string(line)));
            std::string_view flagsStr = line.substr(0, semicolonPos);
            std::string_view scriptPath = line.substr(semicolonPos + 1);
            while (std::isspace(scriptPath[0]))
                scriptPath = scriptPath.substr(1);

            // Parse flags
            ESM::LuaScriptCfg::Flags flags = 0;
            size_t flagsPos = 0;
            while (true)
            {
                while (flagsPos < flagsStr.size() && (std::isspace(flagsStr[flagsPos]) || flagsStr[flagsPos] == ','))
                    flagsPos++;
                size_t startPos = flagsPos;
                while (flagsPos < flagsStr.size() && !std::isspace(flagsStr[flagsPos]) && flagsStr[flagsPos] != ',')
                    flagsPos++;
                if (startPos == flagsPos)
                    break;
                std::string_view flagName = flagsStr.substr(startPos, flagsPos - startPos);
                auto it = flagsByName.find(flagName);
                if (it != flagsByName.end())
                    flags |= it->second;
                else
                    throw std::runtime_error(Misc::StringUtils::format("Unknown flag '%s' in: %s",
                                                                       std::string(flagName), std::string(line)));
            }
            if ((flags & ESM::LuaScriptCfg::sGlobal) && flags != ESM::LuaScriptCfg::sGlobal)
                throw std::runtime_error("Global script can not have local flags");

            cfg.mScripts.push_back(ESM::LuaScriptCfg{std::string(scriptPath), "", flags});
        }
    }

    std::string scriptCfgToString(const ESM::LuaScriptCfg& script)
    {
        std::stringstream ss;
        for (const auto& [flagName, flag] : flagsByName)
        {
            if (script.mFlags & flag)
                ss << flagName << " ";
        }
        ss << ": " << script.mScriptPath;
        if (!script.mInitializationData.empty())
            ss << " (with data, " << script.mInitializationData.size() << " bytes)";
        return ss.str();
    }

}
