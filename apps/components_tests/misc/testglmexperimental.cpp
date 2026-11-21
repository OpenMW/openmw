// Behavioural tests for the glm experimental (gtx) functions OpenMW relies on. gtx APIs carry no stability
// guarantee between glm releases, so these pin down what our callers expect. They compare against matrices
// assembled by hand rather than bit-exact outputs, so a change in glm's numerics is fine but a change in
// semantics (conjugated rotation, different sign convention for mirroring, ...) fails here.
//
// Currently used: glm::decompose (glm/gtx/matrix_decompose.hpp), by components/lua/shapes/box.cpp and
// components/lua/utilpackage.cpp.

#include <components/lua/shapes/box.hpp>

#include <cmath>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include <gtest/gtest.h>

namespace
{
    constexpr float kEps = 1e-5f;

    struct Decomposed
    {
        glm::vec3 mScale;
        glm::quat mRotation;
        glm::vec3 mTranslation;
        glm::vec3 mSkew;
        glm::vec4 mPerspective;
    };

    Decomposed decompose(const glm::mat4& m)
    {
        Decomposed d;
        EXPECT_TRUE(glm::decompose(m, d.mScale, d.mRotation, d.mTranslation, d.mSkew, d.mPerspective));
        return d;
    }

    glm::mat4 compose(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale)
    {
        return glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation)
            * glm::scale(glm::mat4(1.0f), scale);
    }

    void expectNear(const glm::vec3& actual, const glm::vec3& expected)
    {
        EXPECT_NEAR(actual.x, expected.x, kEps);
        EXPECT_NEAR(actual.y, expected.y, kEps);
        EXPECT_NEAR(actual.z, expected.z, kEps);
    }

    void expectNear(const glm::vec4& actual, const glm::vec4& expected)
    {
        EXPECT_NEAR(actual.x, expected.x, kEps);
        EXPECT_NEAR(actual.y, expected.y, kEps);
        EXPECT_NEAR(actual.z, expected.z, kEps);
        EXPECT_NEAR(actual.w, expected.w, kEps);
    }

    void expectNear(const glm::mat4& actual, const glm::mat4& expected)
    {
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                EXPECT_NEAR(actual[c][r], expected[c][r], kEps) << "column " << c << " row " << r;
    }

    // q and -q describe the same rotation, so compare by what they do rather than component-wise.
    void expectSameRotation(const glm::quat& actual, const glm::quat& expected)
    {
        EXPECT_NEAR(std::abs(glm::dot(actual, expected)), 1.0f, kEps);
        const glm::vec3 probe(0.3f, -1.2f, 2.5f);
        expectNear(actual * probe, expected * probe);
    }

    TEST(GlmExperimentalDecompose, Identity)
    {
        const Decomposed d = decompose(glm::mat4(1.0f));
        expectNear(d.mScale, glm::vec3(1.0f));
        expectSameRotation(d.mRotation, glm::quat(1, 0, 0, 0));
        expectNear(d.mTranslation, glm::vec3(0.0f));
        expectNear(d.mSkew, glm::vec3(0.0f));
        expectNear(d.mPerspective, glm::vec4(0, 0, 0, 1));
    }

    TEST(GlmExperimentalDecompose, TranslationOnly)
    {
        const Decomposed d = decompose(glm::translate(glm::mat4(1.0f), glm::vec3(6, 22, 18)));
        expectNear(d.mTranslation, glm::vec3(6, 22, 18));
        expectNear(d.mScale, glm::vec3(1.0f));
        expectSameRotation(d.mRotation, glm::quat(1, 0, 0, 0));
    }

    TEST(GlmExperimentalDecompose, NonUniformScaleOnly)
    {
        const Decomposed d = decompose(glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 1.0f, 4.0f)));
        expectNear(d.mScale, glm::vec3(0.5f, 1.0f, 4.0f));
        expectSameRotation(d.mRotation, glm::quat(1, 0, 0, 0));
        expectNear(d.mTranslation, glm::vec3(0.0f));
    }

    // Guards against the rotation coming back conjugated (inverted), which older glm releases did.
    TEST(GlmExperimentalDecompose, RotationIsNotConjugated)
    {
        const glm::quat rotation = glm::angleAxis(glm::half_pi<float>(), glm::vec3(0, 0, 1));
        const Decomposed d = decompose(glm::mat4_cast(rotation));
        expectSameRotation(d.mRotation, rotation);
        // +90 degrees around Z takes +X to +Y.
        expectNear(d.mRotation * glm::vec3(1, 0, 0), glm::vec3(0, 1, 0));
    }

    TEST(GlmExperimentalDecompose, TranslateRotateScale)
    {
        const glm::vec3 translation(1.5f, -20.0f, 300.0f);
        const glm::quat rotation = glm::angleAxis(0.7f, glm::normalize(glm::vec3(1, -2, 0.5f)));
        const glm::vec3 scale(2.0f, 0.25f, 3.0f);

        const Decomposed d = decompose(compose(translation, rotation, scale));
        expectNear(d.mTranslation, translation);
        expectSameRotation(d.mRotation, rotation);
        expectNear(d.mScale, scale);
        expectNear(d.mSkew, glm::vec3(0.0f));
        expectNear(d.mPerspective, glm::vec4(0, 0, 0, 1));
    }

    // A mirroring matrix has no unique decomposition; whatever glm picks must recompose to the original.
    TEST(GlmExperimentalDecompose, MirroredRecomposes)
    {
        const glm::mat4 m = compose(glm::vec3(4, 5, 6), glm::angleAxis(1.1f, glm::vec3(0, 1, 0)), glm::vec3(1, -2, 3));
        const Decomposed d = decompose(m);
        EXPECT_LT(d.mScale.x * d.mScale.y * d.mScale.z, 0.0f);
        expectNear(compose(d.mTranslation, d.mRotation, d.mScale), m);
    }

    TEST(GlmExperimentalDecompose, SingularMatrixFails)
    {
        glm::vec3 scale, translation, skew;
        glm::quat rotation;
        glm::vec4 perspective;
        EXPECT_FALSE(glm::decompose(glm::mat4(0.0f), scale, rotation, translation, skew, perspective));
    }

    TEST(LuaUtilBox, FromTransformRoundTrips)
    {
        const LuaUtil::Box box(
            glm::vec3(10, -5, 2), glm::vec3(1, 2, 3), glm::angleAxis(0.4f, glm::normalize(glm::vec3(1, 1, 0))));
        const LuaUtil::Box fromTransform(box.asTransform());
        expectNear(fromTransform.mCenter, box.mCenter);
        expectNear(fromTransform.mHalfSize, box.mHalfSize);
        expectSameRotation(fromTransform.mRotation, box.mRotation);

        const auto expected = box.vertices();
        const auto actual = fromTransform.vertices();
        for (std::size_t i = 0; i < expected.size(); ++i)
            expectNear(actual[i], expected[i]);
    }
}
