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
  ASSERT_EQ(true, Misc::begins("abc", "a"));
  ASSERT_EQ(true, Misc::begins("abc", "ab"));
  ASSERT_EQ(true, Misc::begins("abc", "abc"));
  ASSERT_EQ(true, Misc::begins("abcd", "abc"));
}

TEST_F(StringOpsTest, begins_not_matching)
{
  ASSERT_EQ(false, Misc::begins("abc", "b"));
  ASSERT_EQ(false, Misc::begins("abc", "bc"));
  ASSERT_EQ(false, Misc::begins("abc", "bcd"));
  ASSERT_EQ(false, Misc::begins("abc", "abcd"));
}

TEST_F(StringOpsTest, ibegins_matching)
{
  ASSERT_EQ(true, Misc::ibegins("Abc", "a"));
  ASSERT_EQ(true, Misc::ibegins("aBc", "ab"));
  ASSERT_EQ(true, Misc::ibegins("abC", "abc"));
  ASSERT_EQ(true, Misc::ibegins("abcD", "abc"));
}

TEST_F(StringOpsTest, ibegins_not_matching)
{
  ASSERT_EQ(false, Misc::ibegins("abc", "b"));
  ASSERT_EQ(false, Misc::ibegins("abc", "bc"));
  ASSERT_EQ(false, Misc::ibegins("abc", "bcd"));
  ASSERT_EQ(false, Misc::ibegins("abc", "abcd"));
}

TEST_F(StringOpsTest, ends_matching)
{
  ASSERT_EQ(true, Misc::ends("abc", "c"));
  ASSERT_EQ(true, Misc::ends("abc", "bc"));
  ASSERT_EQ(true, Misc::ends("abc", "abc"));
  ASSERT_EQ(true, Misc::ends("abcd", "abcd"));
}

TEST_F(StringOpsTest, ends_not_matching)
{
  ASSERT_EQ(false, Misc::ends("abc", "b"));
  ASSERT_EQ(false, Misc::ends("abc", "ab"));
  ASSERT_EQ(false, Misc::ends("abc", "bcd"));
  ASSERT_EQ(false, Misc::ends("abc", "abcd"));
}

TEST_F(StringOpsTest, iends_matching)
{
  ASSERT_EQ(true, Misc::iends("Abc", "c"));
  ASSERT_EQ(true, Misc::iends("aBc", "bc"));
  ASSERT_EQ(true, Misc::iends("abC", "abc"));
  ASSERT_EQ(true, Misc::iends("abcD", "abcd"));
}

TEST_F(StringOpsTest, iends_not_matching)
{
  ASSERT_EQ(false, Misc::iends("abc", "b"));
  ASSERT_EQ(false, Misc::iends("abc", "ab"));
  ASSERT_EQ(false, Misc::iends("abc", "bcd"));
  ASSERT_EQ(false, Misc::iends("abc", "abcd"));
}

