#include "ripplesimulation.hpp"

#include <iomanip>

#include <osg/PolygonOffset>
#include <osg/Texture2D>
#include <osg/Material>
#include <osg/Depth>
#include <osg/PositionAttitudeTransform>
#include <osgParticle/ParticleSystem>
#include <osgParticle/ParticleSystemUpdater>

#include <components/misc/rng.hpp>
#include <components/nifosg/controller.hpp>
#include <components/resource/imagemanager.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/fallback/fallback.hpp>
#include <components/sceneutil/vismask.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwmechanics/actorutil.hpp"

namespace
{
    void createWaterRippleStateSet(Resource::ResourceSystem* resourceSystem,osg::Node* node)
    {
        int rippleFrameCount = Fallback::Map::getInt("Water_RippleFrameCount");
        if (rippleFrameCount <= 0)
            return;

        const std::string& tex = Fallback::Map::getString("Water_RippleTexture");

        std::vector<osg::ref_ptr<osg::Texture2D> > textures;
        for (int i=0; i<rippleFrameCount; ++i)
        {
            std::ostringstream texname;
            texname << "textures/water/" << tex << std::setw(2) << std::setfill('0') << i << ".dds";
            osg::ref_ptr<osg::Texture2D> tex2 (new osg::Texture2D(resourceSystem->getImageManager()->getImage(texname.str())));
            tex2->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
            tex2->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
            resourceSystem->getSceneManager()->applyFilterSettings(tex2);
            textures.push_back(tex2);
        }

        osg::ref_ptr<NifOsg::FlipController> controller (new NifOsg::FlipController(0, 0.3f/rippleFrameCount, textures));
        controller->setSource(std::shared_ptr<SceneUtil::ControllerSource>(new SceneUtil::FrameTimeSource));
        node->addUpdateCallback(controller);

        osg::ref_ptr<osg::StateSet> stateset (new osg::StateSet);
        stateset->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
        stateset->setTextureAttributeAndModes(0, textures[0], osg::StateAttribute::ON);

        osg::ref_ptr<osg::Depth> depth (new osg::Depth);
        depth->setWriteMask(false);
        stateset->setAttributeAndModes(depth, osg::StateAttribute::ON);

        osg::ref_ptr<osg::PolygonOffset> polygonOffset (new osg::PolygonOffset);
        polygonOffset->setUnits(-1);
        polygonOffset->setFactor(-1);
        stateset->setAttributeAndModes(polygonOffset, osg::StateAttribute::ON);

        stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

        osg::ref_ptr<osg::Material> mat (new osg::Material);
        mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 1.f));
        mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1.f, 1.f, 1.f, 1.f));
        mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 1.f));
        mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 0.f));
        mat->setColorMode(osg::Material::DIFFUSE);
        stateset->setAttributeAndModes(mat, osg::StateAttribute::ON);

        node->setStateSet(stateset);
    }
}

namespace MWRender
{

RippleSimulation::RippleSimulation(osg::Group *parent, Resource::ResourceSystem* resourceSystem)
    : mParent(parent)
{
    mParticleSystem = new osgParticle::ParticleSystem;

    mParticleSystem->setParticleAlignment(osgParticle::ParticleSystem::FIXED);
    mParticleSystem->setAlignVectorX(osg::Vec3f(1,0,0));
    mParticleSystem->setAlignVectorY(osg::Vec3f(0,1,0));

    osgParticle::Particle& particleTemplate = mParticleSystem->getDefaultParticleTemplate();
    particleTemplate.setSizeRange(osgParticle::rangef(15, 180));
    particleTemplate.setColorRange(osgParticle::rangev4(osg::Vec4f(1,1,1,0.7), osg::Vec4f(1,1,1,0.7)));
    particleTemplate.setAlphaRange(osgParticle::rangef(1.f, 0.f));
    particleTemplate.setAngularVelocity(osg::Vec3f(0,0,Fallback::Map::getFloat("Water_RippleRotSpeed")));
    particleTemplate.setLifeTime(Fallback::Map::getFloat("Water_RippleLifetime"));

    osg::ref_ptr<osgParticle::ParticleSystemUpdater> updater (new osgParticle::ParticleSystemUpdater);
    updater->addParticleSystem(mParticleSystem);

    mParticleNode = new osg::PositionAttitudeTransform;
    mParticleNode->setName("Ripple Root");
    mParticleNode->addChild(updater);
    mParticleNode->addChild(mParticleSystem);
    mParticleNode->setNodeMask(SceneUtil::Mask_Water);

    createWaterRippleStateSet(resourceSystem, mParticleNode);

    resourceSystem->getSceneManager()->recreateShaders(mParticleNode);

    mParent->addChild(mParticleNode);
}

RippleSimulation::~RippleSimulation()
{
    mParent->removeChild(mParticleNode);
}

void RippleSimulation::update(float dt)
{
    const MWBase::World* world = MWBase::Environment::get().getWorld();
    for (Emitter& emitter : mEmitters)
    {
        MWWorld::ConstPtr& ptr = emitter.mPtr;
        if (ptr == MWBase::Environment::get().getWorld ()->getPlayerPtr())
        {
            // fetch a new ptr (to handle cell change etc)
            // for non-player actors this is done in updateObjectCell
            ptr = MWBase::Environment::get().getWorld ()->getPlayerPtr();
        }

        osg::Vec3f currentPos (ptr.getRefData().getPosition().asVec3());

        bool shouldEmit = (world->isUnderwater(ptr.getCell(), currentPos) && !world->isSubmerged(ptr)) || world->isWalkingOnWater(ptr);
        if (shouldEmit && (currentPos - emitter.mLastEmitPosition).length() > 10)
        {
            emitter.mLastEmitPosition = currentPos;

            currentPos.z() = mParticleNode->getPosition().z();

            if (mParticleSystem->numParticles()-mParticleSystem->numDeadParticles() > 500)
                continue; // TODO: remove the oldest particle to make room?

            emitRipple(currentPos);
        }
    }
}


void RippleSimulation::addEmitter(const MWWorld::ConstPtr& ptr, float scale, float force)
{
    Emitter newEmitter;
    newEmitter.mPtr = ptr;
    newEmitter.mScale = scale;
    newEmitter.mForce = force;
    newEmitter.mLastEmitPosition = osg::Vec3f(0,0,0);
    mEmitters.push_back (newEmitter);
}

void RippleSimulation::removeEmitter (const MWWorld::ConstPtr& ptr)
{
    for (std::vector<Emitter>::iterator it = mEmitters.begin(); it != mEmitters.end(); ++it)
    {
        if (it->mPtr == ptr)
        {
            mEmitters.erase(it);
            return;
        }
    }
}

void RippleSimulation::updateEmitterPtr (const MWWorld::ConstPtr& old, const MWWorld::ConstPtr& ptr)
{
    for (std::vector<Emitter>::iterator it = mEmitters.begin(); it != mEmitters.end(); ++it)
    {
        if (it->mPtr == old)
        {
            it->mPtr = ptr;
            return;
        }
    }
}

void RippleSimulation::removeCell(const MWWorld::CellStore *store)
{
    for (std::vector<Emitter>::iterator it = mEmitters.begin(); it != mEmitters.end();)
    {
        if ((it->mPtr.isInCell() && it->mPtr.getCell() == store) && it->mPtr != MWMechanics::getPlayer())
        {
            it = mEmitters.erase(it);
        }
        else
            ++it;
    }
}

void RippleSimulation::emitRipple(const osg::Vec3f &pos)
{
    if (std::abs(pos.z() - mParticleNode->getPosition().z()) < 20)
    {
        osgParticle::Particle* p = mParticleSystem->createParticle(nullptr);
        p->setPosition(osg::Vec3f(pos.x(), pos.y(), 0.f));
        p->setAngle(osg::Vec3f(0,0, Misc::Rng::rollProbability() * osg::PI * 2 - osg::PI));
    }
}

void RippleSimulation::setWaterHeight(float height)
{
    mParticleNode->setPosition(osg::Vec3f(0,0,height));
}

void RippleSimulation::clear()
{
    for (int i=0; i<mParticleSystem->numParticles(); ++i)
        mParticleSystem->destroyParticle(i);
}



}
