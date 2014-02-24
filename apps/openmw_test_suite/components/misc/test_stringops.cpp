#include <gtest/gtest.h>
#include "components/misc/stringops.hpp"

struct StringOpsTest : public ::testing::Test
{
  protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
};

TEST_F(StringOpsTest, begins_matching)
{
  ASSERT_TRUE(Misc::begins("abc", "a"));
  ASSERT_TRUE(Misc::begins("abc", "ab"));
  ASSERT_TRUE(Misc::begins("abc", "abc"));
  ASSERT_TRUE(Misc::begins("abcd", "abc"));
}

TEST_F(StringOpsTest, begins_not_matching)
{
  ASSERT_FALSE(Misc::begins("abc", "b"));
  ASSERT_FALSE(Misc::begins("abc", "bc"));
  ASSERT_FALSE(Misc::begins("abc", "bcd"));
  ASSERT_FALSE(Misc::begins("abc", "abcd"));
}

TEST_F(StringOpsTest, ibegins_matching)
{
  ASSERT_TRUE(Misc::ibegins("Abc", "a"));
  ASSERT_TRUE(Misc::ibegins("aBc", "ab"));
  ASSERT_TRUE(Misc::ibegins("abC", "abc"));
  ASSERT_TRUE(Misc::ibegins("abcD", "abc"));
}

TEST_F(StringOpsTest, ibegins_not_matching)
{
  ASSERT_FALSE(Misc::ibegins("abc", "b"));
  ASSERT_FALSE(Misc::ibegins("abc", "bc"));
  ASSERT_FALSE(Misc::ibegins("abc", "bcd"));
  ASSERT_FALSE(Misc::ibegins("abc", "abcd"));
}

TEST_F(StringOpsTest, ends_matching)
{
  ASSERT_TRUE(Misc::ends("abc", "c"));
  ASSERT_TRUE(Misc::ends("abc", "bc"));
  ASSERT_TRUE(Misc::ends("abc", "abc"));
  ASSERT_TRUE(Misc::ends("abcd", "abcd"));
}

TEST_F(StringOpsTest, ends_not_matching)
{
  ASSERT_FALSE(Misc::ends("abc", "b"));
  ASSERT_FALSE(Misc::ends("abc", "ab"));
  ASSERT_FALSE(Misc::ends("abc", "bcd"));
  ASSERT_FALSE(Misc::ends("abc", "abcd"));
}

TEST_F(StringOpsTest, iends_matching)
{
  ASSERT_TRUE(Misc::iends("Abc", "c"));
  ASSERT_TRUE(Misc::iends("aBc", "bc"));
  ASSERT_TRUE(Misc::iends("abC", "abc"));
  ASSERT_TRUE(Misc::iends("abcD", "abcd"));
}

TEST_F(StringOpsTest, iends_not_matching)
{
  ASSERT_FALSE(Misc::iends("abc", "b"));
  ASSERT_FALSE(Misc::iends("abc", "ab"));
  ASSERT_FALSE(Misc::iends("abc", "bcd"));
  ASSERT_FALSE(Misc::iends("abc", "abcd"));
}

