#include <gtest/gtest.h>
#include "components/esm/esmcommon.hpp"

TEST(EsmFixedString, operator__eq_ne)
{
    {
        SCOPED_TRACE("asdc == asdc");
        ESM::NAME name;
        name.assign("asdc");
        char s[4] = {'a', 's', 'd', 'c'};
        std::string ss(s, 4);

        EXPECT_TRUE(name == s);
        EXPECT_TRUE(name == ss.c_str());
        EXPECT_TRUE(name == ss);
    }
    {
        SCOPED_TRACE("asdc == asdcx");
        ESM::NAME name;
        name.assign("asdc");
        char s[5] = {'a', 's', 'd', 'c', 'x'};
        std::string ss(s, 5);

        EXPECT_TRUE(name != s);
        EXPECT_TRUE(name != ss.c_str());
        EXPECT_TRUE(name != ss);
    }
    {
        SCOPED_TRACE("asdc == asdc[NULL]");
        ESM::NAME name;
        name.assign("asdc");
        char s[5] = {'a', 's', 'd', 'c', '\0'};
        std::string ss(s, 5);

        EXPECT_TRUE(name == s);
        EXPECT_TRUE(name == ss.c_str());
        EXPECT_TRUE(name == ss);
    }
}
TEST(EsmFixedString, operator__eq_ne_const)
{
    {
        SCOPED_TRACE("asdc == asdc (const)");
        ESM::NAME name;
        name.assign("asdc");
        const char s[4] = { 'a', 's', 'd', 'c' };
        std::string ss(s, 4);

        EXPECT_TRUE(name == s);
        EXPECT_TRUE(name == ss.c_str());
        EXPECT_TRUE(name == ss);
    }
    {
        SCOPED_TRACE("asdc == asdcx (const)");
        ESM::NAME name;
        name.assign("asdc");
        const char s[5] = { 'a', 's', 'd', 'c', 'x' };
        std::string ss(s, 5);

        EXPECT_TRUE(name != s);
        EXPECT_TRUE(name != ss.c_str());
        EXPECT_TRUE(name != ss);
    }
    {
        SCOPED_TRACE("asdc == asdc[NULL] (const)");
        ESM::NAME name;
        name.assign("asdc");
        const char s[5] = { 'a', 's', 'd', 'c', '\0' };
        std::string ss(s, 5);

        EXPECT_TRUE(name == s);
        EXPECT_TRUE(name == ss.c_str());
        EXPECT_TRUE(name == ss);
    }
}

TEST(EsmFixedString, empty_strings)
{
    {
        SCOPED_TRACE("4 bytes");
        ESM::NAME empty = ESM::NAME();
        EXPECT_TRUE(empty == "");
        EXPECT_TRUE(empty == static_cast<uint32_t>(0));
        EXPECT_TRUE(empty != "some string");
        EXPECT_TRUE(empty != static_cast<uint32_t>(42));
    }
    {
        SCOPED_TRACE("32 bytes");
        ESM::NAME32 empty = ESM::NAME32();
        EXPECT_TRUE(empty == "");
        EXPECT_TRUE(empty != "some string");
    }
}

TEST(EsmFixedString, struct_size)
{
    ASSERT_EQ(4, sizeof(ESM::NAME));
    ASSERT_EQ(32, sizeof(ESM::NAME32));
    ASSERT_EQ(64, sizeof(ESM::NAME64));
}

TEST(EsmFixedString, is_pod)
{
     ASSERT_TRUE(std::is_pod<ESM::NAME>::value);
     ASSERT_TRUE(std::is_pod<ESM::NAME32>::value);
     ASSERT_TRUE(std::is_pod<ESM::NAME64>::value);
}

TEST(EsmFixedString, assign_should_zero_untouched_bytes_for_4)
{
    ESM::NAME value;
    value = static_cast<uint32_t>(0xFFFFFFFFu);
    value.assign(std::string(1, 'a'));
    EXPECT_EQ(value, static_cast<uint32_t>('a')) << value.toInt();
}

TEST(EsmFixedString, assign_should_only_truncate_for_4)
{
    ESM::NAME value;
    value.assign(std::string(5, 'a'));
    EXPECT_EQ(value, std::string(4, 'a'));
}

TEST(EsmFixedString, assign_should_truncate_and_set_last_element_to_zero)
{
    ESM::FixedString<17> value;
    value.assign(std::string(20, 'a'));
    EXPECT_EQ(value, std::string(16, 'a'));
}

TEST(EsmFixedString, assign_should_truncate_and_set_last_element_to_zero_for_32)
{
    ESM::NAME32 value;
    value.assign(std::string(33, 'a'));
    EXPECT_EQ(value, std::string(31, 'a'));
}

TEST(EsmFixedString, assign_should_truncate_and_set_last_element_to_zero_for_64)
{
    ESM::NAME64 value;
    value.assign(std::string(65, 'a'));
    EXPECT_EQ(value, std::string(63, 'a'));
}

TEST(EsmFixedString, assignment_operator_is_supported_for_uint32)
{
    ESM::NAME value;
    value = static_cast<uint32_t>(0xFEDCBA98u);
    EXPECT_EQ(value, static_cast<uint32_t>(0xFEDCBA98u)) << value.toInt();
}
