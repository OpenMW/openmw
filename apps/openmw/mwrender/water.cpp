#include "water.hpp"

#include <iomanip>

#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/PositionAttitudeTransform>
#include <osg/Depth>

#include <osgUtil/IncrementalCompileOperation>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/texturemanager.hpp>

#include <components/nifosg/controller.hpp>
#include <components/sceneutil/controller.hpp>

#include <components/esm/loadcell.hpp>

#include "vismask.hpp"
#include "ripplesimulation.hpp"
#include "renderbin.hpp"

namespace
{

    osg::ref_ptr<osg::Geometry> createWaterGeometry(float size, int segments, float textureRepeats)
    {
        osg::ref_ptr<osg::Vec3Array> verts (new osg::Vec3Array);
        osg::ref_ptr<osg::Vec2Array> texcoords (new osg::Vec2Array);

        // some drivers don't like huge triangles, so we do some subdivisons
        // a paged solution would be even better
        const float step = size/segments;
        const float texCoordStep = textureRepeats / segments;
        for (int x=0; x<segments; ++x)
        {
            for (int y=0; y<segments; ++y)
            {
                float x1 = -size/2.f + x*step;
                float y1 = -size/2.f + y*step;
                float x2 = x1 + step;
                float y2 = y1 + step;

                verts->push_back(osg::Vec3f(x1, y2, 0.f));
                verts->push_back(osg::Vec3f(x1, y1, 0.f));
                verts->push_back(osg::Vec3f(x2, y1, 0.f));
                verts->push_back(osg::Vec3f(x2, y2, 0.f));

                float u1 = x*texCoordStep;
                float v1 = y*texCoordStep;
                float u2 = u1 + texCoordStep;
                float v2 = v1 + texCoordStep;

                texcoords->push_back(osg::Vec2f(u1, v2));
                texcoords->push_back(osg::Vec2f(u1, v1));
                texcoords->push_back(osg::Vec2f(u2, v1));
                texcoords->push_back(osg::Vec2f(u2, v2));
            }
        }

        osg::ref_ptr<osg::Geometry> waterGeom (new osg::Geometry);
        waterGeom->setVertexArray(verts);
        waterGeom->setTexCoordArray(0, texcoords);

        waterGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,verts->size()));
        return waterGeom;
    }

    void createWaterStateSet(Resource::ResourceSystem* resourceSystem, osg::ref_ptr<osg::Node> node)
    {
        osg::ref_ptr<osg::StateSet> stateset (new osg::StateSet);

        osg::ref_ptr<osg::Material> material (new osg::Material);
        material->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(1.f, 1.f, 1.f, 1.f));
        material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 0.7f));
        material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 1.f));
        material->setColorMode(osg::Material::OFF);
        stateset->setAttributeAndModes(material, osg::StateAttribute::ON);

        stateset->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

        osg::ref_ptr<osg::Depth> depth (new osg::Depth);
        depth->setWriteMask(false);
        stateset->setAttributeAndModes(depth, osg::StateAttribute::ON);

        stateset->setRenderBinDetails(MWRender::RenderBin_Water, "RenderBin");

        std::vector<osg::ref_ptr<osg::Texture2D> > textures;
        for (int i=0; i<32; ++i)
        {
            std::ostringstream texname;
            texname << "textures/water/water" << std::setw(2) << std::setfill('0') << i << ".dds";
            textures.push_back(resourceSystem->getTextureManager()->getTexture2D(texname.str(), osg::Texture::REPEAT, osg::Texture::REPEAT));
        }

        osg::ref_ptr<NifOsg::FlipController> controller (new NifOsg::FlipController(0, 2/32.f, textures));
        controller->setSource(boost::shared_ptr<SceneUtil::ControllerSource>(new SceneUtil::FrameTimeSource));
        node->addUpdateCallback(controller);
        node->setStateSet(stateset);
        stateset->setTextureAttributeAndModes(0, textures[0], osg::StateAttribute::ON);
    }

}

namespace MWRender
{

// --------------------------------------------------------------------------------------------------------------------------------

Water::Water(osg::Group *parent, Resource::ResourceSystem *resourceSystem, osgUtil::IncrementalCompileOperation *ico, const MWWorld::Fallback* fallback)
    : mParent(parent)
    , mResourceSystem(resourceSystem)
    , mEnabled(true)
    , mToggled(true)
    , mTop(0)
{
    mSimulation.reset(new RippleSimulation(parent, resourceSystem, fallback));

    osg::ref_ptr<osg::Geometry> waterGeom = createWaterGeometry(CELL_SIZE*150, 40, 900);

    osg::ref_ptr<osg::Geode> geode (new osg::Geode);
    geode->addDrawable(waterGeom);
    geode->setNodeMask(Mask_Water);

    if (ico)
        ico->add(geode);

    createWaterStateSet(mResourceSystem, geode);

    mWaterNode = new osg::PositionAttitudeTransform;
    mWaterNode->addChild(geode);

    mParent->addChild(mWaterNode);

    setHeight(mTop);
}

Water::~Water()
{
    mParent->removeChild(mWaterNode);
}

void Water::setEnabled(bool enabled)
{
    mEnabled = enabled;
    updateVisible();
}

void Water::changeCell(const MWWorld::CellStore* store)
{
    if (store->getCell()->isExterior())
        mWaterNode->setPosition(getSceneNodeCoordinates(store->getCell()->mData.mX, store->getCell()->mData.mY));
    else
        mWaterNode->setPosition(osg::Vec3f(0,0,mTop));
}

void Water::setHeight(const float height)
{
    mTop = height;

    mSimulation->setWaterHeight(height);

    osg::Vec3f pos = mWaterNode->getPosition();
    pos.z() = height;
    mWaterNode->setPosition(pos);
}

void Water::update(float dt)
{
    mSimulation->update(dt);
}

void Water::updateVisible()
{
    mWaterNode->setNodeMask(mEnabled && mToggled ? ~0 : 0);
}

bool Water::toggle()
{
    mToggled = !mToggled;
    updateVisible();
    return mToggled;
}

bool Water::isUnderwater(const osg::Vec3f &pos) const
{
    return pos.z() < mTop && mToggled && mEnabled;
}

osg::Vec3f Water::getSceneNodeCoordinates(int gridX, int gridY)
{
    return osg::Vec3f(static_cast<float>(gridX * CELL_SIZE + (CELL_SIZE / 2)), static_cast<float>(gridY * CELL_SIZE + (CELL_SIZE / 2)), mTop);
}

void Water::addEmitter (const MWWorld::Ptr& ptr, float scale, float force)
{
    mSimulation->addEmitter (ptr, scale, force);
}

void Water::removeEmitter (const MWWorld::Ptr& ptr)
{
    mSimulation->removeEmitter (ptr);
}

void Water::updateEmitterPtr (const MWWorld::Ptr& old, const MWWorld::Ptr& ptr)
{
    mSimulation->updateEmitterPtr(old, ptr);
}

void Water::removeCell(const MWWorld::CellStore *store)
{
    mSimulation->removeCell(store);
}

void Water::clearRipples()
{
    mSimulation->clear();
}

}
