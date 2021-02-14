#include <gtest/gtest.h>
#include "components/misc/endianness.hpp"

struct EndiannessTest : public ::testing::Test {};

TEST_F(EndiannessTest, test_swap_endianness_inplace1)
{
  uint8_t zero=0x00;
  uint8_t ff=0xFF;
  uint8_t fortytwo=0x42;
  uint8_t half=128;

  Misc::swapEndiannessInplace(zero);
  EXPECT_EQ(zero, 0x00);

  Misc::swapEndiannessInplace(ff);
  EXPECT_EQ(ff, 0xFF);

  Misc::swapEndiannessInplace(fortytwo);
  EXPECT_EQ(fortytwo, 0x42);

  Misc::swapEndiannessInplace(half);
  EXPECT_EQ(half, 128);
}

TEST_F(EndiannessTest, test_swap_endianness_inplace2)
{
  uint16_t zero = 0x0000;
  uint16_t ffff = 0xFFFF;
  uint16_t n12 = 0x0102;
  uint16_t fortytwo = 0x0042;

  Misc::swapEndiannessInplace(zero);
  EXPECT_EQ(zero, 0x0000);
  Misc::swapEndiannessInplace(zero);
  EXPECT_EQ(zero, 0x0000);

  Misc::swapEndiannessInplace(ffff);
  EXPECT_EQ(ffff, 0xFFFF);
  Misc::swapEndiannessInplace(ffff);
  EXPECT_EQ(ffff, 0xFFFF);

  Misc::swapEndiannessInplace(n12);
  EXPECT_EQ(n12, 0x0201);
  Misc::swapEndiannessInplace(n12);
  EXPECT_EQ(n12, 0x0102);

  Misc::swapEndiannessInplace(fortytwo);
  EXPECT_EQ(fortytwo, 0x4200);
  Misc::swapEndiannessInplace(fortytwo);
  EXPECT_EQ(fortytwo, 0x0042);
}

TEST_F(EndiannessTest, test_swap_endianness_inplace4)
{
  uint32_t zero = 0x00000000;
  uint32_t n1234 = 0x01020304;
  uint32_t ffff = 0xFFFFFFFF;

  Misc::swapEndiannessInplace(zero);
  EXPECT_EQ(zero, 0x00000000);
  Misc::swapEndiannessInplace(zero);
  EXPECT_EQ(zero, 0x00000000);

  Misc::swapEndiannessInplace(n1234);
  EXPECT_EQ(n1234, 0x04030201);
  Misc::swapEndiannessInplace(n1234);
  EXPECT_EQ(n1234, 0x01020304);
  
  Misc::swapEndiannessInplace(ffff);
  EXPECT_EQ(ffff, 0xFFFFFFFF);
  Misc::swapEndiannessInplace(ffff);
  EXPECT_EQ(ffff, 0xFFFFFFFF);
}

TEST_F(EndiannessTest, test_swap_endianness_inplace8)
{
  uint64_t zero = 0x0000'0000'0000'0000;
  uint64_t n1234 = 0x0102'0304'0506'0708;
  uint64_t ffff = 0xFFFF'FFFF'FFFF'FFFF;

  Misc::swapEndiannessInplace(zero);
  EXPECT_EQ(zero, 0x0000'0000'0000'0000);
  Misc::swapEndiannessInplace(zero);
  EXPECT_EQ(zero, 0x0000'0000'0000'0000);
  
  Misc::swapEndiannessInplace(ffff);
  EXPECT_EQ(ffff, 0xFFFF'FFFF'FFFF'FFFF);
  Misc::swapEndiannessInplace(ffff);
  EXPECT_EQ(ffff, 0xFFFF'FFFF'FFFF'FFFF);

  Misc::swapEndiannessInplace(n1234);
  EXPECT_EQ(n1234, 0x0807'0605'0403'0201);
  Misc::swapEndiannessInplace(n1234);
  EXPECT_EQ(n1234, 0x0102'0304'0506'0708);
}

TEST_F(EndiannessTest, test_swap_endianness_inplace_float)
{
  const uint32_t original = 0x4023d70a;
  const uint32_t expected = 0x0ad72340;

  float number;
  memcpy(&number, &original, sizeof(original));

  Misc::swapEndiannessInplace(number);

  EXPECT_TRUE(!memcmp(&number, &expected, sizeof(expected)));
}

TEST_F(EndiannessTest, test_swap_endianness_inplace_double)
{
  const uint64_t original = 0x040047ae147ae147ul;
  const uint64_t expected = 0x47e17a14ae470004ul;

  double number;
  memcpy(&number, &original, sizeof(original));

  Misc::swapEndiannessInplace(number);

  EXPECT_TRUE(!memcmp(&number, &expected, sizeof(expected)) );
}
