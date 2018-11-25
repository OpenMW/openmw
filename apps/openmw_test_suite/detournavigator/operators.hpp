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

namespace
{
    template <class T>
    struct Wrapper {
        const T& mValue;
    };

    template <class Range>
    inline testing::Message& writeRange(testing::Message& message, const Range& range)
    {
        message << "{\n";
        for (const auto& v : range)
            message << Wrapper<typename std::decay<decltype(v)>::type> {v} << ",\n";
        return message << "}";
    }
}

namespace testing
{
    template <>
    inline testing::Message& Message::operator <<(const osg::Vec3f& value)
    {
        return (*this) << "osg::Vec3f(" << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.x()
            << ", " << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.y()
            << ", " << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.z()
            << ')';
    }

    template <>
    inline testing::Message& Message::operator <<(const Wrapper<osg::Vec3f>& value)
    {
        return (*this) << value.mValue;
    }

    template <>
    inline testing::Message& Message::operator <<(const Wrapper<float>& value)
    {
        return (*this) << std::setprecision(std::numeric_limits<float>::max_exponent10) << value.mValue;
    }

    template <>
    inline testing::Message& Message::operator <<(const std::deque<osg::Vec3f>& value)
    {
        return writeRange(*this, value);
    }

    template <>
    inline testing::Message& Message::operator <<(const std::vector<osg::Vec3f>& value)
    {
        return writeRange(*this, value);
    }

    template <>
    inline testing::Message& Message::operator <<(const std::vector<float>& value)
    {
        return writeRange(*this, value);
    }
}

#endif
