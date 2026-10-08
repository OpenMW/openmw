#include <components/misc/convert.hpp>

#include <osg/Matrixf>
#include <osg/Quat>
#include <osg/Vec2f>
#include <osg/Vec3f>

#include <glm/gtc/matrix_transform.hpp>

#include <gtest/gtest.h>

namespace Misc::Convert
{
    namespace
    {
        constexpr float kEps = 1e-6f;

        TEST(MiscConvertGlm, Vec2RoundTrip)
        {
            const osg::Vec2f o(1.5f, -2.25f);
            const glm::vec2 g = toGlm(o);
            EXPECT_EQ(g.x, o.x());
            EXPECT_EQ(g.y, o.y());
            EXPECT_EQ(toOsg(g), o);
        }

        TEST(MiscConvertGlm, Vec3RoundTrip)
        {
            const osg::Vec3f o(1.0f, 2.0f, 3.0f);
            const glm::vec3 g = toGlm(o);
            EXPECT_EQ(g.x, 1.0f);
            EXPECT_EQ(g.y, 2.0f);
            EXPECT_EQ(g.z, 3.0f);
            EXPECT_EQ(toOsg(g), o);
        }

        TEST(MiscConvertGlm, QuatRoundTrip)
        {
            const osg::Quat o(0.1, 0.2, 0.3, 0.9);
            const glm::quat g = toGlm(o);
            EXPECT_NEAR(g.x, o.x(), kEps);
            EXPECT_NEAR(g.y, o.y(), kEps);
            EXPECT_NEAR(g.z, o.z(), kEps);
            EXPECT_NEAR(g.w, o.w(), kEps);
            const osg::Quat back = toOsg(g);
            EXPECT_NEAR(back.x(), o.x(), kEps);
            EXPECT_NEAR(back.y(), o.y(), kEps);
            EXPECT_NEAR(back.z(), o.z(), kEps);
            EXPECT_NEAR(back.w(), o.w(), kEps);
        }

        // Same transformation, opposite multiplication order.
        // OSG: v_new = v_old * M       (row-vector convention, row-major storage)
        // glm: v_new = M * v_old       (column-vector convention, column-major storage)
        // The 16-float memory image is identical between the two.
        TEST(MiscConvertGlm, MatrixTranslationPreservesTransform)
        {
            const osg::Matrixf o = osg::Matrixf::translate(4.0f, 5.0f, 6.0f);
            const glm::mat4 g = toGlm(o);

            const osg::Vec3f vo(1.0f, 2.0f, 3.0f);
            const osg::Vec3f mo = vo * o;

            const glm::vec4 vg(1.0f, 2.0f, 3.0f, 1.0f);
            const glm::vec4 mg = g * vg;

            EXPECT_NEAR(mo.x(), mg.x, kEps);
            EXPECT_NEAR(mo.y(), mg.y, kEps);
            EXPECT_NEAR(mo.z(), mg.z, kEps);
        }

        TEST(MiscConvertGlm, MatrixRotationPreservesTransform)
        {
            const osg::Matrixf o = osg::Matrixf::rotate(osg::Quat(0.2, osg::Vec3f(0, 0, 1)));
            const glm::mat4 g = toGlm(o);

            const osg::Vec3f vo(1.0f, 0.0f, 0.0f);
            const osg::Vec3f mo = vo * o;

            const glm::vec4 mg = g * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

            EXPECT_NEAR(mo.x(), mg.x, kEps);
            EXPECT_NEAR(mo.y(), mg.y, kEps);
            EXPECT_NEAR(mo.z(), mg.z, kEps);
        }

        TEST(MiscConvertGlm, MatrixRoundTrip)
        {
            const osg::Matrixf o
                = osg::Matrixf::translate(1.0f, 2.0f, 3.0f) * osg::Matrixf::rotate(osg::Quat(0.4, osg::Vec3f(0, 1, 0)));
            const osg::Matrixf back = toOsg(toGlm(o));
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    EXPECT_NEAR(back(i, j), o(i, j), kEps) << "at (" << i << ", " << j << ")";
        }

        TEST(MiscConvertGlm, DefaultsMatchOsg)
        {
            EXPECT_EQ(glm::vec3(), toGlm(osg::Vec3f()));
            EXPECT_EQ(glm::quat(), toGlm(osg::Quat()));
            EXPECT_EQ(glm::mat4(), toGlm(osg::Matrixf()));
        }
    }
}
