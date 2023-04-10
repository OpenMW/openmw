#include "sanitizerimpl.hpp"

#include <osg/Vec3f>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace Settings
{
    namespace
    {
        template <class T>
        struct Max final : Sanitizer<T>
        {
            T mMax;

            explicit Max(const T& max)
                : mMax(max)
            {
            }

            T apply(const T& value) const override { return std::max(value, mMax); }
        };

        template <class T>
        struct MaxStrict final : Sanitizer<T>
        {
            static_assert(std::is_floating_point_v<T>);

            T mMax;

            explicit MaxStrict(const T& max)
                : mMax(std::nextafter(max, std::numeric_limits<T>::max()))
            {
            }

            T apply(const T& value) const override { return std::max(value, mMax); }
        };

        template <>
        struct MaxStrict<osg::Vec3f> final : Sanitizer<osg::Vec3f>
        {
            osg::Vec3f mMax;

            explicit MaxStrict(const osg::Vec3f& max)
                : mMax(std::nextafter(max.x(), std::numeric_limits<float>::max()),
                    std::nextafter(max.y(), std::numeric_limits<float>::max()),
                    std::nextafter(max.z(), std::numeric_limits<float>::max()))
            {
            }

            osg::Vec3f apply(const osg::Vec3f& value) const override
            {
                return osg::Vec3f(
                    std::max(value.x(), mMax.x()), std::max(value.y(), mMax.y()), std::max(value.z(), mMax.z()));
            }
        };

        template <class T>
        struct Clamp final : Sanitizer<T>
        {
            T mMin;
            T mMax;

            explicit Clamp(const T& min, const T& max)
                : mMin(min)
                , mMax(max)
            {
            }

            T apply(const T& value) const override { return std::clamp(value, mMin, mMax); }
        };

        template <class T>
        auto getPrev(const T& value) -> std::enable_if_t<std::is_floating_point_v<T>, T>
        {
            assert(value > -std::numeric_limits<T>::max());
            return std::nextafter(value, -std::numeric_limits<T>::max());
        }

        template <class T>
        struct ClampStrictMax final : Sanitizer<T>
        {
            T mMin;
            T mMax;

            explicit ClampStrictMax(const T& min, const T& max)
                : mMin(min)
                , mMax(getPrev(max))
            {
            }

            T apply(const T& value) const override { return std::clamp(value, mMin, mMax); }
        };

        template <class T>
        struct Enum final : Sanitizer<T>
        {
            std::vector<T> mValues;

            explicit Enum(std::initializer_list<T> value)
                : mValues(std::make_move_iterator(value.begin()), std::make_move_iterator(value.end()))
            {
            }

            T apply(const T& value) const override
            {
                if (std::find(mValues.begin(), mValues.end(), value) == mValues.end())
                {
                    std::ostringstream message;
                    message << "Invalid enum value: " << value;
                    throw std::runtime_error(message.str());
                }
                return value;
            }
        };

        template <class T>
        struct EqualOrMax final : Sanitizer<T>
        {
            T mEqual;
            T mMax;

            explicit EqualOrMax(const T& equal, const T& max)
                : mEqual(equal)
                , mMax(max)
            {
            }

            T apply(const T& value) const override
            {
                if (value == mEqual)
                    return value;
                return std::max(value, mMax);
            }
        };
    }

    std::unique_ptr<Sanitizer<float>> makeMaxSanitizerFloat(float max)
    {
        return std::make_unique<Max<float>>(max);
    }

    std::unique_ptr<Sanitizer<int>> makeMaxSanitizerInt(int max)
    {
        return std::make_unique<Max<int>>(max);
    }

    std::unique_ptr<Sanitizer<std::size_t>> makeMaxSanitizerSize(std::size_t max)
    {
        return std::make_unique<Max<std::size_t>>(max);
    }

    std::unique_ptr<Sanitizer<std::uint64_t>> makeMaxSanitizerUInt64(std::uint64_t max)
    {
        return std::make_unique<Max<std::uint64_t>>(max);
    }

    std::unique_ptr<Sanitizer<float>> makeMaxStrictSanitizerFloat(float max)
    {
        return std::make_unique<MaxStrict<float>>(max);
    }

    std::unique_ptr<Sanitizer<osg::Vec3f>> makeMaxStrictSanitizerVec3f(const osg::Vec3f& max)
    {
        return std::make_unique<MaxStrict<osg::Vec3f>>(max);
    }

    std::unique_ptr<Sanitizer<float>> makeClampSanitizerFloat(float min, float max)
    {
        return std::make_unique<Clamp<float>>(min, max);
    }

    std::unique_ptr<Sanitizer<double>> makeClampSanitizerDouble(double min, double max)
    {
        return std::make_unique<Clamp<double>>(min, max);
    }

    std::unique_ptr<Sanitizer<int>> makeClampSanitizerInt(int min, int max)
    {
        return std::make_unique<Clamp<int>>(min, max);
    }

    std::unique_ptr<Sanitizer<float>> makeClampStrictMaxSanitizerFloat(float min, float max)
    {
        return std::make_unique<ClampStrictMax<float>>(min, max);
    }

    std::unique_ptr<Sanitizer<int>> makeEnumSanitizerInt(std::initializer_list<int> values)
    {
        return std::make_unique<Enum<int>>(values);
    }

    std::unique_ptr<Sanitizer<std::string>> makeEnumSanitizerString(std::initializer_list<std::string> values)
    {
        return std::make_unique<Enum<std::string>>(values);
    }

    std::unique_ptr<Sanitizer<float>> makeEqualOrMaxSanitizerFloat(float equal, float max)
    {
        return std::make_unique<EqualOrMax<float>>(equal, max);
    }
}
