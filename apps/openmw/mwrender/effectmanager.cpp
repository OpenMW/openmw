#include "effectmanager.hpp"

#include <osg/PositionAttitudeTransform>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/vismask.hpp>

#include "animation.hpp"
#include "util.hpp"

namespace MWRender
{

EffectManager::EffectManager(osg::ref_ptr<osg::Group> parent, Resource::ResourceSystem* resourceSystem)
    : mParentNode(parent)
    , mResourceSystem(resourceSystem)
{
}

EffectManager::~EffectManager()
{
    clear();
}

void EffectManager::addEffect(const std::string &model, const std::string& textureOverride, const osg::Vec3f &worldPosition, float scale, bool isMagicVFX)
{
    osg::ref_ptr<osg::Node> node = mResourceSystem->getSceneManager()->getInstance(model);

    node->setNodeMask(SceneUtil::Mask_Effect);

    Effect effect;
    effect.mAnimTime.reset(new EffectAnimationTime);

    SceneUtil::FindMaxControllerLengthVisitor findMaxLengthVisitor;
    node->accept(findMaxLengthVisitor);
    effect.mMaxControllerLength = findMaxLengthVisitor.getMaxLength();

    osg::ref_ptr<osg::PositionAttitudeTransform> trans = new osg::PositionAttitudeTransform;
    trans->setPosition(worldPosition);
    trans->setScale(osg::Vec3f(scale, scale, scale));
    trans->addChild(node);

    SceneUtil::AssignControllerSourcesVisitor assignVisitor(effect.mAnimTime);
    node->accept(assignVisitor);

    if (isMagicVFX)
        overrideFirstRootTexture(textureOverride, mResourceSystem, node);
    else
        overrideTexture(textureOverride, mResourceSystem, node);

    mParentNode->addChild(trans);

    mEffects[trans] = effect;
}

void EffectManager::update(float dt)
{
    for (EffectMap::iterator it = mEffects.begin(); it != mEffects.end(); )
    {
        it->second.mAnimTime->addTime(dt);

        if (it->second.mAnimTime->getTime() >= it->second.mMaxControllerLength)
        {
            mParentNode->removeChild(it->first);
            mEffects.erase(it++);
        }
        else
            ++it;
    }
}

void EffectManager::clear()
{
    for (EffectMap::iterator it = mEffects.begin(); it != mEffects.end(); ++it)
    {
        mParentNode->removeChild(it->first);
    }
    mEffects.clear();
}

}
