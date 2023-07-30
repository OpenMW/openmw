#include <components/esm3/infoorder.hpp>

#include <gtest/gtest.h>

namespace ESM
{
    namespace
    {
        struct Value
        {
            RefId mId;
            RefId mPrev;

            Value() = default;
            Value(const Value&) = delete;
            Value(Value&&) = default;
            Value& operator=(const Value&) = delete;
            Value& operator=(Value&&) = default;
        };

        TEST(Esm3InfoOrderTest, insertInfoShouldNotCopyValue)
        {
            InfoOrder<Value> order;
            order.insertInfo(Value{}, false);
        }
    }
}
