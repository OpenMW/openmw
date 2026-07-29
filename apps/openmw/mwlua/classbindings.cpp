#include "classbindings.hpp"

#include <components/esm3/loadclas.hpp>
#include <components/lua/luastate.hpp>

#include "idcollectionbindings.hpp"
#include "recordstore.hpp"
#include "types/usertypeutil.hpp"

namespace
{
    constexpr size_t sAttributeCount = std::tuple_size_v<decltype(ESM::Class::CLDTstruct::mAttribute)>;
    constexpr size_t sMajorSkillCount = std::tuple_size_v<decltype(ESM::Class::CLDTstruct::mSkills)>;
    constexpr size_t sMinorSkillCount = std::tuple_size_v<decltype(ESM::Class::CLDTstruct::mSkills)>;

    ESM::RefId& getAttribute(ESM::Class& c, size_t index)
    {
        return c.mData.mAttribute.at(index);
    }
    ESM::RefId& getMinorSkill(ESM::Class& c, size_t index)
    {
        return c.mData.mSkills.at(index)[0];
    }
    ESM::RefId& getMajorSkill(ESM::Class& c, size_t index)
    {
        return c.mData.mSkills.at(index)[1];
    }

    using ClassStatMapper = ESM::RefId& (*)(ESM::Class&, size_t);

    struct ClassStatArray
    {
        MWLua::MutableRecord<ESM::Class> mClass;
        ClassStatMapper mMapper;
        size_t mSize;

        ESM::RefId& find(size_t index) { return mMapper(mClass.find(), index); }

        const ESM::RefId& find(size_t index) const
        {
            return mMapper(const_cast<MWLua::MutableRecord<ESM::Class>&>(mClass).find(), index);
        }
    };
}

namespace sol
{
    template <>
    struct is_automagical<ESM::Class> : std::false_type
    {
    };
    template <>
    struct is_automagical<ClassStatArray> : std::false_type
    {
    };
}

namespace MWLua
{
    namespace
    {
        void setFromTable(ESM::Class& cls, std::size_t size, ClassStatMapper mapper, const sol::object& value)
        {
            if (value == sol::nil)
            {
                for (size_t i = 0; i < size; ++i)
                    mapper(cls, i) = {};
            }
            else if (value.is<ClassStatArray>())
            {
                const ClassStatArray& other = value.as<ClassStatArray>();
                for (size_t i = 0; i < size; ++i)
                {
                    if (i < other.mSize)
                        mapper(cls, i) = other.find(i);
                    else
                        mapper(cls, i) = {};
                }
            }
            else
            {
                auto table = value.as<sol::lua_table>();
                const size_t length = table.size();
                for (size_t i = 0; i < size; ++i)
                {
                    if (i < length)
                        mapper(cls, i)
                            = ESM::RefId::deserializeText(table.get<std::string_view>(LuaUtil::toLuaIndex(i)));
                    else
                        mapper(cls, i) = {};
                }
            }
        }

        auto addStatArray(std::size_t size, ClassStatMapper mapper)
        {
            return sol::property(
                [=](const MutableRecord<ESM::Class>& rec) {
                    return ClassStatArray{ rec, mapper, size };
                },
                [=](MutableRecord<ESM::Class>& rec, const sol::object& value) {
                    ESM::Class& cls = rec.find();
                    setFromTable(cls, size, mapper, value);
                });
        }

        int32_t getSpecialization(std::string_view id)
        {
            for (int32_t i = ESM::Class::Combat; i <= ESM::Class::Stealth; ++i)
            {
                if (ESM::Class::specializationIndexToLuaId[i] == id)
                    return i;
            }
            throw std::runtime_error("invalid specialization");
        }

        void addClassStatArrayType(sol::state_view& lua)
        {
            auto arrayT = lua.new_usertype<ClassStatArray>("ESM3_MutableClassStatIds");
            arrayT[sol::meta_function::length] = [](const ClassStatArray& array) { return array.mSize; };
            arrayT[sol::meta_function::index]
                = [](const ClassStatArray& array, uint32_t index) -> std::optional<ESM::RefId> {
                if (index == 0 || index > array.mSize)
                    return {};
                return array.find(index - 1);
            };
            arrayT[sol::meta_function::new_index] = [](ClassStatArray& array, uint32_t index, std::string_view value) {
                if (index == 0 || index > array.mSize)
                    throw std::runtime_error("index out of range");
                array.find(index - 1) = ESM::RefId::deserializeText(value);
            };
            arrayT[sol::meta_function::ipairs] = lua["ipairsForArray"].template get<sol::function>();
            arrayT[sol::meta_function::pairs] = lua["ipairsForArray"].template get<sol::function>();
        }

        template <class T>
        void addUserType(sol::state_view& lua, std::string_view name)
        {
            auto record = lua.new_usertype<T>(name);
            record[sol::meta_function::to_string]
                = [](const T& rec) -> std::string { return "ESM3_Class[" + rec.mId.toDebugString() + "]"; };
            record["id"] = sol::readonly_property([](const T& rec) -> ESM::RefId { return rec.mId; });
            Types::addProperty(record, "name", &ESM::Class::mName);
            Types::addProperty(record, "description", &ESM::Class::mDescription);
            Types::addProperty(record, "isPlayable", &ESM::Class::mData, &ESM::Class::CLDTstruct::mIsPlayable);
            if constexpr (Types::RecordType<T>::isMutable)
            {
                addClassStatArrayType(lua);
                record["attributes"] = addStatArray(sAttributeCount, getAttribute);
                record["majorSkills"] = addStatArray(sMajorSkillCount, getMajorSkill);
                record["minorSkills"] = addStatArray(sMinorSkillCount, getMinorSkill);
                record["specialization"] = sol::property(
                    [](const MutableRecord<ESM::Class>& rec) -> std::string_view {
                        return ESM::Class::specializationIndexToLuaId.at(rec.find().mData.mSpecialization);
                    },
                    [](MutableRecord<ESM::Class>& rec, std::string_view value) {
                        ESM::Class& cls = rec.find();
                        cls.mData.mSpecialization = getSpecialization(value);
                    });
            }
            else
            {
                record["attributes"] = sol::readonly_property([lua](const ESM::Class& rec) -> sol::table {
                    return createReadOnlyRefIdTable(lua, rec.mData.mAttribute);
                });
                record["majorSkills"] = sol::readonly_property([lua](const ESM::Class& rec) -> sol::table {
                    return createReadOnlyRefIdTable(lua, rec.mData.mSkills, [](const auto& pair) { return pair[1]; });
                });
                record["minorSkills"] = sol::readonly_property([lua](const ESM::Class& rec) -> sol::table {
                    return createReadOnlyRefIdTable(lua, rec.mData.mSkills, [](const auto& pair) { return pair[0]; });
                });
                record["specialization"] = sol::readonly_property([](const ESM::Class& rec) -> std::string_view {
                    return ESM::Class::specializationIndexToLuaId.at(rec.mData.mSpecialization);
                });
            }
        }
    }

    ESM::Class tableToClass(const sol::table& rec)
    {
        auto cls = Types::initFromTemplate<ESM::Class>(rec);
        if (rec["name"] != sol::nil)
            cls.mName = rec["name"];
        if (rec["description"] != sol::nil)
            cls.mDescription = rec["description"];
        if (rec["isPlayable"] != sol::nil)
            cls.mData.mIsPlayable = rec["isPlayable"].get<bool>();
        if (rec["specialization"] != sol::nil)
            cls.mData.mSpecialization = getSpecialization(rec["specialization"].get<std::string_view>());
        if (rec["attributes"] != sol::nil)
            setFromTable(cls, sAttributeCount, getAttribute, rec["attributes"]);
        if (rec["minorSkills"] != sol::nil)
            setFromTable(cls, sMinorSkillCount, getMinorSkill, rec["minorSkills"]);
        if (rec["majorSkills"] != sol::nil)
            setFromTable(cls, sMajorSkillCount, getMajorSkill, rec["majorSkills"]);
        return cls;
    }

    void addMutableClassType(sol::state_view& lua)
    {
        addUserType<MutableRecord<ESM::Class>>(lua, "ESM3_MutableClass");
    }

    sol::table initClassRecordBindings(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table classes(lua, sol::create);
        addRecordFunctionBinding<ESM::Class>(classes, context);
        addUserType<ESM::Class>(lua, "ESM3_Class");
        return LuaUtil::makeReadOnly(classes);
    }
}
