#include "components/esm/defs.hpp"
#include "components/esm/esmcommon.hpp"
#include <gtest/gtest.h>

namespace
{
    TEST(EsmFixedString, operator__eq_ne)
    {
        {
            SCOPED_TRACE("asdc == asdc");
            constexpr ESM::NAME name("asdc");
            char s[4] = { 'a', 's', 'd', 'c' };
            std::string ss(s, 4);

            EXPECT_TRUE(name == s);
            EXPECT_TRUE(name == ss.c_str());
            EXPECT_TRUE(name == ss);
        }
        {
            SCOPED_TRACE("asdc == asdcx");
            constexpr ESM::NAME name("asdc");
            char s[5] = { 'a', 's', 'd', 'c', 'x' };
            std::string ss(s, 5);

            EXPECT_TRUE(name != s);
            EXPECT_TRUE(name != ss.c_str());
            EXPECT_TRUE(name != ss);
        }
        {
            SCOPED_TRACE("asdc == asdc[NULL]");
            const ESM::NAME name("asdc");
            char s[5] = { 'a', 's', 'd', 'c', '\0' };
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
            constexpr ESM::NAME name("asdc");
            const char s[4] = { 'a', 's', 'd', 'c' };
            std::string ss(s, 4);

            EXPECT_TRUE(name == s);
            EXPECT_TRUE(name == ss.c_str());
            EXPECT_TRUE(name == ss);
        }
        {
            SCOPED_TRACE("asdc == asdcx (const)");
            constexpr ESM::NAME name("asdc");
            const char s[5] = { 'a', 's', 'd', 'c', 'x' };
            std::string ss(s, 5);

            EXPECT_TRUE(name != s);
            EXPECT_TRUE(name != ss.c_str());
            EXPECT_TRUE(name != ss);
        }
        {
            SCOPED_TRACE("asdc == asdc[NULL] (const)");
            constexpr ESM::NAME name("asdc");
            const char s[5] = { 'a', 's', 'd', 'c', '\0' };
            std::string ss(s, 5);

            EXPECT_TRUE(name == s);
            EXPECT_TRUE(name == ss.c_str());
            EXPECT_TRUE(name == ss);
        }
    }

    TEST(EsmFixedString, empty_strings)
    {
        constexpr std::string_view someStr = "some string";
        {
            SCOPED_TRACE("4 bytes");
            ESM::NAME empty = ESM::NAME();
            EXPECT_TRUE(empty == "");
            EXPECT_TRUE(empty == static_cast<uint32_t>(0));
            EXPECT_TRUE(empty != someStr);
            EXPECT_TRUE(empty != static_cast<uint32_t>(42));
        }
        {
            SCOPED_TRACE("32 bytes");
            ESM::NAME32 empty = ESM::NAME32();
            EXPECT_TRUE(empty == "");
            EXPECT_TRUE(empty != someStr);
        }
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

    TEST(EsmFixedString, construction_from_uint32_is_supported)
    {
        constexpr ESM::NAME value(0xFEDCBA98u);
        EXPECT_EQ(value, static_cast<std::uint32_t>(0xFEDCBA98u)) << value.toInt();
    }

    TEST(EsmFixedString, construction_from_RecNameInts_is_supported)
    {
        constexpr ESM::NAME value(ESM::RecNameInts::REC_ACTI);
        EXPECT_EQ(value, static_cast<std::uint32_t>(ESM::RecNameInts::REC_ACTI)) << value.toInt();
    }

    TEST(EsmFixedString, equality_operator_for_not_convertible_to_uint32_with_string_literal)
    {
        const ESM::FixedString<5> value("abcd");
        EXPECT_EQ(value, "abcd");
    }

    TEST(EsmFixedString, equality_operator_for_not_convertible_to_uint32_with_fixed_size_char_array)
    {
        const ESM::FixedString<5> value("abcd");
        const char other[5] = { 'a', 'b', 'c', 'd', '\0' };
        EXPECT_EQ(value, other);
    }

    TEST(EsmFixedString, equality_operator_for_not_convertible_to_uint32_with_const_char_pointer)
    {
        const ESM::FixedString<5> value("abcd");
        const char other[5] = { 'a', 'b', 'c', 'd', '\0' };
        EXPECT_EQ(value, static_cast<const char*>(other));
    }

    TEST(EsmFixedString, equality_operator_for_not_convertible_to_uint32_with_string)
    {
        const ESM::FixedString<5> value("abcd");
        EXPECT_EQ(value, std::string("abcd"));
    }

    TEST(EsmFixedString, equality_operator_for_not_convertible_to_uint32_with_string_view)
    {
        const ESM::FixedString<5> value("abcd");
        const std::string other("abcd");
        EXPECT_EQ(value, std::string_view(other));
    }

    TEST(EsmFixedString, equality_operator_should_not_get_out_of_bounds)
    {
        ESM::FixedString<5> value;
        const char other[5] = { 'a', 'b', 'c', 'd', 'e' };
        std::memcpy(value.mData, other, sizeof(other));
        EXPECT_EQ(value, static_cast<const char*>(other));
    }
}
