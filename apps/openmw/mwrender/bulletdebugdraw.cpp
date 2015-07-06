#include "bulletdebugdraw.hpp"

#include <iostream>

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>

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
    mGeode = new osg::Geode;
    mParentNode->addChild(mGeode);
    mGeode->setNodeMask(Mask_Debug);

    createGeometry();

    mParentNode->addChild(mGeode);
}

void DebugDrawer::createGeometry()
{
    if (!mGeometry)
    {
        mGeometry = new osg::Geometry;

        mVertices = new osg::Vec3Array;

        mDrawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES);

        mGeometry->setUseDisplayList(false);
        mGeometry->setVertexArray(mVertices);
        mGeometry->setDataVariance(osg::Object::DYNAMIC);
        mGeometry->addPrimitiveSet(mDrawArrays);

        mGeode->addDrawable(mGeometry);
    }
}

void DebugDrawer::destroyGeometry()
{
    if (mGeometry)
    {
        mGeode->removeDrawable(mGeometry);
        mGeometry = NULL;
        mVertices = NULL;
        mDrawArrays = NULL;
    }
}

DebugDrawer::~DebugDrawer()
{
    mParentNode->removeChild(mGeode);
}

void DebugDrawer::step()
{
    if (mDebugOn)
    {
        mVertices->clear();
        mWorld->debugDrawWorld();
        mDrawArrays->setCount(mVertices->size());
        mVertices->dirty();
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
    std::cerr << warningString << std::endl;
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
