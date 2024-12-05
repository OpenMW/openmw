#include "storage.hpp"

#include <filesystem>
#include <fstream>

#include <components/debug/debuglog.hpp>

#include "luastate.hpp"

namespace sol
{
    template <>
    struct is_automagical<LuaUtil::LuaStorage::SectionView> : std::false_type
    {
    };
}

namespace LuaUtil
{
    LuaStorage::Value LuaStorage::Section::sEmpty;

    void LuaStorage::registerLifeTime(LuaUtil::LuaView& view, sol::table& res)
    {
        res["LIFE_TIME"] = LuaUtil::makeStrictReadOnly(tableFromPairs<std::string_view, Section::LifeTime>(view.sol(),
            {
                { "Persistent", Section::LifeTime::Persistent },
                { "GameSession", Section::LifeTime::GameSession },
                { "Temporary", Section::LifeTime::Temporary },
            }));
    }

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
        checkIfActive();
        auto it = mValues.find(key);
        if (it != mValues.end())
            return it->second;
        else
            return sEmpty;
    }

    void LuaStorage::Section::runCallbacks(sol::optional<std::string_view> changedKey)
    {
        mStorage->mRunningCallbacks.insert(this);
        mCallbacks.erase(std::remove_if(mCallbacks.begin(), mCallbacks.end(),
                             [&](const Callback& callback) {
                                 bool valid = callback.isValid();
                                 if (valid)
                                     callback.tryCall(mSectionName, changedKey);
                                 return !valid;
                             }),
            mCallbacks.end());
        mMenuScriptsCallbacks.erase(std::remove_if(mMenuScriptsCallbacks.begin(), mMenuScriptsCallbacks.end(),
                                        [&](const Callback& callback) {
                                            bool valid = callback.isValid();
                                            if (valid)
                                                callback.tryCall(mSectionName, changedKey);
                                            return !valid;
                                        }),
            mMenuScriptsCallbacks.end());
        mStorage->mRunningCallbacks.erase(this);
    }

    void LuaStorage::Section::throwIfCallbackRecursionIsTooDeep()
    {
        if (mStorage->mRunningCallbacks.count(this) > 0)
            throw std::runtime_error(
                "Storage handler shouldn't change the storage section it handles (leads to an infinite recursion)");
        if (mStorage->mRunningCallbacks.size() > 10)
            throw std::runtime_error(
                "Too many subscribe callbacks triggering in a chain, likely an infinite recursion");
    }

    void LuaStorage::Section::set(std::string_view key, const sol::object& value)
    {
        checkIfActive();
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
        checkIfActive();
        throwIfCallbackRecursionIsTooDeep();
        mValues.clear();
        if (values)
        {
            for (const auto& [k, v] : *values)
                mValues[cast<std::string>(k)] = Value(v);
        }
        if (mStorage->mListener)
            mStorage->mListener->sectionReplaced(mSectionName, values);
        runCallbacks(sol::nullopt);
    }

    sol::table LuaStorage::Section::asTable(lua_State* L)
    {
        checkIfActive();
        sol::table res(L, sol::create);
        for (const auto& [k, v] : mValues)
            res[k] = v.getCopy(L);
        return res;
    }

    void LuaStorage::initLuaBindings(LuaUtil::LuaView& view)
    {
        sol::usertype<SectionView> sview = view.sol().new_usertype<SectionView>("Section");
        sview["get"] = [](sol::this_state s, const SectionView& section, std::string_view key) {
            return section.mSection->get(key).getReadOnly(s);
        };
        sview["getCopy"] = [](sol::this_state s, const SectionView& section, std::string_view key) {
            return section.mSection->get(key).getCopy(s);
        };
        sview["asTable"]
            = [](sol::this_state lua, const SectionView& section) { return section.mSection->asTable(lua); };
        sview["subscribe"] = [](const SectionView& section, const sol::table& callback) {
            std::vector<Callback>& callbacks
                = section.mForMenuScripts ? section.mSection->mMenuScriptsCallbacks : section.mSection->mCallbacks;
            if (!callbacks.empty() && callbacks.size() == callbacks.capacity())
            {
                callbacks.erase(
                    std::remove_if(callbacks.begin(), callbacks.end(), [&](const Callback& c) { return !c.isValid(); }),
                    callbacks.end());
            }
            callbacks.push_back(Callback::fromLua(callback));
        };
        sview["reset"] = [](const SectionView& section, const sol::optional<sol::table>& newValues) {
            if (section.mReadOnly)
                throw std::runtime_error("Access to storage is read only");
            section.mSection->setAll(newValues);
        };
        sview["removeOnExit"] = [](const SectionView& section) {
            if (section.mReadOnly)
                throw std::runtime_error("Access to storage is read only");
            section.mSection->mLifeTime = Section::Temporary;
        };
        sview["setLifeTime"] = [](const SectionView& section, Section::LifeTime lifeTime) {
            if (section.mReadOnly)
                throw std::runtime_error("Access to storage is read only");
            section.mSection->mLifeTime = lifeTime;
        };
        sview["set"] = [](const SectionView& section, std::string_view key, const sol::object& value) {
            if (section.mReadOnly)
                throw std::runtime_error("Access to storage is read only");
            section.mSection->set(key, value);
        };
    }

    sol::table LuaStorage::initGlobalPackage(LuaUtil::LuaView& view, LuaStorage* globalStorage)
    {
        sol::table res(view.sol(), sol::create);
        registerLifeTime(view, res);

        res["globalSection"] = [globalStorage](sol::this_state lua, std::string_view section) {
            return globalStorage->getMutableSection(lua, section);
        };
        res["allGlobalSections"] = [globalStorage](sol::this_state lua) { return globalStorage->getAllSections(lua); };
        return LuaUtil::makeReadOnly(res);
    }

    sol::table LuaStorage::initLocalPackage(LuaUtil::LuaView& view, LuaStorage* globalStorage)
    {
        sol::table res(view.sol(), sol::create);
        registerLifeTime(view, res);

        res["globalSection"] = [globalStorage](sol::this_state lua, std::string_view section) {
            return globalStorage->getReadOnlySection(lua, section);
        };
        return LuaUtil::makeReadOnly(res);
    }

    sol::table LuaStorage::initPlayerPackage(
        LuaUtil::LuaView& view, LuaStorage* globalStorage, LuaStorage* playerStorage)
    {
        sol::table res(view.sol(), sol::create);
        registerLifeTime(view, res);

        res["globalSection"] = [globalStorage](sol::this_state lua, std::string_view section) {
            return globalStorage->getReadOnlySection(lua, section);
        };
        res["playerSection"] = [playerStorage](sol::this_state lua, std::string_view section) {
            return playerStorage->getMutableSection(lua, section);
        };
        res["allPlayerSections"] = [playerStorage](sol::this_state lua) { return playerStorage->getAllSections(lua); };
        return LuaUtil::makeReadOnly(res);
    }

    sol::table LuaStorage::initMenuPackage(LuaUtil::LuaView& view, LuaStorage* globalStorage, LuaStorage* playerStorage)
    {
        sol::table res(view.sol(), sol::create);
        registerLifeTime(view, res);

        res["playerSection"] = [playerStorage](sol::this_state lua, std::string_view section) {
            return playerStorage->getMutableSection(lua, section, /*forMenuScripts=*/true);
        };
        res["globalSection"] = [globalStorage](sol::this_state lua, std::string_view section) {
            return globalStorage->getReadOnlySection(lua, section);
        };
        res["allPlayerSections"] = [playerStorage](sol::this_state lua) { return playerStorage->getAllSections(lua); };
        return LuaUtil::makeReadOnly(res);
    }

    void LuaStorage::clearTemporaryAndRemoveCallbacks()
    {
        auto it = mData.begin();
        while (it != mData.end())
        {
            it->second->mCallbacks.clear();
            // Note that we don't clear menu callbacks for permanent sections
            // because starting/loading a game doesn't reset menu scripts.
            if (it->second->mLifeTime == Section::Temporary)
            {
                it->second->mMenuScriptsCallbacks.clear();
                it->second->mValues.clear();
                it = mData.erase(it);
            }
            else
                ++it;
        }
    }

    void LuaStorage::load(lua_State* L, const std::filesystem::path& path)
    {
        assert(mData.empty()); // Shouldn't be used before loading
        try
        {
            std::uintmax_t fileSize = std::filesystem::file_size(path);
            Log(Debug::Info) << "Loading Lua storage \"" << path << "\" (" << fileSize << " bytes)";
            if (fileSize == 0)
                throw std::runtime_error("Storage file has zero length");

            std::ifstream fin(path, std::fstream::binary);
            std::string serializedData((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
            sol::table data = deserialize(L, serializedData);
            for (const auto& [sectionName, sectionTable] : data)
            {
                const std::shared_ptr<Section>& section = getSection(cast<std::string_view>(sectionName));
                for (const auto& [key, value] : cast<sol::table>(sectionTable))
                    section->set(cast<std::string_view>(key), value);
            }
        }
        catch (std::exception& e)
        {
            Log(Debug::Error) << "Cannot read \"" << path << "\": " << e.what();
        }
    }

    void LuaStorage::save(lua_State* L, const std::filesystem::path& path) const
    {
        sol::table data(L, sol::create);
        for (const auto& [sectionName, section] : mData)
        {
            if (section->mLifeTime == Section::Persistent && !section->mValues.empty())
                data[sectionName] = section->asTable(L);
        }
        std::string serializedData = serialize(data);
        Log(Debug::Info) << "Saving Lua storage \"" << path << "\" (" << serializedData.size() << " bytes)";
        std::ofstream fout(path, std::fstream::binary);
        fout.write(serializedData.data(), serializedData.size());
        fout.close();
    }

    const std::shared_ptr<LuaStorage::Section>& LuaStorage::getSection(std::string_view sectionName)
    {
        checkIfActive();
        auto it = mData.find(sectionName);
        if (it != mData.end())
            return it->second;
        auto section = std::make_shared<Section>(this, std::string(sectionName));
        sectionName = section->mSectionName;
        auto [newIt, _] = mData.emplace(sectionName, std::move(section));
        return newIt->second;
    }

    sol::object LuaStorage::getSection(lua_State* L, std::string_view sectionName, bool readOnly, bool forMenuScripts)
    {
        checkIfActive();
        const std::shared_ptr<Section>& section = getSection(sectionName);
        return sol::make_object<SectionView>(L, SectionView{ section, readOnly, forMenuScripts });
    }

    sol::table LuaStorage::getAllSections(lua_State* L, bool readOnly)
    {
        checkIfActive();
        sol::table res(L, sol::create);
        for (const auto& [sectionName, _] : mData)
            res[sectionName] = getSection(L, sectionName, readOnly);
        return res;
    }

}
