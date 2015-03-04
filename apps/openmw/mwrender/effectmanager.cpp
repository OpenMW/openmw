#include "effectmanager.hpp"

#include <components/misc/resourcehelpers.hpp>

#include <OgreSceneManager.h>
#include <OgreParticleSystem.h>
#include <OgreSceneNode.h>
#include <OgreTechnique.h>

#include "animation.hpp"
#include "renderconst.hpp"

namespace MWRender
{

EffectManager::EffectManager(Ogre::SceneManager *sceneMgr)
    : mSceneMgr(sceneMgr)
{
}

void EffectManager::addEffect(const std::string &model, std::string textureOverride, const Ogre::Vector3 &worldPosition, float scale)
{
    Ogre::SceneNode* sceneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(worldPosition);
    sceneNode->setScale(scale,scale,scale);

    NifOgre::ObjectScenePtr scene = NifOgre::Loader::createObjects(sceneNode, model);

    MWRender::Animation::setRenderProperties(scene, RV_Effects,
                        RQG_Main, RQG_Alpha, 0.f, false, NULL);

    for(size_t i = 0;i < scene->mControllers.size();i++)
    {
        if(scene->mControllers[i].getSource().isNull())
            scene->mControllers[i].setSource(Ogre::SharedPtr<EffectAnimationTime> (new EffectAnimationTime()));
    }

    if (!textureOverride.empty())
    {
        std::string correctedTexture = Misc::ResourceHelpers::correctTexturePath(textureOverride);
        for(size_t i = 0;i < scene->mParticles.size(); ++i)
        {
            Ogre::ParticleSystem* partSys = scene->mParticles[i];

            Ogre::MaterialPtr mat = scene->mMaterialControllerMgr.getWritableMaterial(partSys);

            for (int t=0; t<mat->getNumTechniques(); ++t)
            {
                Ogre::Technique* tech = mat->getTechnique(t);
                for (int p=0; p<tech->getNumPasses(); ++p)
                {
                    Ogre::Pass* pass = tech->getPass(p);
                    for (int tex=0; tex<pass->getNumTextureUnitStates(); ++tex)
                    {
                        Ogre::TextureUnitState* tus = pass->getTextureUnitState(tex);
                        tus->setTextureName(correctedTexture);
                    }
                }
            }
        }
    }

    mEffects.push_back(std::make_pair(sceneNode, scene));
}

void EffectManager::update(float dt, Ogre::Camera* camera)
{
    for (std::vector<std::pair<Ogre::SceneNode*, NifOgre::ObjectScenePtr> >::iterator it = mEffects.begin(); it != mEffects.end(); )
    {
        NifOgre::ObjectScenePtr objects = it->second;
        for(size_t i = 0; i < objects->mControllers.size() ;i++)
        {
            EffectAnimationTime* value = dynamic_cast<EffectAnimationTime*>(objects->mControllers[i].getSource().get());
            if (value)
                value->addTime(dt);

            objects->mControllers[i].update();
        }
        objects->rotateBillboardNodes(camera);

        // Finished playing?
        if (objects->mControllers[0].getSource()->getValue() >= objects->mMaxControllerLength)
        {
            Ogre::SceneNode* node = it->first;
            it = mEffects.erase(it);
            mSceneMgr->destroySceneNode(node);
            continue;
        }
        ++it;
    }
}

void EffectManager::clear()
{
    for (std::vector<std::pair<Ogre::SceneNode*, NifOgre::ObjectScenePtr> >::iterator it = mEffects.begin(); it != mEffects.end(); )
    {
        Ogre::SceneNode* node = it->first;
        it = mEffects.erase(it);
        mSceneMgr->destroySceneNode(node);
    }
}

}
