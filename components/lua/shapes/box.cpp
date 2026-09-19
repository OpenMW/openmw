#include "box.hpp"

#include <glm/gtc/matrix_transform.hpp>
// matrix_decompose only exists as an experimental (gtx) extension whose API may change between glm
// releases. Opting in per TU flags this file for review on a glm bump and keeps the define out of
// every other TU's compile flags.
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

namespace LuaUtil
{
    Box::Box(const glm::vec3& center, const glm::vec3& halfSize, const glm::quat& rotation)
        : mCenter(center)
        , mHalfSize(halfSize)
        , mRotation(rotation)
    {
    }

    Box::Box(const glm::mat4& transform)
    {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(transform, mHalfSize, mRotation, mCenter, skew, perspective);
    }

    std::array<glm::vec3, 8> Box::vertices() const
    {
        return {
            mCenter + mRotation * glm::vec3(-mHalfSize.x, -mHalfSize.y, -mHalfSize.z),
            mCenter + mRotation * glm::vec3(mHalfSize.x, -mHalfSize.y, -mHalfSize.z),
            mCenter + mRotation * glm::vec3(mHalfSize.x, mHalfSize.y, -mHalfSize.z),
            mCenter + mRotation * glm::vec3(-mHalfSize.x, mHalfSize.y, -mHalfSize.z),
            mCenter + mRotation * glm::vec3(-mHalfSize.x, -mHalfSize.y, mHalfSize.z),
            mCenter + mRotation * glm::vec3(mHalfSize.x, -mHalfSize.y, mHalfSize.z),
            mCenter + mRotation * glm::vec3(mHalfSize.x, mHalfSize.y, mHalfSize.z),
            mCenter + mRotation * glm::vec3(-mHalfSize.x, mHalfSize.y, mHalfSize.z),
        };
    }

    glm::mat4 Box::asTransform() const
    {
        // glm column-vector composition. Applied to a unit-cube vertex v the result is:
        //   asTransform() * v = translate(center) * rotate(rotation) * scale(halfSize) * v
        // which matches the previous osg row-vector preMult* chain in reverse order (same
        // final transformation, opposite multiplication convention).
        return glm::translate(glm::mat4(1.0f), mCenter) * glm::mat4_cast(mRotation)
            * glm::scale(glm::mat4(1.0f), mHalfSize);
    }

    bool Box::operator==(const Box& other) const
    {
        return mCenter == other.mCenter && mHalfSize == other.mHalfSize && mRotation == other.mRotation;
    }
}
