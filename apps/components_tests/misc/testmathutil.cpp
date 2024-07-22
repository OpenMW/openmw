#include <components/misc/mathutil.hpp>

#include <osg/io_utils>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <iomanip>
#include <limits>

MATCHER_P2(Vec3fEq, other, precision, "")
{
    return std::abs(arg.x() - other.x()) < precision && std::abs(arg.y() - other.y()) < precision
        && std::abs(arg.z() - other.z()) < precision;
}

namespace testing
{
    template <>
    inline testing::Message& Message::operator<<(const osg::Vec3f& value)
    {
        return (*this) << std::setprecision(std::numeric_limits<float>::max_exponent10) << "osg::Vec3f(" << value.x()
                       << ", " << value.y() << ", " << value.z() << ')';
    }

    template <>
    inline testing::Message& Message::operator<<(const osg::Quat& value)
    {
        return (*this) << std::setprecision(std::numeric_limits<float>::max_exponent10) << "osg::Quat(" << value.x()
                       << ", " << value.y() << ", " << value.z() << ", " << value.w() << ')';
    }
}

namespace Misc
{
    namespace
    {
        using namespace testing;

        struct MiscToEulerAnglesXZQuatTest : TestWithParam<std::pair<osg::Quat, osg::Vec3f>>
        {
        };

        TEST_P(MiscToEulerAnglesXZQuatTest, shouldReturnValueCloseTo)
        {
            const osg::Vec3f result = toEulerAnglesXZ(GetParam().first);
            EXPECT_THAT(result, Vec3fEq(GetParam().second, std::numeric_limits<float>::epsilon()))
                << "toEulerAnglesXZ(" << GetParam().first << ") = " << result;
        }

        const std::pair<osg::Quat, osg::Vec3f> eulerAnglesXZQuat[] = {
            {
                osg::Quat(1, 0, 0, 0),
                osg::Vec3f(0, 0, osg::PI),
            },
            {
                osg::Quat(0, 1, 0, 0),
                osg::Vec3f(0, 0, 0),
            },
            {
                osg::Quat(0, 0, 1, 0),
                osg::Vec3f(0, 0, osg::PI),
            },
            {
                osg::Quat(0, 0, 0, 1),
                osg::Vec3f(0, 0, 0),
            },
            {
                osg::Quat(-0.5, -0.5, -0.5, -0.5),
                osg::Vec3f(-osg::PI_2f, 0, 0),
            },
            {
                osg::Quat(0.5, -0.5, -0.5, -0.5),
                osg::Vec3f(0, 0, -osg::PI_2f),
            },
            {
                osg::Quat(0.5, 0.5, -0.5, -0.5),
                osg::Vec3f(osg::PI_2f, 0, 0),
            },
            {
                osg::Quat(0.5, 0.5, 0.5, -0.5),
                osg::Vec3f(0, 0, osg::PI_2f),
            },
            {
                osg::Quat(0.5, 0.5, 0.5, 0.5),
                osg::Vec3f(-osg::PI_2f, 0, 0),
            },
            {
                // normalized osg::Quat(0.1, 0.2, 0.3, 0.4)
                osg::Quat(0.18257418583505536, 0.36514837167011072, 0.54772255750516607, 0.73029674334022143),
                osg::Vec3f(-0.72972762584686279296875f, 0, -1.10714876651763916015625f),
            },
            {
                osg::Quat(-0.18257418583505536, 0.36514837167011072, 0.54772255750516607, 0.73029674334022143),
                osg::Vec3f(-0.13373161852359771728515625f, 0, -1.2277724742889404296875f),
            },
            {
                osg::Quat(0.18257418583505536, -0.36514837167011072, 0.54772255750516607, 0.73029674334022143),
                osg::Vec3f(0.13373161852359771728515625f, 0, -1.2277724742889404296875f),
            },
            {
                osg::Quat(0.18257418583505536, 0.36514837167011072, -0.54772255750516607, 0.73029674334022143),
                osg::Vec3f(0.13373161852359771728515625f, 0, 1.2277724742889404296875f),
            },
            {
                osg::Quat(0.18257418583505536, 0.36514837167011072, 0.54772255750516607, -0.73029674334022143),
                osg::Vec3f(-0.13373161852359771728515625, 0, 1.2277724742889404296875f),
            },
            {
                osg::Quat(0.246736, -0.662657, -0.662667, 0.246739),
                osg::Vec3f(-osg::PI_2f, 0, 2.5199801921844482421875f),
            },
        };

        INSTANTIATE_TEST_SUITE_P(FromQuat, MiscToEulerAnglesXZQuatTest, ValuesIn(eulerAnglesXZQuat));

        struct MiscToEulerAnglesZYXQuatTest : TestWithParam<std::pair<osg::Quat, osg::Vec3f>>
        {
        };

        TEST_P(MiscToEulerAnglesZYXQuatTest, shouldReturnValueCloseTo)
        {
            const osg::Vec3f result = toEulerAnglesZYX(GetParam().first);
            EXPECT_THAT(result, Vec3fEq(GetParam().second, std::numeric_limits<float>::epsilon()))
                << "toEulerAnglesZYX(" << GetParam().first << ") = " << result;
        }

        const std::pair<osg::Quat, osg::Vec3f> eulerAnglesZYXQuat[] = {
            {
                osg::Quat(1, 0, 0, 0),
                osg::Vec3f(osg::PI, 0, 0),
            },
            {
                osg::Quat(0, 1, 0, 0),
                osg::Vec3f(osg::PI, 0, osg::PI),
            },
            {
                osg::Quat(0, 0, 1, 0),
                osg::Vec3f(0, 0, osg::PI),
            },
            {
                osg::Quat(0, 0, 0, 1),
                osg::Vec3f(0, 0, 0),
            },
            {
                osg::Quat(-0.5, -0.5, -0.5, -0.5),
                osg::Vec3f(0, -osg::PI_2f, -osg::PI_2f),
            },
            {
                osg::Quat(0.5, -0.5, -0.5, -0.5),
                osg::Vec3f(osg::PI_2f, 0, -osg::PI_2f),
            },
            {
                osg::Quat(0.5, 0.5, -0.5, -0.5),
                osg::Vec3f(0, osg::PI_2f, -osg::PI_2f),
            },
            {
                osg::Quat(0.5, 0.5, 0.5, -0.5),
                osg::Vec3f(osg::PI_2f, 0, osg::PI_2f),
            },
            {
                osg::Quat(0.5, 0.5, 0.5, 0.5),
                osg::Vec3f(0, -osg::PI_2f, -osg::PI_2f),
            },
            {
                // normalized osg::Quat(0.1, 0.2, 0.3, 0.4)
                osg::Quat(0.18257418583505536, 0.36514837167011072, 0.54772255750516607, 0.73029674334022143),
                osg::Vec3f(0.1973955929279327392578125f, -0.8232119083404541015625f, -1.37340080738067626953125f),
            },
            {
                osg::Quat(-0.18257418583505536, 0.36514837167011072, 0.54772255750516607, 0.73029674334022143),
                osg::Vec3f(0.78539812564849853515625f, -0.339836895465850830078125f, -1.428899288177490234375f),
            },
            {
                osg::Quat(0.18257418583505536, -0.36514837167011072, 0.54772255750516607, 0.73029674334022143),
                osg::Vec3f(-0.78539812564849853515625f, 0.339836895465850830078125f, -1.428899288177490234375f),
            },
            {
                osg::Quat(0.18257418583505536, 0.36514837167011072, -0.54772255750516607, 0.73029674334022143),
                osg::Vec3f(-0.78539812564849853515625f, -0.339836895465850830078125f, 1.428899288177490234375f),
            },
            {
                osg::Quat(0.18257418583505536, 0.36514837167011072, 0.54772255750516607, -0.73029674334022143),
                osg::Vec3f(0.78539812564849853515625f, 0.339836895465850830078125f, 1.428899288177490234375f),
            },
            {
                osg::Quat(0.246736, -0.662657, 0.246739, -0.662667),
                osg::Vec3f(0.06586204469203948974609375f, -osg::PI_2f, 0.64701664447784423828125f),
            },
        };

        INSTANTIATE_TEST_SUITE_P(FromQuat, MiscToEulerAnglesZYXQuatTest, ValuesIn(eulerAnglesZYXQuat));
    }
}
