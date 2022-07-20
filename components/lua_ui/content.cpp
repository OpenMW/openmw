#include "content.hpp"

namespace LuaUi
{
    Content::Content(const sol::table& table)
    {
        size_t size = table.size();
        for (size_t index = 0; index < size; ++index)
        {
            sol::object value = table.get<sol::object>(index + 1);
            if (value.is<sol::table>())
                assign(index, value.as<sol::table>());
            else
                throw std::logic_error("UI Content children must all be tables.");
        }
    }

    void Content::assign(size_t index, const sol::table& table)
    {
        if (mOrdered.size() < index)
            throw std::logic_error("Can't have gaps in UI Content.");
        if (index == mOrdered.size())
            mOrdered.push_back(table);
        else
        {
            sol::optional<std::string> oldName = mOrdered[index]["name"];
            if (oldName.has_value())
                mNamed.erase(oldName.value());
            mOrdered[index] = table;
        }
        sol::optional<std::string> name = table["name"];
        if (name.has_value())
            mNamed[name.value()] = index;
    }

    void Content::assign(std::string_view name, const sol::table& table)
    {
        auto it = mNamed.find(name);
        if (it != mNamed.end())
            assign(it->second, table);
        else
            throw std::logic_error(std::string("Can't find a UI Content child with name ") += name);
    }

    void Content::insert(size_t index, const sol::table& table)
    {
        if (mOrdered.size() < index)
            throw std::logic_error("Can't have gaps in UI Content.");
        mOrdered.insert(mOrdered.begin() + index, table);
        for (size_t i = index; i < mOrdered.size(); ++i)
        {
            sol::optional<std::string> name = mOrdered[i]["name"];
            if (name.has_value())
                mNamed[name.value()] = index;
        }
    }

    sol::table Content::at(size_t index) const
    {
        if (index > size())
            throw std::logic_error("Invalid UI Content index.");
        return mOrdered.at(index);
    }

    sol::table Content::at(std::string_view name) const
    {
        auto it = mNamed.find(name);
        if (it == mNamed.end())
            throw std::logic_error("Invalid UI Content name.");
        return mOrdered.at(it->second);
    }

    size_t Content::remove(size_t index)
    {
        sol::table table = at(index);
        sol::optional<std::string> name = table["name"];
        if (name.has_value())
        {
            auto it = mNamed.find(name.value());
            if (it != mNamed.end())
                mNamed.erase(it);
        }
        mOrdered.erase(mOrdered.begin() + index);
        return index;
    }

    size_t Content::remove(std::string_view name)
    {
        auto it = mNamed.find(name);
        if (it == mNamed.end())
            throw std::logic_error("Invalid UI Content name.");
        size_t index = it->second;
        remove(index);
        return index;
    }

    size_t Content::indexOf(const sol::table& table)
    {
        auto it = std::find(mOrdered.begin(), mOrdered.end(), table);
        if (it == mOrdered.end())
            return size();
        else
            return it - mOrdered.begin();
    }
}
