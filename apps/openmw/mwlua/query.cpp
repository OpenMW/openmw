#include "query.hpp"

#include <sol/sol.hpp>

#include <components/lua/luastate.hpp>

#include "../mwclass/container.hpp"

#include "worldview.hpp"

namespace MWLua
{

    static std::vector<QueryFieldGroup> initBasicFieldGroups()
    {
        auto createGroup = [](std::string name, const auto& arr) -> QueryFieldGroup
        {
            std::vector<const Queries::Field*> fieldPtrs;
            fieldPtrs.reserve(arr.size());
            for (const Queries::Field& field : arr)
                fieldPtrs.push_back(&field);
            return {std::move(name), std::move(fieldPtrs)};
        };
        static std::array objectFields = {
            Queries::Field({"type"}, typeid(std::string)),
            Queries::Field({"recordId"}, typeid(std::string)),
            Queries::Field({"count"}, typeid(int32_t)),
        };
        static std::array doorFields = {
            Queries::Field({"isTeleport"}, typeid(bool)),
            Queries::Field({"destCell"}, typeid(std::string)),
        };
        return std::vector<QueryFieldGroup>{
            createGroup("OBJECT", objectFields),
            createGroup("DOOR", doorFields),
        };
    }

    const std::vector<QueryFieldGroup>& getBasicQueryFieldGroups()
    {
        static std::vector<QueryFieldGroup> fieldGroups = initBasicFieldGroups();
        return fieldGroups;
    }

    ObjectIdList selectObjectsFromList(const Queries::Query& query, const ObjectIdList& list, const Context& context)
    {
        if (!query.mOrderBy.empty() || !query.mGroupBy.empty() || query.mOffset > 0)
            throw std::runtime_error("OrderBy, GroupBy, and Offset are not supported");

        ObjectIdList res = std::make_shared<std::vector<ObjectId>>();
        std::vector<char> condStack;
        auto compareFn = [](auto&& a, auto&& b, Queries::Condition::Type t)
        {
            switch (t)
            {
                case Queries::Condition::EQUAL: return a == b;
                case Queries::Condition::NOT_EQUAL: return a != b;
                case Queries::Condition::GREATER: return a > b;
                case Queries::Condition::GREATER_OR_EQUAL: return a >= b;
                case Queries::Condition::LESSER: return a < b;
                case Queries::Condition::LESSER_OR_EQUAL: return a <= b;
                default:
                    throw std::runtime_error("Unsupported condition type");
            }
        };
        for (const ObjectId& id : *list)
        {
            if (static_cast<int64_t>(res->size()) == query.mLimit)
                break;
            sol::object obj;
            MWWorld::Ptr ptr;
            if (context.mIsGlobal)
            {
                GObject g(id, context.mWorldView->getObjectRegistry());
                if (!g.isValid()) continue;
                ptr = g.ptr();
                obj = sol::make_object(context.mLua->sol(), g);
            }
            else
            {
                LObject l(id, context.mWorldView->getObjectRegistry());
                if (!l.isValid()) continue;
                ptr = l.ptr();
                obj = sol::make_object(context.mLua->sol(), l);
            }
            const MWWorld::Class& cls = ptr.getClass();
            if (cls.isActivator() && query.mQueryType != ObjectQueryTypes::ACTIVATORS)
                continue;
            if (cls.isActor() && query.mQueryType != ObjectQueryTypes::ACTORS)
                continue;
            if (cls.isDoor() && query.mQueryType != ObjectQueryTypes::DOORS)
                continue;
            if (typeid(cls) == typeid(MWClass::Container) && query.mQueryType != ObjectQueryTypes::CONTAINERS)
                continue;

            condStack.clear();
            for (const Queries::Operation& op : query.mFilter.mOperations)
            {
                switch(op.mType)
                {
                    case Queries::Operation::PUSH:
                    {
                        const Queries::Condition& cond = query.mFilter.mConditions[op.mConditionIndex];
                        sol::object fieldObj = obj;
                        for (const std::string& field : cond.mField->path())
                            fieldObj = sol::table(fieldObj)[field];
                        bool c;
                        if (cond.mField->type() == typeid(std::string))
                            c = compareFn(fieldObj.as<std::string_view>(), std::get<std::string>(cond.mValue), cond.mType);
                        else if (cond.mField->type() == typeid(float))
                            c = compareFn(fieldObj.as<float>(), std::get<float>(cond.mValue), cond.mType);
                        else if (cond.mField->type() == typeid(double))
                            c = compareFn(fieldObj.as<double>(), std::get<double>(cond.mValue), cond.mType);
                        else if (cond.mField->type() == typeid(bool))
                            c = compareFn(fieldObj.as<bool>(), std::get<bool>(cond.mValue), cond.mType);
                        else if (cond.mField->type() == typeid(int32_t))
                            c = compareFn(fieldObj.as<int32_t>(), std::get<int32_t>(cond.mValue), cond.mType);
                        else if (cond.mField->type() == typeid(int64_t))
                            c = compareFn(fieldObj.as<int64_t>(), std::get<int64_t>(cond.mValue), cond.mType);
                        else
                            throw std::runtime_error("Unknown field type");
                        condStack.push_back(c);
                        break;
                    }
                    case Queries::Operation::NOT:
                        condStack.back() = !condStack.back();
                        break;
                    case Queries::Operation::AND:
                    {
                        bool v = condStack.back();
                        condStack.pop_back();
                        condStack.back() = condStack.back() && v;
                        break;
                    }
                    case Queries::Operation::OR:
                    {
                        bool v = condStack.back();
                        condStack.pop_back();
                        condStack.back() = condStack.back() || v;
                        break;
                    }
                }
            }
            if (condStack.empty() || condStack.back() != 0)
                res->push_back(id);
        }
        return res;
    }

}
