#include "query.hpp"

#include <sol/sol.hpp>

#include <components/lua/luastate.hpp>

#include "../mwclass/container.hpp"
#include "../mwworld/cellstore.hpp"

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
            Queries::Field({"cell", "name"}, typeid(std::string)),
            Queries::Field({"cell", "region"}, typeid(std::string)),
            Queries::Field({"cell", "isExterior"}, typeid(bool)),
            Queries::Field({"count"}, typeid(int32_t)),
        };
        static std::array doorFields = {
            Queries::Field({"isTeleport"}, typeid(bool)),
            Queries::Field({"destCell", "name"}, typeid(std::string)),
            Queries::Field({"destCell", "region"}, typeid(std::string)),
            Queries::Field({"destCell", "isExterior"}, typeid(bool)),
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

    bool checkQueryConditions(const Queries::Query& query, const ObjectId& id, const Context& context)
    {
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
        sol::object obj;
        MWWorld::Ptr ptr;
        if (context.mIsGlobal)
        {
            GObject g(id, context.mWorldView->getObjectRegistry());
            if (!g.isValid())
                return false;
            ptr = g.ptr();
            obj = sol::make_object(context.mLua->sol(), g);
        }
        else
        {
            LObject l(id, context.mWorldView->getObjectRegistry());
            if (!l.isValid())
                return false;
            ptr = l.ptr();
            obj = sol::make_object(context.mLua->sol(), l);
        }
        if (ptr.getRefData().getCount() == 0)
            return false;

        // It is important to exclude all markers before checking what class it is.
        // For example "prisonmarker" has class "Door" despite that it is only an invisible marker.
        if (isMarker(ptr))
            return false;

        const MWWorld::Class& cls = ptr.getClass();
        if (cls.isActivator() != (query.mQueryType == ObjectQueryTypes::ACTIVATORS))
            return false;
        if (cls.isActor() != (query.mQueryType == ObjectQueryTypes::ACTORS))
            return false;
        if (cls.isDoor() != (query.mQueryType == ObjectQueryTypes::DOORS))
            return false;
        if ((typeid(cls) == typeid(MWClass::Container)) != (query.mQueryType == ObjectQueryTypes::CONTAINERS))
            return false;

        std::vector<char> condStack;
        for (const Queries::Operation& op : query.mFilter.mOperations)
        {
            switch(op.mType)
            {
                case Queries::Operation::PUSH:
                {
                    const Queries::Condition& cond = query.mFilter.mConditions[op.mConditionIndex];
                    sol::object fieldObj = obj;
                    for (const std::string& field : cond.mField->path())
                        fieldObj = LuaUtil::getFieldOrNil(fieldObj, field);
                    bool c;
                    if (fieldObj == sol::nil)
                        c = false;
                    else if (cond.mField->type() == typeid(std::string))
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
        return condStack.empty() || condStack.back() != 0;
    }

    ObjectIdList selectObjectsFromList(const Queries::Query& query, const ObjectIdList& list, const Context& context)
    {
        if (!query.mOrderBy.empty() || !query.mGroupBy.empty() || query.mOffset > 0)
            throw std::runtime_error("OrderBy, GroupBy, and Offset are not supported");

        ObjectIdList res = std::make_shared<std::vector<ObjectId>>();
        for (const ObjectId& id : *list)
        {
            if (static_cast<int64_t>(res->size()) == query.mLimit)
                break;
            if (checkQueryConditions(query, id, context))
                res->push_back(id);
        }
        return res;
    }

    ObjectIdList selectObjectsFromCellStore(const Queries::Query& query, MWWorld::CellStore* store, const Context& context)
    {
        if (!query.mOrderBy.empty() || !query.mGroupBy.empty() || query.mOffset > 0)
            throw std::runtime_error("OrderBy, GroupBy, and Offset are not supported");

        ObjectIdList res = std::make_shared<std::vector<ObjectId>>();
        auto visitor = [&](const MWWorld::Ptr& ptr)
        {
            if (static_cast<int64_t>(res->size()) == query.mLimit)
                return false;
            context.mWorldView->getObjectRegistry()->registerPtr(ptr);
            if (checkQueryConditions(query, getId(ptr), context))
                res->push_back(getId(ptr));
            return static_cast<int64_t>(res->size()) != query.mLimit;
        };
        store->forEach(std::move(visitor));  // TODO: maybe use store->forEachType<TYPE> depending on query.mType
        return res;
    }

}
