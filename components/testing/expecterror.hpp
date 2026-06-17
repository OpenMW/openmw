#ifndef OPENMW_COMPONENTS_TESTING_EXPECTERROR_H
#define OPENMW_COMPONENTS_TESTING_EXPECTERROR_H

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <exception>

#define EXPECT_ERROR(X, ERR_SUBSTR)                                                                                    \
    try                                                                                                                \
    {                                                                                                                  \
        X;                                                                                                             \
        FAIL() << "Expected error";                                                                                    \
    }                                                                                                                  \
    catch (const std::exception& e)                                                                                    \
    {                                                                                                                  \
        EXPECT_THAT(e.what(), ::testing::HasSubstr(ERR_SUBSTR));                                                       \
    }

#endif
