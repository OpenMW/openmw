#include "query.hpp"

#include <sstream>
#include <iomanip>

namespace Queries
{
    Field::Field(std::vector<std::string> path, std::type_index type)
        : mPath(std::move(path))
        , mType(type) {}

    std::string Field::toString() const
    {
        std::string result;
        for (const std::string& segment : mPath)
        {
            if (!result.empty())
                result += ".";
            result += segment;
        }
        return result;
    }

    std::string toString(const FieldValue& value)
    {
        return std::visit([](auto&& arg) -> std::string
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::string>)
            {
                std::ostringstream oss;
                oss << std::quoted(arg);
                return oss.str();
            }
            else if constexpr (std::is_same_v<T, bool>)
                return arg ? "true" : "false";
            else
                return std::to_string(arg);
        }, value);
    }

    std::string Condition::toString() const
    {
        std::string res;
        res += mField->toString();
        switch (mType)
        {
            case Condition::EQUAL: res += " == "; break;
            case Condition::NOT_EQUAL: res += " != "; break;
            case Condition::LESSER: res += " < "; break;
            case Condition::LESSER_OR_EQUAL: res += " <= "; break;
            case Condition::GREATER: res += " > "; break;
            case Condition::GREATER_OR_EQUAL: res += " >= "; break;
            case Condition::LIKE: res += " LIKE "; break;
        }
        res += Queries::toString(mValue);
        return res;
    }

    void Filter::add(const Condition& c, Operation::Type op)
    {
        mOperations.push_back({Operation::PUSH, mConditions.size()});
        mConditions.push_back(c);
        if (mConditions.size() > 1)
            mOperations.push_back({op, 0});
    }

    void Filter::add(const Filter& f, Operation::Type op)
    {
        size_t conditionOffset = mConditions.size();
        size_t operationsBefore = mOperations.size();
        mConditions.insert(mConditions.end(), f.mConditions.begin(), f.mConditions.end());
        mOperations.insert(mOperations.end(), f.mOperations.begin(), f.mOperations.end());
        for (size_t i = operationsBefore; i < mOperations.size(); ++i)
            mOperations[i].mConditionIndex += conditionOffset;
        if (conditionOffset > 0 && !f.mConditions.empty())
            mOperations.push_back({op, 0});
    }

    std::string Filter::toString() const
    {
        if(mOperations.empty())
            return "";
        std::vector<std::string> stack;
        auto pop = [&stack](){ auto v = stack.back(); stack.pop_back(); return v; };
        auto push = [&stack](const std::string& s) { stack.push_back(s); };
        for (const Operation& op : mOperations)
        {
            if(op.mType == Operation::PUSH)
                push(mConditions[op.mConditionIndex].toString());
            else if(op.mType == Operation::AND)
            {
                auto rhs = pop();
                auto lhs = pop();
                std::string res;
                res += "(";
                res += lhs;
                res += ") AND (";
                res += rhs;
                res += ")";
                push(res);
            }
            else if (op.mType == Operation::OR)
            {
                auto rhs = pop();
                auto lhs = pop();
                std::string res;
                res += "(";
                res += lhs;
                res += ") OR (";
                res += rhs;
                res += ")";
                push(res);
            }
            else if (op.mType == Operation::NOT)
            {
                std::string res;
                res += "NOT (";
                res += pop();
                res += ")";
                push(res);
            }
            else
                throw std::logic_error("Unknown operation type!");
        }
        return pop();
    }

    std::string Query::toString() const
    {
        std::string res;
        res += "SELECT ";
        res += mQueryType;

        std::string filter = mFilter.toString();
        if(!filter.empty())
        {
            res += " WHERE ";
            res += filter;
        }

        std::string order;
        for(const OrderBy& ord : mOrderBy)
        {
            if(!order.empty())
                order += ", ";
            order += ord.mField->toString();
            if(ord.mDescending)
                order += " DESC";
        }
        if (!order.empty())
        {
            res += " ORDER BY ";
            res += order;
        }

        std::string group;
        for (const Field* f: mGroupBy)
        {
            if (!group.empty())
                group += " ,";
            group += f->toString();
        }
        if (!group.empty())
        {
            res += " GROUP BY ";
            res += group;
        }

        if (mLimit != sNoLimit)
        {
            res += " LIMIT ";
            res += std::to_string(mLimit);
        }

        if (mOffset != 0)
        {
            res += " OFFSET ";
            res += std::to_string(mOffset);
        }

        return res;
    }
}

