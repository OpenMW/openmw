#ifndef OPENMW_TEST_SUITE_DETOURNAVIGATOR_OPERATORS_H
#define OPENMW_TEST_SUITE_DETOURNAVIGATOR_OPERATORS_H

#include <components/bullethelpers/operators.hpp>
#include <components/detournavigator/debug.hpp>

#include <deque>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

#include <gtest/gtest.h>

namespace DetourNavigator
{
    static inline bool operator ==(const TileBounds& lhs, const TileBounds& rhs)
    {
        return lhs.mMin == rhs.mMin && lhs.mMax == rhs.mMax;
    }
}

namespace testing
{
    template <>
    inline testing::Message& Message::operator <<(const std::deque<osg::Vec3f>& value)
    {
        (*this) << "{\n";
        for (const auto& v : value)
        {
            std::ostringstream stream;
            stream << "osg::Vec3f("
                   << std::setprecision(std::numeric_limits<float>::max_exponent10) << v.x() << ", "
                   << std::setprecision(std::numeric_limits<float>::max_exponent10) << v.y() << ", "
                   << std::setprecision(std::numeric_limits<float>::max_exponent10) << v.z() << ")";
            (*this) << stream.str() << ",\n";
        }
        return (*this) << "}";
    }
}

#endif
