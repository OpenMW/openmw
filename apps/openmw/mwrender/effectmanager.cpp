#include "effectmanager.hpp"

#include <osg/PositionAttitudeTransform>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/sceneutil/controller.hpp>

#include "animation.hpp"
#include "util.hpp"
#include "vismask.hpp"

#include <algorithm>

namespace MWRender
{

    EffectManager::EffectManager(osg::ref_ptr<osg::Group> parent, Resource::ResourceSystem* resourceSystem)
        : mParentNode(std::move(parent))
        , mResourceSystem(resourceSystem)
    {
    }

    EffectManager::~EffectManager()
    {
        clear();
    }

    void EffectManager::addEffect(VFS::Path::NormalizedView model, std::string_view textureOverride,
        const osg::Vec3f& worldPosition, float scale, bool isMagicVFX, bool useAmbientLight, std::string_view effectId,
        bool loop)
    {
        osg::ref_ptr<osg::Node> node = mResourceSystem->getSceneManager()->getInstance(model);

        node->setNodeMask(Mask_Effect);

        Effect effect;
        effect.mAnimTime = std::make_shared<EffectAnimationTime>();
        effect.mLoop = loop;
        effect.mEffectId = effectId;

        SceneUtil::FindMaxControllerLengthVisitor findMaxLengthVisitor;
        node->accept(findMaxLengthVisitor);
        effect.mMaxControllerLength = findMaxLengthVisitor.getMaxLength();

        osg::ref_ptr<osg::PositionAttitudeTransform> trans = new osg::PositionAttitudeTransform;
        trans->setPosition(worldPosition);
        trans->setScale(osg::Vec3f(scale, scale, scale));
        trans->addChild(node);

        effect.mTransform = trans;

        SceneUtil::AssignControllerSourcesVisitor assignVisitor(effect.mAnimTime);
        node->accept(assignVisitor);

        if (isMagicVFX)
            overrideFirstRootTexture(VFS::Path::toNormalized(textureOverride), mResourceSystem, *node);
        else
            overrideTexture(VFS::Path::toNormalized(textureOverride), mResourceSystem, *node);

        mParentNode->addChild(trans);

        if (useAmbientLight)
        {
            // Morrowind has a white ambient light attached to the root VFX node of the scenegraph
            node->getOrCreateStateSet()->setAttributeAndModes(
                getVFXLightModelInstance(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        }

        mResourceSystem->getSceneManager()->setUpNormalsRTForStateSet(node->getOrCreateStateSet(), false);

        mEffects.push_back(std::move(effect));
    }

    void EffectManager::removeEffect(std::string_view effectId)
    {
        mEffects.erase(std::remove_if(mEffects.begin(), mEffects.end(),
                           [effectId, this](Effect& effect) {
                               if (effectId == effect.mEffectId)
                               {
                                   mParentNode->removeChild(effect.mTransform);
                                   return true;
                               }

                               return false;
                           }),
            mEffects.end());
    }

    void EffectManager::update(float dt)
    {
        mEffects.erase(std::remove_if(mEffects.begin(), mEffects.end(),
                           [dt, this](Effect& effect) {
                               bool remove = false;
                               effect.mAnimTime->addTime(dt);
                               if (effect.mAnimTime->getTime() >= effect.mMaxControllerLength)
                               {
                                   if (effect.mLoop)
                                   {
                                       float remainder = effect.mAnimTime->getTime() - effect.mMaxControllerLength;
                                       effect.mAnimTime->resetTime(remainder);
                                   }
                                   else
                                   {
                                       mParentNode->removeChild(effect.mTransform);
                                       remove = true;
                                   }
                               }

                               return remove;
                           }),
            mEffects.end());
    }

    void EffectManager::clear()
    {
        for (const auto& effect : mEffects)
        {
            mParentNode->removeChild(effect.mTransform);
        }
        mEffects.clear();
    }

}
