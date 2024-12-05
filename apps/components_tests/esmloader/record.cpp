#include <components/esmloader/record.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <tuple>

namespace
{
    using namespace testing;
    using namespace EsmLoader;

    struct Value
    {
        int mKey;
        int mValue;
    };

    auto tie(const Value& v)
    {
        return std::tie(v.mKey, v.mValue);
    }

    bool operator==(const Value& l, const Value& r)
    {
        return tie(l) == tie(r);
    }

    std::ostream& operator<<(std::ostream& s, const Value& v)
    {
        return s << "Value {" << v.mKey << ", " << v.mValue << "}";
    }

    Record<Value> present(const Value& v)
    {
        return Record<Value>(false, v);
    }

    Record<Value> deleted(const Value& v)
    {
        return Record<Value>(true, v);
    }

    struct Params
    {
        Records<Value> mRecords;
        std::vector<Value> mResult;
    };

    struct EsmLoaderPrepareRecordTest : TestWithParam<Params>
    {
    };

    TEST_P(EsmLoaderPrepareRecordTest, prepareRecords)
    {
        auto records = GetParam().mRecords;
        const auto getKey = [&](const Record<Value>& v) { return v.mValue.mKey; };
        EXPECT_THAT(prepareRecords(records, getKey), ElementsAreArray(GetParam().mResult));
    }

    const std::array params = {
        Params{ {}, {} },
        Params{ { present(Value{ 1, 1 }) }, { Value{ 1, 1 } } },
        Params{ { deleted(Value{ 1, 1 }) }, {} },
        Params{ { present(Value{ 1, 1 }), present(Value{ 2, 2 }) }, { Value{ 1, 1 }, Value{ 2, 2 } } },
        Params{ { present(Value{ 2, 2 }), present(Value{ 1, 1 }) }, { Value{ 1, 1 }, Value{ 2, 2 } } },
        Params{ { present(Value{ 1, 1 }), present(Value{ 1, 2 }) }, { Value{ 1, 2 } } },
        Params{ { present(Value{ 1, 2 }), present(Value{ 1, 1 }) }, { Value{ 1, 1 } } },
        Params{ { present(Value{ 1, 1 }), deleted(Value{ 1, 2 }) }, {} },
        Params{ { deleted(Value{ 1, 1 }), present(Value{ 1, 2 }) }, { Value{ 1, 2 } } },
        Params{ { present(Value{ 1, 2 }), deleted(Value{ 1, 1 }) }, {} },
        Params{ { deleted(Value{ 1, 2 }), present(Value{ 1, 1 }) }, { Value{ 1, 1 } } },
    };

    INSTANTIATE_TEST_SUITE_P(Params, EsmLoaderPrepareRecordTest, ValuesIn(params));
}
