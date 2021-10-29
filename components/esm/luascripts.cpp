#include "luascripts.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

// List of all records, that are related to Lua.
//
// Records:
// LUAL - LuaScriptsCfg - list of all scripts (in content files)
// LUAM - MWLua::LuaManager (in saves)
//
// Subrecords:
// LUAF - LuaScriptCfg::mFlags
// LUAW - Start of MWLua::WorldView data
// LUAE - Start of MWLua::LocalEvent or MWLua::GlobalEvent (eventName)
// LUAS - VFS path to a Lua script
// LUAD - Serialized Lua variable
// LUAT - MWLua::ScriptsContainer::Timer
// LUAC - Name of a timer callback (string)

void ESM::saveLuaBinaryData(ESMWriter& esm, const std::string& data)
{
    if (data.empty())
        return;
    esm.startSubRecord("LUAD");
    esm.write(data.data(), data.size());
    esm.endRecord("LUAD");
}

std::string ESM::loadLuaBinaryData(ESMReader& esm)
{
    std::string data;
    if (esm.isNextSub("LUAD"))
    {
        esm.getSubHeader();
        data.resize(esm.getSubSize());
        esm.getExact(data.data(), static_cast<int>(data.size()));
    }
    return data;
}

void ESM::LuaScriptsCfg::load(ESMReader& esm)
{
    while (esm.isNextSub("LUAS"))
    {
        std::string name = esm.getHString();
        uint64_t flags;
        esm.getHNT(flags, "LUAF");
        std::string data = loadLuaBinaryData(esm);
        mScripts.push_back({std::move(name), std::move(data), flags});
    }
}

void ESM::LuaScriptsCfg::save(ESMWriter& esm) const
{
    for (const LuaScriptCfg& script : mScripts)
    {
        esm.writeHNString("LUAS", script.mScriptPath);
        esm.writeHNT("LUAF", script.mFlags);
        saveLuaBinaryData(esm, script.mInitializationData);
    }
}

void ESM::LuaScripts::load(ESMReader& esm)
{
    while (esm.isNextSub("LUAS"))
    {
        std::string name = esm.getHString();
        std::string data = loadLuaBinaryData(esm);
        std::vector<LuaTimer> timers;
        while (esm.isNextSub("LUAT"))
        {
            esm.getSubHeader();
            LuaTimer timer;
            esm.getT(timer.mUnit);
            esm.getT(timer.mTime);
            timer.mCallbackName = esm.getHNString("LUAC");
            timer.mCallbackArgument = loadLuaBinaryData(esm);
            timers.push_back(std::move(timer));
        }
        mScripts.push_back({std::move(name), std::move(data), std::move(timers)});
    }
}

void ESM::LuaScripts::save(ESMWriter& esm) const
{
    for (const LuaScript& script : mScripts)
    {
        esm.writeHNString("LUAS", script.mScriptPath);
        saveLuaBinaryData(esm, script.mData);
        for (const LuaTimer& timer : script.mTimers)
        {
            esm.startSubRecord("LUAT");
            esm.writeT(timer.mUnit);
            esm.writeT(timer.mTime);
            esm.endRecord("LUAT");
            esm.writeHNString("LUAC", timer.mCallbackName);
            if (!timer.mCallbackArgument.empty())
                saveLuaBinaryData(esm, timer.mCallbackArgument);
            
        }
    }
}
