#include "configuration.hpp"

#include <algorithm>
#include <bitset>
#include <cassert>
#include <format>
#include <sstream>

#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/lower.hpp>

namespace LuaUtil
{

    namespace
    {
        const std::map<std::string, ESM::LuaScriptCfg::Flags, std::less<>> flagsByName{
            { "GLOBAL", ESM::LuaScriptCfg::sGlobal },
            { "CUSTOM", ESM::LuaScriptCfg::sCustom },
            { "PLAYER", ESM::LuaScriptCfg::sPlayer },
            { "MENU", ESM::LuaScriptCfg::sMenu },
        };

        const std::map<std::string, ESM::RecNameInts, std::less<>> typeTagsByName{
            { "ACTIVATOR", ESM::REC_ACTI },
            { "ARMOR", ESM::REC_ARMO },
            { "BOOK", ESM::REC_BOOK },
            { "CLOTHING", ESM::REC_CLOT },
            { "CONTAINER", ESM::REC_CONT },
            { "CREATURE", ESM::REC_CREA },
            { "DOOR", ESM::REC_DOOR },
            { "INGREDIENT", ESM::REC_INGR },
            { "LIGHT", ESM::REC_LIGH },
            { "MISC_ITEM", ESM::REC_MISC },
            { "NPC", ESM::REC_NPC_ },
            { "POTION", ESM::REC_ALCH },
            { "WEAPON", ESM::REC_WEAP },
            { "APPARATUS", ESM::REC_APPA },
            { "LOCKPICK", ESM::REC_LOCK },
            { "PROBE", ESM::REC_PROB },
            { "REPAIR", ESM::REC_REPA },
        };

        bool isSpace(char c)
        {
            return std::isspace(static_cast<unsigned char>(c));
        }
    }

    void ScriptsConfiguration::init(ESM::LuaScriptsCfg cfg, bool remap)
    {
        std::vector<VFS::Path::Normalized> oldPaths;
        if (remap)
        {
            for (ESM::LuaScriptCfg& script : mScripts)
                oldPaths.emplace_back(std::move(script.mScriptPath));
        }
        mScripts.clear();
        mPathToIndex.clear();

        // Find duplicates; only the last occurrence will be used (unless `sMerge` flag is used).
        // Search for duplicates is case insensitive.
        std::vector<bool> skip(cfg.mScripts.size(), false);
        for (int i = 0; i < static_cast<int>(cfg.mScripts.size()); ++i)
        {
            const ESM::LuaScriptCfg& script = cfg.mScripts[i];
            bool global = script.mFlags & ESM::LuaScriptCfg::sGlobal;
            if (global && (script.mFlags & ~ESM::LuaScriptCfg::sMerge) != ESM::LuaScriptCfg::sGlobal)
                throw std::runtime_error(
                    std::string("Global script can not have local flags: ") + script.mScriptPath.value());
            if (global && (!script.mTypes.empty() || !script.mRecords.empty() || !script.mRefs.empty()))
                throw std::runtime_error(std::string("Global script can not have per-type and per-object configuration")
                    + script.mScriptPath.value());
            auto [it, inserted] = mPathToIndex.emplace(script.mScriptPath, i);
            if (inserted)
                continue;
            ESM::LuaScriptCfg& oldScript = cfg.mScripts[it->second];
            if (global != bool(oldScript.mFlags & ESM::LuaScriptCfg::sGlobal))
                throw std::runtime_error(std::string("Flags mismatch for ") + script.mScriptPath.value());
            if (script.mFlags & ESM::LuaScriptCfg::sMerge)
            {
                oldScript.mFlags |= (script.mFlags & ~ESM::LuaScriptCfg::sMerge);
                if (!script.mInitializationData.empty())
                    oldScript.mInitializationData = script.mInitializationData;
                oldScript.mTypes.insert(oldScript.mTypes.end(), script.mTypes.begin(), script.mTypes.end());
                oldScript.mRecords.insert(oldScript.mRecords.end(), script.mRecords.begin(), script.mRecords.end());
                oldScript.mRefs.insert(oldScript.mRefs.end(), script.mRefs.begin(), script.mRefs.end());
                skip[i] = true;
            }
            else
                skip[it->second] = true;
        }

        // Filter duplicates
        for (size_t i = 0; i < cfg.mScripts.size(); ++i)
        {
            if (!skip[i])
                mScripts.push_back(std::move(cfg.mScripts[i]));
        }

        // Initialize mappings
        mPathToIndex.clear();
        mScriptsPerType.clear();
        mScriptsPerRecordId.clear();
        mScriptsPerRefNum.clear();
        for (int i = 0; i < static_cast<int>(mScripts.size()); ++i)
        {
            const ESM::LuaScriptCfg& s = mScripts[i];
            mPathToIndex[s.mScriptPath] = i; // Stored paths are case sensitive.
            for (uint32_t t : s.mTypes)
                mScriptsPerType[t].push_back(i);
            for (const ESM::LuaScriptCfg::PerRecordCfg& r : s.mRecords)
            {
                std::string_view data = r.mInitializationData.empty() ? s.mInitializationData : r.mInitializationData;
                mScriptsPerRecordId[r.mRecordId].push_back(DetailedConf{ i, r.mAttach, data });
            }
            for (const ESM::LuaScriptCfg::PerRefCfg& r : s.mRefs)
            {
                std::string_view data = r.mInitializationData.empty() ? s.mInitializationData : r.mInitializationData;
                mScriptsPerRefNum[ESM::RefNum{ r.mRefnumIndex, r.mRefnumContentFile }].push_back(
                    DetailedConf{ i, r.mAttach, data });
            }
        }

        if (remap)
        {
            mScriptIdMapping.clear();
            for (size_t i = 0; i < oldPaths.size(); ++i)
            {
                if (std::optional<int> id = findId(oldPaths[i]))
                    mScriptIdMapping[static_cast<int>(i)] = *id;
            }
        }
    }

    std::optional<int> ScriptsConfiguration::findId(VFS::Path::NormalizedView path) const
    {
        auto it = mPathToIndex.find(path);
        if (it != mPathToIndex.end())
            return it->second;
        else
            return std::nullopt;
    }

    ScriptIdsWithInitializationData ScriptsConfiguration::getConfByFlag(ESM::LuaScriptCfg::Flags flag) const
    {
        ScriptIdsWithInitializationData res;
        for (size_t id = 0; id < mScripts.size(); ++id)
        {
            const ESM::LuaScriptCfg& script = mScripts[id];
            if (script.mFlags & flag)
                res[static_cast<int>(id)] = script.mInitializationData;
        }
        return res;
    }

    ScriptIdsWithInitializationData ScriptsConfiguration::getLocalConf(
        uint32_t type, const ESM::RefId& recordId, ESM::RefNum refnum) const
    {
        ScriptIdsWithInitializationData res;
        auto typeIt = mScriptsPerType.find(type);
        if (typeIt != mScriptsPerType.end())
            for (int scriptId : typeIt->second)
                res[scriptId] = mScripts[scriptId].mInitializationData;
        auto recordIt = mScriptsPerRecordId.find(recordId);
        if (recordIt != mScriptsPerRecordId.end())
        {
            for (const DetailedConf& d : recordIt->second)
            {
                if (d.mAttach)
                    res[d.mScriptId] = d.mInitializationData;
                else
                    res.erase(d.mScriptId);
            }
        }
        if (!refnum.hasContentFile())
            return res;
        auto refIt = mScriptsPerRefNum.find(refnum);
        if (refIt == mScriptsPerRefNum.end())
            return res;
        for (const DetailedConf& d : refIt->second)
        {
            if (d.mAttach)
                res[d.mScriptId] = d.mInitializationData;
            else
                res.erase(d.mScriptId);
        }
        return res;
    }

    void ScriptsConfiguration::read(ESM::ESMReader& reader)
    {
        reader.mScriptsConfiguration = this;
        mScriptIdMapping.clear();
        int index = 0;
        while (reader.isNextSub("LUAP"))
        {
            VFS::Path::Normalized path(reader.getHString());
            if (std::optional<int> id = findId(path))
                mScriptIdMapping[index] = *id;
            ++index;
        }
    }

    void ScriptsConfiguration::write(ESM::ESMWriter& writer) const
    {
        for (const ESM::LuaScriptCfg& script : mScripts)
            writer.writeHNString("LUAP", script.mScriptPath);
    }

    std::optional<int> ScriptsConfiguration::mapId(int savedId) const
    {
        if (mScriptIdMapping.empty())
        {
            if (savedId == -1)
                return {};
            return savedId;
        }
        auto it = mScriptIdMapping.find(savedId);
        if (it == mScriptIdMapping.end())
            return {};
        return it->second;
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

            while (!line.empty() && isSpace(line[0]))
                line = line.substr(1);
            if (line.empty() || line[0] == '#') // Skip empty lines and comments
                continue;
            while (!line.empty() && isSpace(line.back()))
                line = line.substr(0, line.size() - 1);

            if (!Misc::StringUtils::ciEndsWith(line, ".lua"))
                throw std::runtime_error(
                    std::format("Lua script should have suffix '.lua', got: {}", line.substr(0, 300)));

            // Split tags and script path
            size_t semicolonPos = line.find(':');
            if (semicolonPos == std::string_view::npos)
                throw std::runtime_error(std::format("No flags found in: {}", line));
            std::string_view tagsStr = line.substr(0, semicolonPos);
            std::string_view scriptPath = line.substr(semicolonPos + 1);
            while (!scriptPath.empty() && isSpace(scriptPath[0]))
                scriptPath = scriptPath.substr(1);

            ESM::LuaScriptCfg& script = cfg.mScripts.emplace_back();
            script.mScriptPath = VFS::Path::Normalized(scriptPath);
            script.mFlags = 0;

            // Parse tags
            size_t tagsPos = 0;
            while (true)
            {
                while (tagsPos < tagsStr.size() && (isSpace(tagsStr[tagsPos]) || tagsStr[tagsPos] == ','))
                    tagsPos++;
                size_t startPos = tagsPos;
                while (tagsPos < tagsStr.size() && !isSpace(tagsStr[tagsPos]) && tagsStr[tagsPos] != ',')
                    tagsPos++;
                if (startPos == tagsPos)
                    break;
                std::string_view tagName = tagsStr.substr(startPos, tagsPos - startPos);
                auto it = flagsByName.find(tagName);
                auto typesIt = typeTagsByName.find(tagName);
                if (it != flagsByName.end())
                    script.mFlags |= it->second;
                else if (typesIt != typeTagsByName.end())
                    script.mTypes.push_back(typesIt->second);
                else
                    throw std::runtime_error(std::format("Unknown tag '{}' in: {}", tagName, line));
            }
        }
    }

    std::string scriptCfgToString(const ESM::LuaScriptCfg& script)
    {
        std::stringstream ss;
        if (script.mFlags & ESM::LuaScriptCfg::sMerge)
            ss << "+ ";
        for (const auto& [flagName, flag] : flagsByName)
        {
            if (script.mFlags & flag)
                ss << flagName << " ";
        }
        for (uint32_t type : script.mTypes)
        {
            for (const auto& [tagName, t] : typeTagsByName)
            {
                if (type == t)
                    ss << tagName << " ";
            }
        }
        ss << ": " << script.mScriptPath;
        if (!script.mInitializationData.empty())
            ss << " ; data " << script.mInitializationData.size() << " bytes";
        if (!script.mRecords.empty())
            ss << " ; " << script.mRecords.size() << " records";
        if (!script.mRefs.empty())
            ss << " ; " << script.mRefs.size() << " objects";
        return ss.str();
    }

}
