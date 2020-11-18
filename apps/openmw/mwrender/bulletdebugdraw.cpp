#include <algorithm>

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

#include <osg/Geometry>
#include <osg/Group>

#include <components/debug/debuglog.hpp>
#include <components/misc/convert.hpp>

#include "bulletdebugdraw.hpp"
#include "vismask.hpp"

namespace MWRender
{

DebugDrawer::DebugDrawer(osg::ref_ptr<osg::Group> parentNode, btCollisionWorld *world, int debugMode)
    : mParentNode(parentNode),
      mWorld(world)
{
    setDebugMode(debugMode);
}

void DebugDrawer::createGeometry()
{
    if (!mGeometry)
    {
        mGeometry = new osg::Geometry;
        mGeometry->setNodeMask(Mask_Debug);

        mVertices = new osg::Vec3Array;
        mColors = new osg::Vec4Array;

        mDrawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES);

        mGeometry->setUseDisplayList(false);
        mGeometry->setVertexArray(mVertices);
        mGeometry->setColorArray(mColors);
        mGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        mGeometry->setDataVariance(osg::Object::DYNAMIC);
        mGeometry->addPrimitiveSet(mDrawArrays);

        mParentNode->addChild(mGeometry);
    }
}

void DebugDrawer::destroyGeometry()
{
    if (mGeometry)
    {
        mParentNode->removeChild(mGeometry);
        mGeometry = nullptr;
        mVertices = nullptr;
        mDrawArrays = nullptr;
    }
}

DebugDrawer::~DebugDrawer()
{
    destroyGeometry();
}

void DebugDrawer::step()
{
    if (mDebugOn)
    {
        mVertices->clear();
        mColors->clear();
        mWorld->debugDrawWorld();
        showCollisions();
        mDrawArrays->setCount(mVertices->size());
        mVertices->dirty();
        mColors->dirty();
        mGeometry->dirtyBound();
    }
}

void DebugDrawer::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color)
{
    mVertices->push_back(Misc::Convert::toOsg(from));
    mVertices->push_back(Misc::Convert::toOsg(to));
    mColors->push_back({1,1,1,1});
    mColors->push_back({1,1,1,1});
}

void DebugDrawer::addCollision(const btVector3& orig, const btVector3& normal)
{
    mCollisionViews.emplace_back(orig, normal);
}

void DebugDrawer::showCollisions()
{
    const auto now = std::chrono::steady_clock::now();
    for (auto& [from, to , created] : mCollisionViews)
    {
        if (now - created < std::chrono::seconds(2))
        {
            mVertices->push_back(Misc::Convert::toOsg(from));
            mVertices->push_back(Misc::Convert::toOsg(to));
            mColors->push_back({1,0,0,1});
            mColors->push_back({1,0,0,1});
        }
    }
    mCollisionViews.erase(std::remove_if(mCollisionViews.begin(), mCollisionViews.end(),
                [&now](const CollisionView& view) { return now - view.mCreated >= std::chrono::seconds(2); }),
            mCollisionViews.end());
}

void DebugDrawer::drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color)
{
    mVertices->push_back(Misc::Convert::toOsg(PointOnB));
    mVertices->push_back(Misc::Convert::toOsg(PointOnB) + (Misc::Convert::toOsg(normalOnB) * distance * 20));
    mColors->push_back({1,1,1,1});
    mColors->push_back({1,1,1,1});
}

void DebugDrawer::reportErrorWarning(const char *warningString)
{
    Log(Debug::Warning) << warningString;
}

void DebugDrawer::setDebugMode(int isOn)
{
    mDebugOn = (isOn != 0);

    if (!mDebugOn)
        destroyGeometry();
    else
        createGeometry();
}

int DebugDrawer::getDebugMode() const
{
    return mDebugOn;
}

}
