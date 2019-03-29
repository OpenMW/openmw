#include "agentpath.hpp"
#include "detourdebugdraw.hpp"

#include <components/detournavigator/settings.hpp>

#include <algorithm>

namespace
{
    void drawAgent(duDebugDraw& debugDraw, const osg::Vec3f& pos, const float radius, const float height,
            const float climb, const unsigned color)
    {
        debugDraw.depthMask(false);

        duDebugDrawCylinderWire(&debugDraw, pos.x() - radius, pos.z() + 0.02f, pos.y() - radius, pos.x() + radius,
            pos.z() + height, pos.y() + radius, color, radius * 0.2f);

        duDebugDrawCircle(&debugDraw, pos.x(), pos.z() + climb, pos.y(), radius, duRGBA(0, 0 , 0, 64), radius * 0.1f);

        const auto colb = duRGBA(0, 0, 0, 196);
        debugDraw.begin(DU_DRAW_LINES);
        debugDraw.vertex(pos.x(), pos.z() - climb, pos.y(), colb);
        debugDraw.vertex(pos.x(), pos.z() + climb, pos.y(), colb);
        debugDraw.vertex(pos.x() - radius / 2, pos.z() + 0.02f, pos.y(), colb);
        debugDraw.vertex(pos.x() + radius / 2, pos.z() + 0.02f, pos.y(), colb);
        debugDraw.vertex(pos.x(), pos.z() + 0.02f, pos.y() - radius / 2, colb);
        debugDraw.vertex(pos.x(), pos.z() + 0.02f, pos.y() + radius / 2, colb);
        debugDraw.end();

        debugDraw.depthMask(true);
    }
}

namespace SceneUtil
{
    osg::ref_ptr<osg::Group> createAgentPathGroup(const std::deque<osg::Vec3f>& path,
            const osg::Vec3f& halfExtents, const osg::Vec3f& start, const osg::Vec3f& end,
            const DetourNavigator::Settings& settings)
    {
        using namespace DetourNavigator;

        const osg::ref_ptr<osg::Group> group(new osg::Group);

        DebugDraw debugDraw(*group, osg::Vec3f(0, 0, 0), 1);

        const auto agentRadius = halfExtents.x();
        const auto agentHeight = 2.0f * halfExtents.z();
        const auto agentClimb = settings.mMaxClimb;
        const auto startColor = duRGBA(128, 25, 0, 192);
        const auto endColor = duRGBA(51, 102, 0, 129);

        drawAgent(debugDraw, start, agentRadius, agentHeight, agentClimb, startColor);
        drawAgent(debugDraw, end, agentRadius, agentHeight, agentClimb, endColor);

        const auto pathColor = duRGBA(0, 0, 0, 220);

        debugDraw.depthMask(false);

        debugDraw.begin(osg::PrimitiveSet::LINE_STRIP, agentRadius * 0.5f);
        debugDraw.vertex(osg::Vec3f(start.x(), start.z() + agentClimb, start.y()).ptr(), startColor);
        std::for_each(path.begin(), path.end(),
            [&] (const osg::Vec3f& v) { debugDraw.vertex(osg::Vec3f(v.x(), v.z() + agentClimb, v.y()).ptr(), pathColor); });
        debugDraw.vertex(osg::Vec3f(end.x(), end.z() + agentClimb, end.y()).ptr(), endColor);
        debugDraw.end();

        debugDraw.depthMask(true);

        return group;
    }
}
