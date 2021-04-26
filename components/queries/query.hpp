#ifndef COMPONENTS_QUERIES_QUERY
#define COMPONENTS_QUERIES_QUERY

#include <string>
#include <vector>
#include <typeindex>
#include <variant>
#include <stdexcept>

namespace Queries
{
    class Field
    {
        public:
            Field(std::vector<std::string> path, std::type_index type);

            const std::vector<std::string>& path() const { return mPath; }
            const std::type_index type() const { return mType; }

            std::string toString() const;

        private:
            std::vector<std::string> mPath;
            std::type_index mType;
    };

    struct OrderBy
    {
        const Field* mField;
        bool mDescending;
    };

    using FieldValue = std::variant<bool, int32_t, int64_t, float, double, std::string>;
    std::string toString(const FieldValue& value);

    struct Condition
    {
        enum Type
        {
            EQUAL = 0,
            NOT_EQUAL = 1,
            GREATER = 2,
            GREATER_OR_EQUAL = 3,
            LESSER = 4,
            LESSER_OR_EQUAL = 5,
            LIKE = 6,
        };

        std::string toString() const;

        const Field* mField;
        Type mType;
        FieldValue mValue;
    };

    struct Operation
    {
        enum Type
        {
            PUSH = 0, // push condition on stack
            NOT = 1, // invert top condition on stack
            AND = 2, // logic AND for two top conditions
            OR = 3, // logic OR for two top conditions
        };

        Type mType;
        size_t mConditionIndex; // used only if mType == PUSH
    };

    struct Filter
    {
        std::string toString() const;

        // combines with given condition or filter using operation `AND` or `OR`.
        void add(const Condition& c, Operation::Type op = Operation::AND);
        void add(const Filter& f, Operation::Type op = Operation::AND);

        std::vector<Condition> mConditions;
        std::vector<Operation> mOperations; // operations on conditions in reverse polish notation
    };

    struct Query
    {
        static constexpr int64_t sNoLimit = -1;

        Query(std::string type) : mQueryType(std::move(type)) {}
        std::string toString() const;

        std::string mQueryType;
        Filter mFilter;
        std::vector<OrderBy> mOrderBy;
        std::vector<const Field*> mGroupBy;
        int64_t mOffset = 0;
        int64_t mLimit = sNoLimit;
    };
}

#endif // !COMPONENTS_QUERIES_QUERY

