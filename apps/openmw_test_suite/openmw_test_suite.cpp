#include <gmock/gmock.h>
#include <gtest/gtest.h>


int main(int argc, char** argv) {
  // The following line causes Google Mock to throw an exception on failure,
  // which will be interpreted by your testing framework as a test failure.
  ::testing::GTEST_FLAG(throw_on_failure) = true;
  ::testing::InitGoogleMock(&argc, argv);

  return RUN_ALL_TESTS();
}