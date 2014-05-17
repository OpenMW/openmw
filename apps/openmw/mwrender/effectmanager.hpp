#ifndef OPENMW_MWRENDER_EFFECTMANAGER_H
#define OPENMW_MWRENDER_EFFECTMANAGER_H

#include <components/nifogre/ogrenifloader.hpp>

namespace MWRender
{

    class EffectAnimationTime : public Ogre::ControllerValue<Ogre::Real>
    {
    private:
        float mTime;
    public:
        EffectAnimationTime() : mTime(0) {  }
        void addTime(float time) { mTime += time; }

        virtual Ogre::Real getValue() const { return mTime; }
        virtual void setValue(Ogre::Real value) {}
    };


    // Note: effects attached to another object should be managed by MWRender::Animation::addEffect.
    // This class manages "free" effects, i.e. attached to a dedicated scene node in the world.
    class EffectManager
    {
    public:
        EffectManager(Ogre::SceneManager* sceneMgr);
        ~EffectManager() { clear(); }

        /// Add an effect. When it's finished playing, it will be removed automatically.
        void addEffect (const std::string& model, std::string textureOverride, const Ogre::Vector3& worldPosition, float scale);

        void update(float dt, Ogre::Camera* camera);

        /// Remove all effects
        void clear();

    private:
        std::vector<std::pair<Ogre::SceneNode*, NifOgre::ObjectScenePtr> > mEffects;
        Ogre::SceneManager* mSceneMgr;
    };

}

#endif
