#ifndef COMPONENTS_LUA_SHAPES_BOX_H
#define COMPONENTS_LUA_SHAPES_BOX_H

#include <array>

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace LuaUtil
{
    class Box
    {
    public:
        Box(const glm::vec3& center, const glm::vec3& halfSize, const glm::quat& rotation = glm::quat(1, 0, 0, 0));
        Box(const glm::mat4& transform);

        std::array<glm::vec3, 8> vertices() const;

        glm::mat4 asTransform() const;

        // TODO: Add `contains` and `intersects` methods

        bool operator==(const Box& other) const;

        glm::vec3 mCenter;
        glm::vec3 mHalfSize;
        glm::quat mRotation;
    };
}
#endif // COMPONENTS_LUA_SHAPES_BOX_H
