#include <gtest/gtest.h>
#include "components/misc/slice_array.hpp"

struct SliceArrayTest : public ::testing::Test
{
  protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
};

TEST_F(SliceArrayTest, hello_string)
{
  Misc::SString s("hello");
  ASSERT_EQ(sizeof("hello") - 1, s.length);
  ASSERT_FALSE(s=="hel");
  ASSERT_FALSE(s=="hell");
  ASSERT_TRUE(s=="hello");
}

TEST_F(SliceArrayTest, othello_string_with_offset_2_and_size_4)
{
  Misc::SString s("othello" + 2, 4);
  ASSERT_EQ(sizeof("hell") - 1, s.length);
  ASSERT_FALSE(s=="hel");
  ASSERT_TRUE(s=="hell");
  ASSERT_FALSE(s=="hello");
}

