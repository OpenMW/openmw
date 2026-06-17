#include <algorithm>

#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

#include <osg/Geometry>
#include <osg/Group>
#include <osg/Material>

#include <components/debug/debuglog.hpp>
#include <components/misc/convert.hpp>
#include <components/sceneutil/depth.hpp>
#include <osg/PolygonMode>
#include <osg/PolygonOffset>
#include <osg/ShapeDrawable>
#include <osg/StateSet>

#include "bulletdebugdraw.hpp"
#include "vismask.hpp"

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>

#include "../mwbase/environment.hpp"

namespace MWRender
{

    DebugDrawer::DebugDrawer(osg::ref_ptr<osg::Group> parentNode, btCollisionWorld* world, int debugMode)
        : mParentNode(std::move(parentNode))
        , mWorld(world)
    {
        DebugDrawer::setDebugMode(debugMode);
    }

    void DebugDrawer::createGeometry()
    {
        if (!mLinesGeometry)
        {
            mLinesGeometry = new osg::Geometry;
            mTrisGeometry = new osg::Geometry;
            mLinesGeometry->setNodeMask(Mask_Debug);
            mTrisGeometry->setNodeMask(Mask_Debug);

            mLinesVertices = new osg::Vec3Array;
            mTrisVertices = new osg::Vec3Array;
            mLinesColors = new osg::Vec4Array;

            mLinesDrawArrays = new osg::DrawArrays(osg::PrimitiveSet::LINES);
            mTrisDrawArrays = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES);

            mLinesGeometry->setUseDisplayList(false);
            mLinesGeometry->setVertexArray(mLinesVertices);
            mLinesGeometry->setColorArray(mLinesColors);
            mLinesGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
            mLinesGeometry->setDataVariance(osg::Object::DYNAMIC);
            mLinesGeometry->addPrimitiveSet(mLinesDrawArrays);

            mTrisGeometry->setUseDisplayList(false);
            mTrisGeometry->setVertexArray(mTrisVertices);
            mTrisGeometry->setDataVariance(osg::Object::DYNAMIC);
            mTrisGeometry->addPrimitiveSet(mTrisDrawArrays);

            mParentNode->addChild(mLinesGeometry);
            mParentNode->addChild(mTrisGeometry);

            auto* stateSet = new osg::StateSet;
            stateSet->setAttributeAndModes(
                new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE),
                osg::StateAttribute::ON);
            stateSet->setAttributeAndModes(new osg::PolygonOffset(
                SceneUtil::AutoDepth::isReversed() ? 1.f : -1.f, SceneUtil::AutoDepth::isReversed() ? 1.f : -1.f));
            osg::ref_ptr<osg::Material> material = new osg::Material;
            material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
            stateSet->setAttribute(material);
            mLinesGeometry->setStateSet(stateSet);
            mTrisGeometry->setStateSet(stateSet);
            mShapesRoot = new osg::Group;
            mShapesRoot->setStateSet(stateSet);
            mShapesRoot->setDataVariance(osg::Object::DYNAMIC);
            mShapesRoot->setNodeMask(Mask_Debug);
            mParentNode->addChild(mShapesRoot);

            MWBase::Environment::get().getResourceSystem()->getSceneManager()->recreateShaders(mLinesGeometry, "debug");
            MWBase::Environment::get().getResourceSystem()->getSceneManager()->recreateShaders(mTrisGeometry, "debug");
            MWBase::Environment::get().getResourceSystem()->getSceneManager()->recreateShaders(mShapesRoot, "debug");
        }
    }

    void DebugDrawer::destroyGeometry()
    {
        if (mLinesGeometry)
        {
            mParentNode->removeChild(mLinesGeometry);
            mParentNode->removeChild(mTrisGeometry);
            mParentNode->removeChild(mShapesRoot);
            mLinesGeometry = nullptr;
            mLinesVertices = nullptr;
            mLinesColors = nullptr;
            mLinesDrawArrays = nullptr;
            mTrisGeometry = nullptr;
            mTrisVertices = nullptr;
            mTrisDrawArrays = nullptr;
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
            mLinesVertices->clear();
            mTrisVertices->clear();
            mLinesColors->clear();
            mShapesRoot->removeChildren(0, mShapesRoot->getNumChildren());
            mWorld->debugDrawWorld();
            showCollisions();
            mLinesDrawArrays->setCount(static_cast<GLsizei>(mLinesVertices->size()));
            mTrisDrawArrays->setCount(static_cast<GLsizei>(mTrisVertices->size()));
            mLinesVertices->dirty();
            mTrisVertices->dirty();
            mLinesColors->dirty();
            mLinesGeometry->dirtyBound();
            mTrisGeometry->dirtyBound();
        }
    }

    void DebugDrawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
    {
        mLinesVertices->push_back(Misc::Convert::toOsg(from));
        mLinesVertices->push_back(Misc::Convert::toOsg(to));
        mLinesColors->push_back({ 1, 1, 1, 1 });
        mLinesColors->push_back({ 1, 1, 1, 1 });

#if BT_BULLET_VERSION < 317
        size_t size = mLinesVertices->size();
        if (size >= 6 && (*mLinesVertices)[size - 1] == (*mLinesVertices)[size - 6]
            && (*mLinesVertices)[size - 2] == (*mLinesVertices)[size - 3]
            && (*mLinesVertices)[size - 4] == (*mLinesVertices)[size - 5])
        {
            mTrisVertices->push_back(mLinesVertices->back());
            mLinesVertices->pop_back();
            mLinesColors->pop_back();
            mTrisVertices->push_back(mLinesVertices->back());
            mLinesVertices->pop_back();
            mLinesColors->pop_back();
            mLinesVertices->pop_back();
            mLinesColors->pop_back();
            mTrisVertices->push_back(mLinesVertices->back());
            mLinesVertices->pop_back();
            mLinesColors->pop_back();
            mLinesVertices->pop_back();
            mLinesColors->pop_back();
            mLinesVertices->pop_back();
            mLinesColors->pop_back();
        }
#endif
    }

    void DebugDrawer::drawTriangle(
        const btVector3& v0, const btVector3& v1, const btVector3& v2, const btVector3& color, btScalar)
    {
        mTrisVertices->push_back(Misc::Convert::toOsg(v0));
        mTrisVertices->push_back(Misc::Convert::toOsg(v1));
        mTrisVertices->push_back(Misc::Convert::toOsg(v2));
    }

    void DebugDrawer::addCollision(const btVector3& orig, const btVector3& normal)
    {
        mCollisionViews.emplace_back(orig, normal);
    }

    void DebugDrawer::showCollisions()
    {
        const auto now = std::chrono::steady_clock::now();
        for (auto& [from, to, created] : mCollisionViews)
        {
            if (now - created < std::chrono::seconds(2))
            {
                mLinesVertices->push_back(Misc::Convert::toOsg(from));
                mLinesVertices->push_back(Misc::Convert::toOsg(to));
                mLinesColors->push_back({ 1, 0, 0, 1 });
                mLinesColors->push_back({ 1, 0, 0, 1 });
            }
        }
        mCollisionViews.erase(
            std::remove_if(mCollisionViews.begin(), mCollisionViews.end(),
                [&now](const CollisionView& view) { return now - view.mCreated >= std::chrono::seconds(2); }),
            mCollisionViews.end());
    }

    void DebugDrawer::drawSphere(btScalar radius, const btTransform& transform, const btVector3& color)
    {
        auto* geom = new osg::ShapeDrawable(
            new osg::Sphere(Misc::Convert::toOsg(transform.getOrigin()), static_cast<float>(radius)));
        geom->setColor(osg::Vec4(1, 1, 1, 1));
        mShapesRoot->addChild(geom);
    }

    void DebugDrawer::reportErrorWarning(const char* warningString)
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
