#include "luascripts.hpp"

#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>

#include <components/lua/configuration.hpp>
#include <components/lua/luastateptr.hpp>
#include <components/lua/serialization.hpp>

// List of all records, that are related to Lua.
//
// Records:
// LUAL - LuaScriptsCfg - list of all scripts (in content files)
// LUAM - MWLua::LuaManager (in saves)
//
// Subrecords:
// LUAF - LuaScriptCfg::mFlags and ESM::RecNameInts list
// LUAW - Simulation time and last generated RefNum
// LUAE - Start of MWLua::LocalEvent or MWLua::GlobalEvent (eventName)
// LUAS - VFS path to a Lua script
// LUAD - Serialized Lua variable
// LUAT - MWLua::ScriptsContainer::Timer
// LUAC - Name of a timer callback (string)
// LUAR - Attach script to a specific record (LuaScriptCfg::PerRecordCfg)
// LUAI - Attach script to a specific instance (LuaScriptCfg::PerRefCfg)

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

static bool readBool(ESM::ESMReader& esm)
{
    char c;
    esm.getT<char>(c);
    return c != 0;
}

void ESM::LuaScriptsCfg::load(ESMReader& esm)
{
    while (esm.isNextSub("LUAS"))
    {
        mScripts.emplace_back();
        ESM::LuaScriptCfg& script = mScripts.back();
        script.mScriptPath = VFS::Path::Normalized(esm.getHString());

        esm.getSubNameIs("LUAF");
        esm.getSubHeader();
        if (esm.getSubSize() < 4 || (esm.getSubSize() % 4 != 0))
            esm.fail("Incorrect LUAF size");
        esm.getT(script.mFlags);
        script.mTypes.resize((esm.getSubSize() - 4) / 4);
        for (uint32_t& type : script.mTypes)
            esm.getT(type);

        script.mInitializationData = loadLuaBinaryData(esm);

        while (esm.isNextSub("LUAR"))
        {
            esm.getSubHeader();
            script.mRecords.emplace_back();
            ESM::LuaScriptCfg::PerRecordCfg& recordCfg = script.mRecords.back();
            recordCfg.mAttach = readBool(esm);
            recordCfg.mRecordId = esm.getRefId(esm.getSubSize() - 1);
            recordCfg.mInitializationData = loadLuaBinaryData(esm);
        }
        while (esm.isNextSub("LUAI"))
        {
            esm.getSubHeader();
            script.mRefs.emplace_back();
            ESM::LuaScriptCfg::PerRefCfg& refCfg = script.mRefs.back();
            refCfg.mAttach = readBool(esm);
            esm.getT<uint32_t>(refCfg.mRefnumIndex);
            esm.getT<int32_t>(refCfg.mRefnumContentFile);
            refCfg.mInitializationData = loadLuaBinaryData(esm);
        }
    }
}

void ESM::LuaScriptsCfg::adjustRefNums(const ESMReader& esm)
{
    auto adjustRefNumFn = [&esm](int contentFile) -> int {
        if (contentFile == 0)
            return esm.getIndex();
        else if (contentFile > 0 && contentFile <= static_cast<int>(esm.getParentFileIndices().size()))
            return esm.getParentFileIndices()[contentFile - 1];
        else
            throw std::runtime_error("Incorrect contentFile index");
    };

    LuaUtil::LuaStatePtr state(luaL_newstate());
    if (state == nullptr)
        throw std::runtime_error("Failed to create Lua runtime");

    LuaUtil::BasicSerializer serializer(adjustRefNumFn);

    auto adjustLuaData = [&](std::string& data) {
        if (data.empty())
            return;
        sol::object luaData = LuaUtil::deserialize(state.get(), data, &serializer);
        data = LuaUtil::serialize(luaData, &serializer);
    };

    for (LuaScriptCfg& script : mScripts)
    {
        adjustLuaData(script.mInitializationData);
        for (LuaScriptCfg::PerRecordCfg& recordCfg : script.mRecords)
            adjustLuaData(recordCfg.mInitializationData);
        for (LuaScriptCfg::PerRefCfg& refCfg : script.mRefs)
        {
            adjustLuaData(refCfg.mInitializationData);
            refCfg.mRefnumContentFile = adjustRefNumFn(refCfg.mRefnumContentFile);
        }
    }
}

void ESM::LuaScriptsCfg::save(ESMWriter& esm) const
{
    for (const LuaScriptCfg& script : mScripts)
    {
        esm.writeHNString("LUAS", script.mScriptPath);
        esm.startSubRecord("LUAF");
        esm.writeT<uint32_t>(script.mFlags);
        for (uint32_t type : script.mTypes)
            esm.writeT<uint32_t>(type);
        esm.endRecord("LUAF");
        saveLuaBinaryData(esm, script.mInitializationData);
        for (const LuaScriptCfg::PerRecordCfg& recordCfg : script.mRecords)
        {
            esm.startSubRecord("LUAR");
            esm.writeT<char>(recordCfg.mAttach ? 1 : 0);
            esm.writeHRefId(recordCfg.mRecordId);
            esm.endRecord("LUAR");
            saveLuaBinaryData(esm, recordCfg.mInitializationData);
        }
        for (const LuaScriptCfg::PerRefCfg& refCfg : script.mRefs)
        {
            esm.startSubRecord("LUAI");
            esm.writeT<char>(refCfg.mAttach ? 1 : 0);
            esm.writeT<uint32_t>(refCfg.mRefnumIndex);
            esm.writeT<int32_t>(refCfg.mRefnumContentFile);
            esm.endRecord("LUAI");
            saveLuaBinaryData(esm, refCfg.mInitializationData);
        }
    }
}

void ESM::LuaScripts::load(ESMReader& esm)
{
    while (esm.isNextSub("LUAS"))
    {
        int32_t id = -1;
        if (esm.getFormatVersion() <= ESM::MaxLuaScriptPathFormatVersion)
        {
            VFS::Path::Normalized name(esm.getHString());
            if (esm.mScriptsConfiguration)
                id = esm.mScriptsConfiguration->findId(name).value_or(-1);
        }
        else
            esm.getHT(id);
        std::string data = loadLuaBinaryData(esm);
        std::vector<LuaTimer> timers;
        while (esm.isNextSub("LUAT"))
        {
            esm.getSubHeader();
            LuaTimer timer;
            esm.getT(timer.mType);
            esm.getT(timer.mTime);
            timer.mCallbackName = esm.getHNString("LUAC");
            timer.mCallbackArgument = loadLuaBinaryData(esm);
            timers.push_back(std::move(timer));
        }
        mScripts.push_back({ id, std::move(data), std::move(timers) });
    }
}

void ESM::LuaScripts::save(ESMWriter& esm) const
{
    for (const LuaScript& script : mScripts)
    {
        esm.writeHNT("LUAS", script.mScriptId);
        saveLuaBinaryData(esm, script.mData);
        for (const LuaTimer& timer : script.mTimers)
        {
            esm.startSubRecord("LUAT");
            esm.writeT(timer.mType);
            esm.writeT(timer.mTime);
            esm.endRecord("LUAT");
            esm.writeHNString("LUAC", timer.mCallbackName);
            if (!timer.mCallbackArgument.empty())
                saveLuaBinaryData(esm, timer.mCallbackArgument);
        }
    }
}
