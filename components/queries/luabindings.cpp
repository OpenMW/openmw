#include "luabindings.hpp"

namespace sol
{
    template <>
    struct is_automagical<Queries::Field> : std::false_type {};

    template <>
    struct is_automagical<Queries::Filter> : std::false_type {};

    template <>
    struct is_automagical<Queries::Query> : std::false_type {};
}

namespace Queries
{
    template <Condition::Type type>
    struct CondBuilder
    {
        Filter operator()(const Field& field, const sol::object& o)
        {
            FieldValue value;
            if (field.type() == typeid(bool) && o.is<bool>())
                value = o.as<bool>();
            else if (field.type() == typeid(int32_t) && o.is<int32_t>())
                value = o.as<int32_t>();
            else if (field.type() == typeid(int64_t) && o.is<int64_t>())
                value = o.as<int64_t>();
            else if (field.type() == typeid(float) && o.is<float>())
                value = o.as<float>();
            else if (field.type() == typeid(double) && o.is<double>())
                value = o.as<double>();
            else if (field.type() == typeid(std::string) && o.is<std::string>())
                value = o.as<std::string>();
            else
                throw std::logic_error("Invalid value for field " + field.toString());
            Filter filter;
            filter.add({&field, type, value});
            return filter;
        }
    };

    void registerQueryBindings(sol::state& lua)
    {
        sol::usertype<Field> field = lua.new_usertype<Field>("QueryField");
        sol::usertype<Filter> filter = lua.new_usertype<Filter>("QueryFilter");
        sol::usertype<Query> query = lua.new_usertype<Query>("Query");

        field[sol::meta_function::to_string] = [](const Field& f) { return f.toString(); };
        field["eq"] = CondBuilder<Condition::EQUAL>();
        field["neq"] = CondBuilder<Condition::NOT_EQUAL>();
        field["lt"] = CondBuilder<Condition::LESSER>();
        field["lte"] = CondBuilder<Condition::LESSER_OR_EQUAL>();
        field["gt"] = CondBuilder<Condition::GREATER>();
        field["gte"] = CondBuilder<Condition::GREATER_OR_EQUAL>();
        field["like"] = CondBuilder<Condition::LIKE>();

        filter[sol::meta_function::to_string] = [](const Filter& filter) { return filter.toString(); };
        filter[sol::meta_function::multiplication] = [](const Filter& a, const Filter& b)
        {
            Filter res = a;
            res.add(b, Operation::AND);
            return res;
        };
        filter[sol::meta_function::addition] = [](const Filter& a, const Filter& b)
        {
            Filter res = a;
            res.add(b, Operation::OR);
            return res;
        };
        filter[sol::meta_function::unary_minus] = [](const Filter& a)
        {
            Filter res = a;
            if (!a.mConditions.empty())
                res.mOperations.push_back({Operation::NOT, 0});
            return res;
        };

        query[sol::meta_function::to_string] = [](const Query& q) { return q.toString(); };
        query["where"] = [](const Query& q, const Filter& filter)
        {
            Query res = q;
            res.mFilter.add(filter, Operation::AND);
            return res;
        };
        query["orderBy"] = [](const Query& q, const Field& field)
        {
            Query res = q;
            res.mOrderBy.push_back({&field, false});
            return res;
        };
        query["orderByDesc"] = [](const Query& q, const Field& field)
        {
            Query res = q;
            res.mOrderBy.push_back({&field, true});
            return res;
        };
        query["groupBy"] = [](const Query& q, const Field& field)
        {
            Query res = q;
            res.mGroupBy.push_back(&field);
            return res;
        };
        query["offset"] = [](const Query& q, int64_t offset)
        {
            Query res = q;
            res.mOffset = offset;
            return res;
        };
        query["limit"] = [](const Query& q, int64_t limit)
        {
            Query res = q;
            res.mLimit = limit;
            return res;
        };
    }
}

