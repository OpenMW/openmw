#include "ripplesimulation.hpp"

#include <iomanip>

#include <osg/PolygonOffset>
#include <osg/Geode>
#include <osg/Texture2D>
#include <osg/Material>
#include <osg/Depth>
#include <osg/PositionAttitudeTransform>
#include <osgParticle/ParticleSystem>
#include <osgParticle/ParticleSystemUpdater>

#include <components/misc/rng.hpp>
#include <components/nifosg/controller.hpp>
#include <components/resource/texturemanager.hpp>
#include <components/resource/resourcesystem.hpp>

#include "vismask.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/fallback.hpp"

#include "../mwmechanics/actorutil.hpp"

namespace
{
    void createWaterRippleStateSet(Resource::ResourceSystem* resourceSystem, const MWWorld::Fallback* fallback, osg::Node* node)
    {
        int rippleFrameCount = fallback->getFallbackInt("Water_RippleFrameCount");
        if (rippleFrameCount <= 0)
            return;

        std::string tex = fallback->getFallbackString("Water_RippleTexture");

        std::vector<osg::ref_ptr<osg::Texture2D> > textures;
        for (int i=0; i<rippleFrameCount; ++i)
        {
            std::ostringstream texname;
            texname << "textures/water/" << tex << std::setw(2) << std::setfill('0') << i << ".dds";
            textures.push_back(resourceSystem->getTextureManager()->getTexture2D(texname.str(), osg::Texture::REPEAT, osg::Texture::REPEAT));
        }

        osg::ref_ptr<NifOsg::FlipController> controller (new NifOsg::FlipController(0, 0.3f/rippleFrameCount, textures));
        controller->setSource(boost::shared_ptr<SceneUtil::ControllerSource>(new SceneUtil::FrameTimeSource));
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
        mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 1.f));
        mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(1.f, 1.f, 1.f, 1.f));
        mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 0.f));
        mat->setColorMode(osg::Material::DIFFUSE);
        stateset->setAttributeAndModes(mat, osg::StateAttribute::ON);

        node->setStateSet(stateset);
    }
}

namespace MWRender
{

RippleSimulation::RippleSimulation(osg::Group *parent, Resource::ResourceSystem* resourceSystem, const MWWorld::Fallback* fallback)
    : mParent(parent)
{
    osg::ref_ptr<osg::Geode> geode (new osg::Geode);

    mParticleSystem = new osgParticle::ParticleSystem;
    geode->addDrawable(mParticleSystem);

    mParticleSystem->setParticleAlignment(osgParticle::ParticleSystem::FIXED);
    mParticleSystem->setAlignVectorX(osg::Vec3f(1,0,0));
    mParticleSystem->setAlignVectorY(osg::Vec3f(0,1,0));

    osgParticle::Particle& particleTemplate = mParticleSystem->getDefaultParticleTemplate();
    particleTemplate.setSizeRange(osgParticle::rangef(15, 180));
    particleTemplate.setColorRange(osgParticle::rangev4(osg::Vec4f(1,1,1,0.7), osg::Vec4f(1,1,1,0.7)));
    particleTemplate.setAlphaRange(osgParticle::rangef(1.f, 0.f));
    particleTemplate.setAngularVelocity(osg::Vec3f(0,0,fallback->getFallbackFloat("Water_RippleRotSpeed")));
    particleTemplate.setLifeTime(fallback->getFallbackFloat("Water_RippleLifetime"));

    osg::ref_ptr<osgParticle::ParticleSystemUpdater> updater (new osgParticle::ParticleSystemUpdater);
    updater->addParticleSystem(mParticleSystem);

    mParticleNode = new osg::PositionAttitudeTransform;
    mParticleNode->addChild(updater);
    mParticleNode->addChild(geode);
    mParticleNode->setNodeMask(Mask_Effect);

    createWaterRippleStateSet(resourceSystem, fallback, mParticleNode);

    mParent->addChild(mParticleNode);
}

RippleSimulation::~RippleSimulation()
{
    mParent->removeChild(mParticleNode);
}

void RippleSimulation::update(float dt)
{
    for (std::vector<Emitter>::iterator it=mEmitters.begin(); it !=mEmitters.end(); ++it)
    {
        if (it->mPtr == MWBase::Environment::get().getWorld ()->getPlayerPtr())
        {
            // fetch a new ptr (to handle cell change etc)
            // for non-player actors this is done in updateObjectCell
            it->mPtr = MWBase::Environment::get().getWorld ()->getPlayerPtr();
        }

        osg::Vec3f currentPos (it->mPtr.getRefData().getPosition().asVec3());
        currentPos.z() = 0; // Z is set by the Scene Node

        if ( (currentPos - it->mLastEmitPosition).length() > 10
             // Only emit when close to the water surface, not above it and not too deep in the water
            && MWBase::Environment::get().getWorld ()->isUnderwater (it->mPtr.getCell(), it->mPtr.getRefData().getPosition().asVec3())
             && !MWBase::Environment::get().getWorld()->isSubmerged(it->mPtr))
        {
            it->mLastEmitPosition = currentPos;

            if (mParticleSystem->numParticles()-mParticleSystem->numDeadParticles() > 500)
                continue; // TODO: remove the oldest particle to make room?

            osgParticle::Particle* p = mParticleSystem->createParticle(NULL);
            p->setPosition(currentPos);
            p->setAngle(osg::Vec3f(0,0, Misc::Rng::rollProbability() * osg::PI * 2 - osg::PI));
        }
    }
}


void RippleSimulation::addEmitter(const MWWorld::Ptr& ptr, float scale, float force)
{
    Emitter newEmitter;
    newEmitter.mPtr = ptr;
    newEmitter.mScale = scale;
    newEmitter.mForce = force;
    newEmitter.mLastEmitPosition = osg::Vec3f(0,0,0);
    mEmitters.push_back (newEmitter);
}

void RippleSimulation::removeEmitter (const MWWorld::Ptr& ptr)
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

void RippleSimulation::updateEmitterPtr (const MWWorld::Ptr& old, const MWWorld::Ptr& ptr)
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
        if (it->mPtr.getCell() == store && it->mPtr != MWMechanics::getPlayer())
        {
            it = mEmitters.erase(it);
        }
        else
            ++it;
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
