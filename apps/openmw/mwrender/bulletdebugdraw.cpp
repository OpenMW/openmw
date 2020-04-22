#include "bulletdebugdraw.hpp"

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

#include <osg/Geometry>
#include <osg/Group>

#include <components/debug/debuglog.hpp>

#include "vismask.hpp"

namespace
{
    osg::Vec3f toOsg(const btVector3& vec)
    {
        return osg::Vec3f(vec.x(), vec.y(), vec.z());
    }
}

namespace MWRender
{

DebugDrawer::DebugDrawer(osg::ref_ptr<osg::Group> parentNode, btCollisionWorld *world)
    : mParentNode(parentNode),
      mWorld(world),
      mDebugOn(true)
{

    createGeometry();
}

void DebugDrawer::createGeometry()
{
    if (!mGeometry)
    {
        mGeometry = new osg::Geometry;
        mGeometry->setNodeMask(Mask_Debug);

        mVertices = new osg::Vec3Array;

        mDrawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES);

        mGeometry->setUseDisplayList(false);
        mGeometry->setVertexArray(mVertices);
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
        mWorld->debugDrawWorld();
        mDrawArrays->setCount(mVertices->size());
        mVertices->dirty();
        mGeometry->dirtyBound();
    }
}

void DebugDrawer::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color)
{
    mVertices->push_back(toOsg(from));
    mVertices->push_back(toOsg(to));
}

void DebugDrawer::drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color)
{
    mVertices->push_back(toOsg(PointOnB));
    mVertices->push_back(toOsg(PointOnB) + (toOsg(normalOnB) * distance * 20));
}

void DebugDrawer::reportErrorWarning(const char *warningString)
{
    Log(Debug::Warning) << warningString;
}

void DebugDrawer::setDebugMode(int isOn)
{
    mDebugOn = (isOn == 0) ? false : true;

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
