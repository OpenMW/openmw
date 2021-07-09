#include "luascripts.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

// List of all records, that are related to Lua.
//
// Record:
// LUAM - MWLua::LuaManager
//
// Subrecords:
// LUAW - Start of MWLua::WorldView data
// LUAE - Start of MWLua::LocalEvent or MWLua::GlobalEvent (eventName)
// LUAS - Start LuaUtil::ScriptsContainer data (scriptName)
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
        esm.getExact(data.data(), data.size());
    }
    return data;
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
        if (!script.mData.empty())
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
