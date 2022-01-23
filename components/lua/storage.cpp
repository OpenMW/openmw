#include "storage.hpp"

#include <filesystem>
#include <fstream>

#include <components/debug/debuglog.hpp>

namespace sol
{
    template <>
    struct is_automagical<LuaUtil::LuaStorage::SectionMutableView> : std::false_type {};
    template <>
    struct is_automagical<LuaUtil::LuaStorage::SectionReadOnlyView> : std::false_type {};
}

namespace LuaUtil
{
    LuaStorage::Value LuaStorage::Section::sEmpty;

    sol::object LuaStorage::Value::getCopy(lua_State* L) const
    {
        return deserialize(L, mSerializedValue);
    }

    sol::object LuaStorage::Value::getReadOnly(lua_State* L) const
    {
        if (mReadOnlyValue == sol::nil && !mSerializedValue.empty())
            mReadOnlyValue = deserialize(L, mSerializedValue, nullptr, true);
        return mReadOnlyValue;
    }

    const LuaStorage::Value& LuaStorage::Section::get(std::string_view key) const
    {
        auto it = mValues.find(key);
        if (it != mValues.end())
            return it->second;
        else
            return sEmpty;
    }

    void LuaStorage::Section::set(std::string_view key, const sol::object& value)
    {
        mValues[std::string(key)] = Value(value);
        mChangeCounter++;
        if (mStorage->mListener)
            (*mStorage->mListener)(mSectionName, key, value);
    }

    bool LuaStorage::Section::wasChanged(int64_t& lastCheck)
    {
        bool res = lastCheck < mChangeCounter;
        lastCheck = mChangeCounter;
        return res;
    }

    sol::table LuaStorage::Section::asTable()
    {
        sol::table res(mStorage->mLua, sol::create);
        for (const auto& [k, v] : mValues)
            res[k] = v.getCopy(mStorage->mLua);
        return res;
    }

    void LuaStorage::initLuaBindings(lua_State* L)
    {
        sol::state_view lua(L);
        sol::usertype<SectionReadOnlyView> roView = lua.new_usertype<SectionReadOnlyView>("ReadOnlySection");
        sol::usertype<SectionMutableView> mutableView = lua.new_usertype<SectionMutableView>("MutableSection");
        roView["get"] = [](sol::this_state s, SectionReadOnlyView& section, std::string_view key)
        {
            return section.mSection->get(key).getReadOnly(s);
        };
        roView["getCopy"] = [](sol::this_state s, SectionReadOnlyView& section, std::string_view key)
        {
            return section.mSection->get(key).getCopy(s);
        };
        roView["wasChanged"] = [](SectionReadOnlyView& section) { return section.mSection->wasChanged(section.mLastCheck); };
        roView["asTable"] = [](SectionReadOnlyView& section) { return section.mSection->asTable(); };
        mutableView["get"] = [](sol::this_state s, SectionMutableView& section, std::string_view key)
        {
            return section.mSection->get(key).getReadOnly(s);
        };
        mutableView["getCopy"] = [](sol::this_state s, SectionMutableView& section, std::string_view key)
        {
            return section.mSection->get(key).getCopy(s);
        };
        mutableView["wasChanged"] = [](SectionMutableView& section) { return section.mSection->wasChanged(section.mLastCheck); };
        mutableView["asTable"] = [](SectionMutableView& section) { return section.mSection->asTable(); };
        mutableView["reset"] = [](SectionMutableView& section, sol::optional<sol::table> newValues)
        {
            section.mSection->mValues.clear();
            if (newValues)
            {
                for (const auto& [k, v] : *newValues)
                {
                    try
                    {
                        section.mSection->set(k.as<std::string_view>(), v);
                    }
                    catch (std::exception& e)
                    {
                        Log(Debug::Error) << "LuaUtil::LuaStorage::Section::reset(table): " << e.what();
                    }
                }
            }
            section.mSection->mChangeCounter++;
            section.mLastCheck = section.mSection->mChangeCounter;
        };
        mutableView["removeOnExit"] = [](SectionMutableView& section) { section.mSection->mPermanent = false; };
        mutableView["set"] = [](SectionMutableView& section, std::string_view key, const sol::object& value)
        {
            if (section.mLastCheck == section.mSection->mChangeCounter)
                section.mLastCheck++;
            section.mSection->set(key, value);
        };
    }

    void LuaStorage::clearTemporary()
    {
        auto it = mData.begin();
        while (it != mData.end())
        {
            if (!it->second->mPermanent)
            {
                it->second->mValues.clear();
                it = mData.erase(it);
            }
            else
                ++it;
        }
    }

    void LuaStorage::load(const std::string& path)
    {
        assert(mData.empty());  // Shouldn't be used before loading
        try
        {
            Log(Debug::Info) << "Loading Lua storage \"" << path << "\" (" << std::filesystem::file_size(path) << " bytes)";
            std::ifstream fin(path, std::fstream::binary);
            std::string serializedData((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
            sol::table data = deserialize(mLua, serializedData);
            for (const auto& [sectionName, sectionTable] : data)
            {
                const std::shared_ptr<Section>& section = getSection(sectionName.as<std::string_view>());
                for (const auto& [key, value] : sol::table(sectionTable))
                    section->set(key.as<std::string_view>(), value);
            }
        }
        catch (std::exception& e)
        {
            Log(Debug::Error) << "Can not read \"" << path << "\": " << e.what();
        }
    }

    void LuaStorage::save(const std::string& path) const
    {
        sol::table data(mLua, sol::create);
        for (const auto& [sectionName, section] : mData)
        {
            if (section->mPermanent)
                data[sectionName] = section->asTable();
        }
        std::string serializedData = serialize(data);
        Log(Debug::Info) << "Saving Lua storage \"" << path << "\" (" << serializedData.size() << " bytes)";
        std::ofstream fout(path, std::fstream::binary);
        fout.write(serializedData.data(), serializedData.size());
        fout.close();
    }

    const std::shared_ptr<LuaStorage::Section>& LuaStorage::getSection(std::string_view sectionName)
    {
        auto it = mData.find(sectionName);
        if (it != mData.end())
            return it->second;
        auto section = std::make_shared<Section>(this, std::string(sectionName));
        sectionName = section->mSectionName;
        auto [newIt, _] = mData.emplace(sectionName, std::move(section));
        return newIt->second;
    }

    sol::object LuaStorage::getReadOnlySection(std::string_view sectionName)
    {
        const std::shared_ptr<Section>& section = getSection(sectionName);
        return sol::make_object<SectionReadOnlyView>(mLua, SectionReadOnlyView{section, section->mChangeCounter});
    }

    sol::object LuaStorage::getMutableSection(std::string_view sectionName)
    {
        const std::shared_ptr<Section>& section = getSection(sectionName);
        return sol::make_object<SectionMutableView>(mLua, SectionMutableView{section, section->mChangeCounter});
    }

    sol::table LuaStorage::getAllSections()
    {
        sol::table res(mLua, sol::create);
        for (const auto& [sectionName, _] : mData)
            res[sectionName] = getMutableSection(sectionName);
        return res;
    }

}
