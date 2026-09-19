#ifndef OPENMW_COMPONENTS_SCENEUTIL_CLONE_H
#define OPENMW_COMPONENTS_SCENEUTIL_CLONE_H

#include <map>
#include <vector>

#include <osg/CopyOp>
#include <osg/Object>
#include <osg/UserDataContainer>

namespace osgParticle
{
    class ParticleProcessor;
    class ParticleSystem;
    class ParticleSystemUpdater;
}

namespace SceneUtil
{
    class Controller;

    /// @par Defines the cloning behaviour we need:
    /// * Assigns updated ParticleSystem pointers on cloned emitters and programs.
    /// * Deep copies RigGeometry and MorphGeometry so they can animate without affecting clones.
    /// @warning Avoid using this class directly. The safety of cloning operations depends on the copy flags and the
    /// objects involved. Consider using SceneManager::cloneNode for additional safety.
    /// @warning Do not use an object of this class for more than one copy operation.
    class CopyOp : public osg::CopyOp
    {
    public:
        CopyOp();

        virtual osgParticle::ParticleSystem* operator()(const osgParticle::ParticleSystem* partsys) const;
        virtual osgParticle::ParticleProcessor* operator()(const osgParticle::ParticleProcessor* processor) const;

        osg::Node* operator()(const osg::Node* node) const override;
        osg::Drawable* operator()(const osg::Drawable* drawable) const override;
        osg::Callback* operator()(const osg::Callback* callback) const override;

        osg::Node* getClonedNode(const osg::Node* original) const;
        void remapControllerTargets() const;

    private:
        // maps new pointers to their old pointers
        // a little messy, but I think this should be the most efficient way
        mutable std::map<osgParticle::ParticleProcessor*, const osgParticle::ParticleSystem*> mProcessorToOldPs;
        mutable std::map<osgParticle::ParticleSystemUpdater*, const osgParticle::ParticleSystem*> mUpdaterToOldPs;
        mutable std::map<const osgParticle::ParticleSystem*, osgParticle::ParticleSystem*> mOldPsToNewPs;
        mutable std::map<const osg::Node*, osg::Node*> mClonedNodes;
        mutable std::vector<Controller*> mControllersToRemap;
    };

    /// Pin @a tmpl's lifetime to @a derived's by parking it in derived's user data container. Used after a shallow
    /// copy so the template survives as long as anything copied from it, and so a cache holding the template sees it
    /// as still in use. osg::UserDataContainer::addUserObject only takes a non-const pointer, hence the const_cast;
    /// nothing ever reads the object back out, so it is never mutated through that pointer.
    inline void addTemplateRef(osg::Object& derived, const osg::Object* tmpl)
    {
        if (tmpl != nullptr)
            derived.getOrCreateUserDataContainer()->addUserObject(const_cast<osg::Object*>(tmpl));
    }

}

#endif
