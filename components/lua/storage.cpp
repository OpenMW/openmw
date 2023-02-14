#include "storage.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>

#include <components/debug/debuglog.hpp>

namespace sol
{
    template <>
    struct is_automagical<LuaUtil::LuaStorage::SectionView> : std::false_type {};
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

    void LuaStorage::Section::runCallbacks(sol::optional<std::string_view> changedKey)
    {
        mStorage->mRunningCallbacks.insert(this);
        mCallbacks.erase(std::remove_if(mCallbacks.begin(), mCallbacks.end(), [&](const Callback& callback)
        {
            bool valid = callback.isValid();
            if (valid)
                callback.tryCall(mSectionName, changedKey);
            return !valid;
        }), mCallbacks.end());
        mStorage->mRunningCallbacks.erase(this);
    }

    void LuaStorage::Section::throwIfCallbackRecursionIsTooDeep()
    {
        if (mStorage->mRunningCallbacks.count(this) > 0)
            throw std::runtime_error("Storage handler shouldn't change the storage section it handles (leads to an infinite recursion)");
        if (mStorage->mRunningCallbacks.size() > 10)
            throw std::runtime_error("Too many subscribe callbacks triggering in a chain, likely an infinite recursion");
    }

    void LuaStorage::Section::set(std::string_view key, const sol::object& value)
    {
        throwIfCallbackRecursionIsTooDeep();
        if (value != sol::nil)
            mValues[std::string(key)] = Value(value);
        else
        {
            auto it = mValues.find(key);
            if (it != mValues.end())
                mValues.erase(it);
        }
        if (mStorage->mListener)
            mStorage->mListener->valueChanged(mSectionName, key, value);
        runCallbacks(key);
    }

    void LuaStorage::Section::setAll(const sol::optional<sol::table>& values)
    {
        throwIfCallbackRecursionIsTooDeep();
        mValues.clear();
        if (values)
        {
            for (const auto& [k, v] : *values)
                mValues[k.as<std::string>()] = Value(v);
        }
        if (mStorage->mListener)
            mStorage->mListener->sectionReplaced(mSectionName, values);
        runCallbacks(sol::nullopt);
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
        sol::usertype<SectionView> sview = lua.new_usertype<SectionView>("Section");
        sview["get"] = [](sol::this_state s, const SectionView& section, std::string_view key)
        {
            return section.mSection->get(key).getReadOnly(s);
        };
        sview["getCopy"] = [](sol::this_state s, const SectionView& section, std::string_view key)
        {
            return section.mSection->get(key).getCopy(s);
        };
        sview["asTable"] = [](const SectionView& section) { return section.mSection->asTable(); };
        sview["subscribe"] = [](const SectionView& section, const sol::table& callback) {
            std::vector<Callback>& callbacks = section.mSection->mCallbacks;
            if (!callbacks.empty() && callbacks.size() == callbacks.capacity())
            {
                callbacks.erase(std::remove_if(callbacks.begin(), callbacks.end(),
                                               [&](const Callback& c) { return !c.isValid(); }),
                                callbacks.end());
            }
            callbacks.push_back(Callback::fromLua(callback));
        };
        sview["reset"] = [](const SectionView& section, const sol::optional<sol::table>& newValues)
        {
            if (section.mReadOnly)
                throw std::runtime_error("Access to storage is read only");
            section.mSection->setAll(newValues);
        };
        sview["removeOnExit"] = [](const SectionView& section)
        {
            if (section.mReadOnly)
                throw std::runtime_error("Access to storage is read only");
            section.mSection->mPermanent = false;
        };
        sview["set"] = [](const SectionView& section, std::string_view key, const sol::object& value)
        {
            if (section.mReadOnly)
                throw std::runtime_error("Access to storage is read only");
            section.mSection->set(key, value);
        };
    }

    void LuaStorage::clearTemporaryAndRemoveCallbacks()
    {
        auto it = mData.begin();
        while (it != mData.end())
        {
            it->second->mCallbacks.clear();
            if (!it->second->mPermanent)
            {
                it->second->mValues.clear();
                it = mData.erase(it);
            }
            else
                ++it;
        }
    }

    void LuaStorage::load(const boost::filesystem::path& path)
    {
        assert(mData.empty());  // Shouldn't be used before loading
        try
        {
            Log(Debug::Info) << "Loading Lua storage \"" << path << "\" (" << boost::filesystem::file_size(path) << " bytes)";
            boost::filesystem::ifstream fin(path, boost::filesystem::fstream::binary);
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

    void LuaStorage::save(const boost::filesystem::path& path) const
    {
        sol::table data(mLua, sol::create);
        for (const auto& [sectionName, section] : mData)
        {
            if (section->mPermanent && !section->mValues.empty())
                data[sectionName] = section->asTable();
        }
        std::string serializedData = serialize(data);
        Log(Debug::Info) << "Saving Lua storage \"" << path << "\" (" << serializedData.size() << " bytes)";
        boost::filesystem::ofstream fout(path, boost::filesystem::ofstream::binary);
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

    sol::object LuaStorage::getSection(std::string_view sectionName, bool readOnly)
    {
        const std::shared_ptr<Section>& section = getSection(sectionName);
        return sol::make_object<SectionView>(mLua, SectionView{section, readOnly});
    }

    sol::table LuaStorage::getAllSections(bool readOnly)
    {
        sol::table res(mLua, sol::create);
        for (const auto& [sectionName, _] : mData)
            res[sectionName] = getSection(sectionName, readOnly);
        return res;
    }

}
