#include "animation.hpp"

#include <algorithm>
#include <iomanip>
#include <limits>

#include <osg/BlendFunc>
#include <osg/LightModel>
#include <osg/Material>
#include <osg/MatrixTransform>
#include <osg/Switch>

#include <osgParticle/ParticleProcessor>
#include <osgParticle/ParticleSystem>

#include <osgAnimation/Bone>
#include <osgAnimation/UpdateBone>

#include <components/debug/debuglog.hpp>

#include <components/resource/animblendrulesmanager.hpp>
#include <components/resource/keyframemanager.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/esm3/loadcont.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadrace.hpp>
#include <components/esm4/loadligh.hpp>

#include <components/misc/constants.hpp>
#include <components/misc/pathhelpers.hpp>
#include <components/misc/resourcehelpers.hpp>

#include <components/vfs/manager.hpp>
#include <components/vfs/pathutil.hpp>
#include <components/vfs/recursivedirectoryiterator.hpp>

#include <components/sceneutil/keyframe.hpp>
#include <components/sceneutil/lightcommon.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/lightutil.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/skeleton.hpp>
#include <components/sceneutil/statesetupdater.hpp>
#include <components/sceneutil/util.hpp>
#include <components/sceneutil/visitor.hpp>

#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/character.hpp" // FIXME: for MWMechanics::Priority
#include "../mwmechanics/weapontype.hpp"

#include "actorutil.hpp"
#include "rotatecontroller.hpp"
#include "util.hpp"
#include "vismask.hpp"

namespace
{
    class MarkDrawablesVisitor : public osg::NodeVisitor
    {
    public:
        MarkDrawablesVisitor(osg::Node::NodeMask mask)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mMask(mask)
        {
        }

        void apply(osg::Drawable& drawable) override { drawable.setNodeMask(mMask); }

    private:
        osg::Node::NodeMask mMask = 0;
    };

    /// Removes all particle systems and related nodes in a subgraph.
    class RemoveParticlesVisitor : public osg::NodeVisitor
    {
    public:
        RemoveParticlesVisitor()
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        {
        }

        void apply(osg::Node& node) override
        {
            if (dynamic_cast<osgParticle::ParticleProcessor*>(&node))
                mToRemove.emplace_back(&node);

            traverse(node);
        }

        void apply(osg::Drawable& drw) override
        {
            if (osgParticle::ParticleSystem* partsys = dynamic_cast<osgParticle::ParticleSystem*>(&drw))
                mToRemove.emplace_back(partsys);
        }

        void remove()
        {
            for (osg::Node* node : mToRemove)
            {
                // FIXME: a Drawable might have more than one parent
                if (node->getNumParents())
                    node->getParent(0)->removeChild(node);
            }
            mToRemove.clear();
        }

    private:
        std::vector<osg::ref_ptr<osg::Node>> mToRemove;
    };

    class DayNightCallback : public SceneUtil::NodeCallback<DayNightCallback, osg::Switch*>
    {
    public:
        DayNightCallback()
            : mCurrentState(0)
        {
        }

        void operator()(osg::Switch* node, osg::NodeVisitor* nv)
        {
            unsigned int state = MWBase::Environment::get().getWorld()->getNightDayMode();
            const unsigned int newState = node->getNumChildren() > state ? state : 0;

            if (newState != mCurrentState)
            {
                mCurrentState = newState;
                node->setSingleChildOn(mCurrentState);
            }

            traverse(node, nv);
        }

    private:
        unsigned int mCurrentState;
    };

    class AddSwitchCallbacksVisitor : public osg::NodeVisitor
    {
    public:
        AddSwitchCallbacksVisitor()
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        {
        }

        void apply(osg::Switch& switchNode) override
        {
            if (switchNode.getName() == Constants::NightDayLabel)
                switchNode.addUpdateCallback(new DayNightCallback());

            traverse(switchNode);
        }
    };

    class HarvestVisitor : public osg::NodeVisitor
    {
    public:
        HarvestVisitor()
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        {
        }

        void apply(osg::Switch& node) override
        {
            if (node.getName() == Constants::HerbalismLabel)
            {
                node.setSingleChildOn(1);
            }

            traverse(node);
        }
    };

    bool equalsParts(std::string_view value, std::string_view s1, std::string_view s2, std::string_view s3 = {})
    {
        if (value.starts_with(s1))
        {
            value = value.substr(s1.size());
            if (value.starts_with(s2))
                return value.substr(s2.size()) == s3;
        }
        return false;
    }

    float calcAnimVelocity(const SceneUtil::TextKeyMap& keys, SceneUtil::KeyframeController* nonaccumctrl,
        const osg::Vec3f& accum, std::string_view groupname)
    {
        float starttime = std::numeric_limits<float>::max();
        float stoptime = 0.0f;

        // Pick the last Loop Stop key and the last Loop Start key.
        // This is required because of broken text keys in AshVampire.nif.
        // It has *two* WalkForward: Loop Stop keys at different times, the first one is used for stopping playback
        // but the animation velocity calculation uses the second one.
        // As result the animation velocity calculation is not correct, and this incorrect velocity must be replicated,
        // because otherwise the Creature's Speed (dagoth uthol) would not be sufficient to move fast enough.
        auto keyiter = keys.rbegin();
        while (keyiter != keys.rend())
        {
            if (equalsParts(keyiter->second, groupname, ": start")
                || equalsParts(keyiter->second, groupname, ": loop start"))
            {
                starttime = keyiter->first;
                break;
            }
            ++keyiter;
        }
        keyiter = keys.rbegin();
        while (keyiter != keys.rend())
        {
            if (equalsParts(keyiter->second, groupname, ": stop"))
                stoptime = keyiter->first;
            else if (equalsParts(keyiter->second, groupname, ": loop stop"))
            {
                stoptime = keyiter->first;
                break;
            }
            ++keyiter;
        }

        if (stoptime > starttime)
        {
            osg::Vec3f startpos = osg::componentMultiply(nonaccumctrl->getTranslation(starttime), accum);
            osg::Vec3f endpos = osg::componentMultiply(nonaccumctrl->getTranslation(stoptime), accum);

            return (startpos - endpos).length() / (stoptime - starttime);
        }

        return 0.0f;
    }

    class GetExtendedBonesVisitor : public osg::NodeVisitor
    {
    public:
        GetExtendedBonesVisitor()
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        {
        }

        void apply(osg::Node& node) override
        {
            if (SceneUtil::hasUserDescription(&node, "CustomBone"))
            {
                mFoundBones.emplace_back(&node, node.getParent(0));
                return;
            }

            traverse(node);
        }

        std::vector<std::pair<osg::Node*, osg::Group*>> mFoundBones;
    };

    class RemoveFinishedCallbackVisitor : public SceneUtil::RemoveVisitor
    {
    public:
        bool mHasMagicEffects;

        RemoveFinishedCallbackVisitor()
            : RemoveVisitor()
            , mHasMagicEffects(false)
        {
        }

        void apply(osg::Node& node) override { traverse(node); }

        void apply(osg::Group& group) override
        {
            traverse(group);

            osg::Callback* callback = group.getUpdateCallback();
            if (callback)
            {
                // We should remove empty transformation nodes and finished callbacks here
                MWRender::UpdateVfxCallback* vfxCallback = dynamic_cast<MWRender::UpdateVfxCallback*>(callback);
                if (vfxCallback)
                {
                    if (vfxCallback->mFinished)
                        mToRemove.emplace_back(group.asNode(), group.getParent(0));
                    else
                        mHasMagicEffects = true;
                }
            }
        }

        void apply(osg::MatrixTransform& node) override { traverse(node); }

        void apply(osg::Geometry&) override {}
    };

    class RemoveCallbackVisitor : public SceneUtil::RemoveVisitor
    {
    public:
        bool mHasMagicEffects;

        RemoveCallbackVisitor()
            : RemoveVisitor()
            , mHasMagicEffects(false)
        {
        }

        RemoveCallbackVisitor(std::string_view effectId)
            : RemoveVisitor()
            , mHasMagicEffects(false)
            , mEffectId(effectId)
        {
        }

        void apply(osg::Node& node) override { traverse(node); }

        void apply(osg::Group& group) override
        {
            traverse(group);

            osg::Callback* callback = group.getUpdateCallback();
            if (callback)
            {
                MWRender::UpdateVfxCallback* vfxCallback = dynamic_cast<MWRender::UpdateVfxCallback*>(callback);
                if (vfxCallback)
                {
                    bool toRemove = mEffectId == "" || vfxCallback->mParams.mEffectId == mEffectId;
                    if (toRemove)
                        mToRemove.emplace_back(group.asNode(), group.getParent(0));
                    else
                        mHasMagicEffects = true;
                }
            }
        }

        void apply(osg::MatrixTransform& node) override { traverse(node); }

        void apply(osg::Geometry&) override {}

    private:
        std::string_view mEffectId;
    };

    class FindVfxCallbacksVisitor : public osg::NodeVisitor
    {
    public:
        std::vector<MWRender::UpdateVfxCallback*> mCallbacks;

        FindVfxCallbacksVisitor()
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        {
        }

        FindVfxCallbacksVisitor(std::string_view effectId)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mEffectId(effectId)
        {
        }

        void apply(osg::Node& node) override { traverse(node); }

        void apply(osg::Group& group) override
        {
            osg::Callback* callback = group.getUpdateCallback();
            if (callback)
            {
                MWRender::UpdateVfxCallback* vfxCallback = dynamic_cast<MWRender::UpdateVfxCallback*>(callback);
                if (vfxCallback)
                {
                    if (mEffectId == "" || vfxCallback->mParams.mEffectId == mEffectId)
                    {
                        mCallbacks.push_back(vfxCallback);
                    }
                }
            }
            traverse(group);
        }

        void apply(osg::MatrixTransform& node) override { traverse(node); }

        void apply(osg::Geometry&) override {}

    private:
        std::string_view mEffectId;
    };

    namespace
    {
        osg::ref_ptr<osg::LightModel> makeVFXLightModelInstance()
        {
            osg::ref_ptr<osg::LightModel> lightModel = new osg::LightModel;
            lightModel->setAmbientIntensity({ 1, 1, 1, 1 });
            return lightModel;
        }

        const osg::ref_ptr<osg::LightModel>& getVFXLightModelInstance()
        {
            static const osg::ref_ptr<osg::LightModel> lightModel = makeVFXLightModelInstance();
            return lightModel;
        }
    }

    void assignBoneBlendCallbackRecursive(MWRender::BoneAnimBlendController* controller, osg::Node* parent, bool isRoot)
    {
        // Attempt to cast node to an osgAnimation::Bone
        if (!isRoot && dynamic_cast<osgAnimation::Bone*>(parent))
        {
            // Wrapping in a custom callback object allows for nested callback chaining, otherwise it has link to self
            // issues we need to share the base BoneAnimBlendController as that contains blending information and is
            // guaranteed to update before
            osgAnimation::Bone* bone = static_cast<osgAnimation::Bone*>(parent);
            osg::ref_ptr<osg::Callback> cb = new MWRender::BoneAnimBlendControllerWrapper(controller, bone);

            // Ensure there is no other AnimBlendController - this can happen when using
            // multiple animations with different roots, such as NPC animation
            osg::Callback* updateCb = bone->getUpdateCallback();
            while (updateCb)
            {
                if (dynamic_cast<MWRender::BoneAnimBlendController*>(updateCb))
                {
                    osg::ref_ptr<osg::Callback> nextCb = updateCb->getNestedCallback();
                    bone->removeUpdateCallback(updateCb);
                    updateCb = nextCb;
                }
                else
                {
                    updateCb = updateCb->getNestedCallback();
                }
            }

            // Find UpdateBone callback and bind to just after that (order is important)
            // NOTE: if it doesn't have an UpdateBone callback, we shouldn't be doing blending!
            updateCb = bone->getUpdateCallback();
            while (updateCb)
            {
                if (dynamic_cast<osgAnimation::UpdateBone*>(updateCb))
                {
                    // Override the immediate callback after the UpdateBone
                    osg::ref_ptr<osg::Callback> lastCb = updateCb->getNestedCallback();
                    updateCb->setNestedCallback(cb);
                    if (lastCb)
                        cb->setNestedCallback(lastCb);
                    break;
                }

                updateCb = updateCb->getNestedCallback();
            }
        }

        // Traverse child bones if this is a group
        osg::Group* group = parent->asGroup();
        if (group)
            for (unsigned int i = 0; i < group->getNumChildren(); ++i)
                assignBoneBlendCallbackRecursive(controller, group->getChild(i), false);
    }
}

namespace MWRender
{
    class TransparencyUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        TransparencyUpdater(const float alpha)
            : mAlpha(alpha)
        {
        }

        void setAlpha(const float alpha) { mAlpha = alpha; }

    protected:
        void setDefaults(osg::StateSet* stateset) override
        {
            osg::BlendFunc* blendfunc(new osg::BlendFunc);
            stateset->setAttributeAndModes(blendfunc, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

            stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
            stateset->setRenderBinMode(osg::StateSet::OVERRIDE_RENDERBIN_DETAILS);

            // FIXME: overriding diffuse/ambient/emissive colors
            osg::Material* material = new osg::Material;
            material->setColorMode(osg::Material::OFF);
            material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(1, 1, 1, mAlpha));
            material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1, 1, 1, 1));
            stateset->setAttributeAndModes(material, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            stateset->addUniform(
                new osg::Uniform("colorMode", 0), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        }

        void apply(osg::StateSet* stateset, osg::NodeVisitor* /*nv*/) override
        {
            osg::Material* material
                = static_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
            material->setAlpha(osg::Material::FRONT_AND_BACK, mAlpha);
        }

    private:
        float mAlpha;
    };

    struct Animation::AnimSource
    {
        osg::ref_ptr<const SceneUtil::KeyframeHolder> mKeyframes;

        typedef std::map<std::string, osg::ref_ptr<SceneUtil::KeyframeController>> ControllerMap;

        ControllerMap mControllerMap[sNumBlendMasks];

        const SceneUtil::TextKeyMap& getTextKeys() const;

        osg::ref_ptr<const SceneUtil::AnimBlendRules> mAnimBlendRules;
    };

    void UpdateVfxCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        traverse(node, nv);

        if (mFinished)
            return;

        double newTime = nv->getFrameStamp()->getSimulationTime();
        if (mStartingTime == 0)
        {
            mStartingTime = newTime;
            return;
        }

        double duration = newTime - mStartingTime;
        mStartingTime = newTime;

        mParams.mAnimTime->addTime(duration);
        if (mParams.mAnimTime->getTime() >= mParams.mMaxControllerLength)
        {
            if (mParams.mLoop)
            {
                // Start from the beginning again; carry over the remainder
                // Not sure if this is actually needed, the controller function might already handle loops
                float remainder = mParams.mAnimTime->getTime() - mParams.mMaxControllerLength;
                mParams.mAnimTime->resetTime(remainder);
            }
            else
            {
                // Hide effect immediately
                node->setNodeMask(0);
                mFinished = true;
            }
        }
    }

    class ResetAccumRootCallback : public SceneUtil::NodeCallback<ResetAccumRootCallback, osg::MatrixTransform*>
    {
    public:
        void operator()(osg::MatrixTransform* transform, osg::NodeVisitor* nv)
        {
            osg::Matrix mat = transform->getMatrix();
            osg::Vec3f position = mat.getTrans();
            position = osg::componentMultiply(mResetAxes, position);
            mat.setTrans(position);
            transform->setMatrix(mat);

            traverse(transform, nv);
        }

        void setAccumulate(const osg::Vec3f& accumulate)
        {
            // anything that accumulates (1.f) should be reset in the callback to (0.f)
            mResetAxes.x() = accumulate.x() != 0.f ? 0.f : 1.f;
            mResetAxes.y() = accumulate.y() != 0.f ? 0.f : 1.f;
            mResetAxes.z() = accumulate.z() != 0.f ? 0.f : 1.f;
        }

    private:
        osg::Vec3f mResetAxes;
    };

    Animation::Animation(
        const MWWorld::Ptr& ptr, osg::ref_ptr<osg::Group> parentNode, Resource::ResourceSystem* resourceSystem)
        : mInsert(std::move(parentNode))
        , mSkeleton(nullptr)
        , mNodeMapCreated(false)
        , mPtr(ptr)
        , mResourceSystem(resourceSystem)
        , mAccumulate(1.f, 1.f, 0.f)
        , mTextKeyListener(nullptr)
        , mHeadYawRadians(0.f)
        , mHeadPitchRadians(0.f)
        , mUpperBodyYawRadians(0.f)
        , mLegsYawRadians(0.f)
        , mBodyPitchRadians(0.f)
        , mHasMagicEffects(false)
        , mAlpha(1.f)
        , mPlayScriptedOnly(false)
        , mRequiresBoneMap(false)
    {
        for (size_t i = 0; i < sNumBlendMasks; i++)
            mAnimationTimePtr[i] = std::make_shared<AnimationTime>();

        mLightListCallback = new SceneUtil::LightListCallback;
    }

    Animation::~Animation()
    {
        removeFromSceneImpl();
    }

    void Animation::setActive(int active)
    {
        if (mSkeleton)
            mSkeleton->setActive(static_cast<SceneUtil::Skeleton::ActiveType>(active));
    }

    void Animation::updatePtr(const MWWorld::Ptr& ptr)
    {
        mPtr = ptr;
    }

    void Animation::setAccumulation(const osg::Vec3f& accum)
    {
        mAccumulate = accum;

        if (mResetAccumRootCallback)
            mResetAccumRootCallback->setAccumulate(mAccumulate);
    }

    // controllerName is used for Collada animated deforming models
    size_t Animation::detectBlendMask(const osg::Node* node, const std::string& controllerName) const
    {
        static const std::string_view sBlendMaskRoots[sNumBlendMasks] = {
            "", /* Lower body / character root */
            "Bip01 Spine1", /* Torso */
            "Bip01 L Clavicle", /* Left arm */
            "Bip01 R Clavicle", /* Right arm */
        };

        while (node != mObjectRoot)
        {
            const std::string& name = node->getName();
            for (size_t i = 1; i < sNumBlendMasks; i++)
            {
                if (name == sBlendMaskRoots[i] || controllerName == sBlendMaskRoots[i])
                    return i;
            }

            assert(node->getNumParents() > 0);

            node = node->getParent(0);
        }

        return 0;
    }

    const SceneUtil::TextKeyMap& Animation::AnimSource::getTextKeys() const
    {
        return mKeyframes->mTextKeys;
    }

    void Animation::loadAdditionalAnimations(VFS::Path::NormalizedView model, const std::string& baseModel)
    {
        constexpr VFS::Path::NormalizedView meshes("meshes/");
        if (!model.value().starts_with(meshes.value()))
            return;

        std::string path(model.value());

        constexpr VFS::Path::NormalizedView animations("animations/");
        path.replace(0, meshes.value().size(), animations.value());

        const std::string::size_type extensionStart = path.find_last_of(VFS::Path::extensionSeparator);
        if (extensionStart == std::string::npos)
            return;

        path.replace(extensionStart, path.size() - extensionStart, "/");

        for (const VFS::Path::Normalized& name : mResourceSystem->getVFS()->getRecursiveDirectoryIterator(path))
        {
            if (Misc::getFileExtension(name) == "kf")
            {
                addSingleAnimSource(name, baseModel);
            }
        }
    }

    void Animation::addAnimSource(std::string_view model, const std::string& baseModel)
    {
        VFS::Path::Normalized kfname(model);

        if (Misc::getFileExtension(kfname) == "nif")
            kfname.changeExtension("kf");

        addSingleAnimSource(kfname, baseModel);

        if (Settings::game().mUseAdditionalAnimSources)
            loadAdditionalAnimations(kfname, baseModel);
    }

    std::shared_ptr<Animation::AnimSource> Animation::addSingleAnimSource(
        const std::string& kfname, const std::string& baseModel)
    {
        if (!mResourceSystem->getVFS()->exists(kfname))
            return nullptr;

        auto animsrc = std::make_shared<AnimSource>();
        animsrc->mKeyframes = mResourceSystem->getKeyframeManager()->get(VFS::Path::toNormalized(kfname));

        if (!animsrc->mKeyframes || animsrc->mKeyframes->mTextKeys.empty()
            || animsrc->mKeyframes->mKeyframeControllers.empty())
            return nullptr;

        const NodeMap& nodeMap = getNodeMap();
        const auto& controllerMap = animsrc->mKeyframes->mKeyframeControllers;
        for (SceneUtil::KeyframeHolder::KeyframeControllerMap::const_iterator it = controllerMap.begin();
             it != controllerMap.end(); ++it)
        {
            std::string bonename = Misc::StringUtils::lowerCase(it->first);
            NodeMap::const_iterator found = nodeMap.find(bonename);
            if (found == nodeMap.end())
            {
                Log(Debug::Warning) << "Warning: addAnimSource: can't find bone '" + bonename << "' in " << baseModel
                                    << " (referenced by " << kfname << ")";
                continue;
            }

            osg::Node* node = found->second;

            size_t blendMask = detectBlendMask(node, it->second->getName());

            // clone the controller, because each Animation needs its own ControllerSource
            osg::ref_ptr<SceneUtil::KeyframeController> cloned
                = osg::clone(it->second.get(), osg::CopyOp::SHALLOW_COPY);
            cloned->setSource(mAnimationTimePtr[blendMask]);

            animsrc->mControllerMap[blendMask].insert(std::make_pair(bonename, cloned));
        }

        mAnimSources.push_back(animsrc);

        for (const std::string& group : mAnimSources.back()->getTextKeys().getGroups())
            mSupportedAnimations.insert(group);

        SceneUtil::AssignControllerSourcesVisitor assignVisitor(mAnimationTimePtr[0]);
        mObjectRoot->accept(assignVisitor);

        // Determine the movement accumulation bone if necessary
        if (!mAccumRoot)
        {
            // Priority matters! bip01 is preferred.
            static const std::initializer_list<std::string_view> accumRootNames = { "bip01", "root bone" };
            NodeMap::const_iterator found = nodeMap.end();
            for (const std::string_view& name : accumRootNames)
            {
                found = nodeMap.find(name);
                if (found == nodeMap.end())
                    continue;
                for (SceneUtil::KeyframeHolder::KeyframeControllerMap::const_iterator it = controllerMap.begin();
                     it != controllerMap.end(); ++it)
                {
                    if (Misc::StringUtils::ciEqual(it->first, name))
                    {
                        mAccumRoot = found->second;
                        break;
                    }
                }
                if (mAccumRoot)
                    break;
            }
        }

        // Get the blending rules
        if (Settings::game().mSmoothAnimTransitions)
        {
            // Note, even if the actual config is .json - we should send a .yaml path to AnimBlendRulesManager, the
            // manager will check for .json if it will not find a specified .yaml file.
            VFS::Path::Normalized blendConfigPath(kfname);
            blendConfigPath.changeExtension("yaml");

            // globalBlendConfigPath is only used with actors! Objects have no default blending.
            constexpr VFS::Path::NormalizedView globalBlendConfigPath("animations/animation-config.yaml");

            osg::ref_ptr<const SceneUtil::AnimBlendRules> blendRules;
            if (mPtr.getClass().isActor())
            {
                blendRules
                    = mResourceSystem->getAnimBlendRulesManager()->getRules(globalBlendConfigPath, blendConfigPath);
                if (blendRules == nullptr)
                    Log(Debug::Warning) << "Unable to find animation blending rules: '" << blendConfigPath << "' or '"
                                        << globalBlendConfigPath << "'";
            }
            else
            {
                blendRules = mResourceSystem->getAnimBlendRulesManager()->getRules(blendConfigPath, blendConfigPath);
            }

            // At this point blendRules will either be nullptr or an AnimBlendRules instance with > 0 rules inside.
            animsrc->mAnimBlendRules = blendRules;
        }

        return animsrc;
    }

    void Animation::clearAnimSources()
    {
        mStates.clear();

        for (size_t i = 0; i < sNumBlendMasks; i++)
            mAnimationTimePtr[i]->setTimePtr(std::shared_ptr<float>());

        mAccumCtrl = nullptr;

        mSupportedAnimations.clear();
        mAnimSources.clear();

        mAnimVelocities.clear();
    }

    bool Animation::hasAnimation(std::string_view anim) const
    {
        return mSupportedAnimations.find(anim) != mSupportedAnimations.end();
    }

    bool Animation::isLoopingAnimation(std::string_view group) const
    {
        // In Morrowind, a some animation groups are always considered looping, regardless
        // of loop start/stop keys.
        // To be match vanilla behavior we probably only need to check this list, but we don't
        // want to prevent modded animations with custom group names from looping either.
        static const std::unordered_set<std::string_view> loopingAnimations = { "walkforward", "walkback", "walkleft",
            "walkright", "swimwalkforward", "swimwalkback", "swimwalkleft", "swimwalkright", "runforward", "runback",
            "runleft", "runright", "swimrunforward", "swimrunback", "swimrunleft", "swimrunright", "sneakforward",
            "sneakback", "sneakleft", "sneakright", "turnleft", "turnright", "swimturnleft", "swimturnright",
            "spellturnleft", "spellturnright", "torch", "idle", "idle2", "idle3", "idle4", "idle5", "idle6", "idle7",
            "idle8", "idle9", "idlesneak", "idlestorm", "idleswim", "jump", "inventoryhandtohand",
            "inventoryweapononehand", "inventoryweapontwohand", "inventoryweapontwowide" };
        static const std::vector<std::string_view> shortGroups = MWMechanics::getAllWeaponTypeShortGroups();

        if (getTextKeyTime(std::string(group) + ": loop start") >= 0)
            return true;

        // Most looping animations have variants for each weapon type shortgroup.
        // Just remove the shortgroup instead of enumerating all of the possible animation groupnames.
        // Make sure we pick the longest shortgroup so e.g. "bow" doesn't get picked over "crossbow"
        // when the shortgroup is crossbow.
        std::size_t suffixLength = 0;
        for (std::string_view suffix : shortGroups)
        {
            if (suffix.length() > suffixLength && group.ends_with(suffix))
            {
                suffixLength = suffix.length();
            }
        }
        group.remove_suffix(suffixLength);

        return loopingAnimations.count(group) > 0;
    }

    float Animation::getStartTime(const std::string& groupname) const
    {
        for (AnimSourceList::const_reverse_iterator iter(mAnimSources.rbegin()); iter != mAnimSources.rend(); ++iter)
        {
            const SceneUtil::TextKeyMap& keys = (*iter)->getTextKeys();

            const auto found = keys.findGroupStart(groupname);
            if (found != keys.end())
                return found->first;
        }
        return -1.f;
    }

    float Animation::getTextKeyTime(std::string_view textKey) const
    {
        for (AnimSourceList::const_reverse_iterator iter(mAnimSources.rbegin()); iter != mAnimSources.rend(); ++iter)
        {
            const SceneUtil::TextKeyMap& keys = (*iter)->getTextKeys();

            for (auto iterKey = keys.begin(); iterKey != keys.end(); ++iterKey)
            {
                if (iterKey->second.starts_with(textKey))
                    return iterKey->first;
            }
        }

        return -1.f;
    }

    void Animation::handleTextKey(AnimState& state, std::string_view groupname,
        SceneUtil::TextKeyMap::ConstIterator key, const SceneUtil::TextKeyMap& map)
    {
        std::string_view evt = key->second;

        if (evt.starts_with(groupname) && evt.substr(groupname.size()).starts_with(": "))
        {
            size_t off = groupname.size() + 2;
            if (evt.substr(off) == "loop start")
                state.mLoopStartTime = key->first;
            else if (evt.substr(off) == "loop stop")
                state.mLoopStopTime = key->first;
        }

        try
        {
            if (mTextKeyListener != nullptr)
                mTextKeyListener->handleTextKey(groupname, key, map);
        }
        catch (std::exception& e)
        {
            Log(Debug::Error) << "Error handling text key " << evt << ": " << e.what();
        }
    }

    void Animation::play(std::string_view groupname, const AnimPriority& priority, int blendMask, bool autodisable,
        float speedmult, std::string_view start, std::string_view stop, float startpoint, uint32_t loops,
        bool loopfallback)
    {
        if (!mObjectRoot || mAnimSources.empty())
            return;

        if (groupname.empty())
        {
            resetActiveGroups();
            return;
        }

        AnimStateMap::iterator foundstateiter = mStates.find(groupname);
        if (foundstateiter != mStates.end())
        {
            foundstateiter->second.mPriority = priority;
        }

        AnimStateMap::iterator stateiter = mStates.begin();
        while (stateiter != mStates.end())
        {
            if (stateiter->second.mPriority == priority && stateiter->first != groupname)
                mStates.erase(stateiter++);
            else
                ++stateiter;
        }

        if (foundstateiter != mStates.end())
        {
            resetActiveGroups();
            return;
        }

        /* Look in reverse; last-inserted source has priority. */
        AnimState state;
        AnimSourceList::reverse_iterator iter(mAnimSources.rbegin());
        for (; iter != mAnimSources.rend(); ++iter)
        {
            const SceneUtil::TextKeyMap& textkeys = (*iter)->getTextKeys();
            if (reset(state, textkeys, groupname, start, stop, startpoint, loopfallback))
            {
                state.mSource = *iter;
                state.mSpeedMult = speedmult;
                state.mLoopCount = loops;
                state.mPlaying = (state.getTime() < state.mStopTime);
                state.mPriority = priority;
                state.mBlendMask = blendMask;
                state.mAutoDisable = autodisable;
                state.mGroupname = groupname;
                state.mStartKey = start;
                mStates[std::string{ groupname }] = state;

                if (state.mPlaying)
                {
                    auto textkey = textkeys.lowerBound(state.getTime());
                    while (textkey != textkeys.end() && textkey->first <= state.getTime())
                    {
                        handleTextKey(state, groupname, textkey, textkeys);
                        ++textkey;
                    }
                }

                if (state.getTime() >= state.mLoopStopTime && state.mLoopCount > 0)
                {
                    state.mLoopCount--;
                    state.setTime(state.mLoopStartTime);
                    state.mPlaying = true;
                    if (state.getTime() >= state.mLoopStopTime)
                        break;

                    auto textkey = textkeys.lowerBound(state.getTime());
                    while (textkey != textkeys.end() && textkey->first <= state.getTime())
                    {
                        handleTextKey(state, groupname, textkey, textkeys);
                        ++textkey;
                    }
                }

                break;
            }
        }

        resetActiveGroups();
    }

    bool Animation::reset(AnimState& state, const SceneUtil::TextKeyMap& keys, std::string_view groupname,
        std::string_view start, std::string_view stop, float startpoint, bool loopfallback)
    {
        // Look for text keys in reverse. This normally wouldn't matter, but for some reason undeadwolf_2.nif has two
        // separate walkforward keys, and the last one is supposed to be used.
        auto groupend = keys.rbegin();
        for (; groupend != keys.rend(); ++groupend)
        {
            if (groupend->second.starts_with(groupname) && groupend->second.compare(groupname.size(), 2, ": ") == 0)
                break;
        }

        auto startkey = groupend;
        while (startkey != keys.rend() && !equalsParts(startkey->second, groupname, ": ", start))
            ++startkey;
        if (startkey == keys.rend() && start == "loop start")
        {
            startkey = groupend;
            while (startkey != keys.rend() && !equalsParts(startkey->second, groupname, ": start"))
                ++startkey;
        }
        if (startkey == keys.rend())
            return false;

        auto stopkey = groupend;
        std::size_t checkLength = groupname.size() + 2 + stop.size();
        while (stopkey != keys.rend()
            // We have to ignore extra garbage at the end.
            // The Scrib's idle3 animation has "Idle3: Stop." instead of "Idle3: Stop".
            // Why, just why? :(
            && !equalsParts(std::string_view{ stopkey->second }.substr(0, checkLength), groupname, ": ", stop))
            ++stopkey;
        if (stopkey == keys.rend())
            return false;

        if (startkey->first > stopkey->first)
            return false;

        state.mStartTime = startkey->first;
        if (loopfallback)
        {
            state.mLoopStartTime = startkey->first;
            state.mLoopStopTime = stopkey->first;
        }
        else
        {
            state.mLoopStartTime = startkey->first;
            state.mLoopStopTime = std::numeric_limits<float>::max();
        }
        state.mStopTime = stopkey->first;

        state.setTime(state.mStartTime + ((state.mStopTime - state.mStartTime) * startpoint));

        // mLoopStartTime and mLoopStopTime normally get assigned when encountering these keys while playing the
        // animation (see handleTextKey). But if startpoint is already past these keys, or start time is == stop time,
        // we need to assign them now.

        auto key = groupend;
        for (; key != startkey && key != keys.rend(); ++key)
        {
            if (key->first > state.getTime())
                continue;

            if (equalsParts(key->second, groupname, ": loop start"))
                state.mLoopStartTime = key->first;
            else if (equalsParts(key->second, groupname, ": loop stop"))
                state.mLoopStopTime = key->first;
        }

        return true;
    }

    void Animation::setTextKeyListener(TextKeyListener* listener)
    {
        mTextKeyListener = listener;
    }

    const Animation::NodeMap& Animation::getNodeMap() const
    {
        if (!mNodeMapCreated && mObjectRoot)
        {
            // If the base of this animation is an osgAnimation, we should map the bones not matrix transforms
            if (mRequiresBoneMap)
            {
                SceneUtil::NodeMapVisitorBoneOnly visitor(mNodeMap);
                mObjectRoot->accept(visitor);
            }
            else
            {
                SceneUtil::NodeMapVisitor visitor(mNodeMap);
                mObjectRoot->accept(visitor);
            }
            mNodeMapCreated = true;
        }
        return mNodeMap;
    }

    template <typename ControllerType>
    inline osg::Callback* Animation::handleBlendTransform(const osg::ref_ptr<osg::Node>& node,
        osg::ref_ptr<SceneUtil::KeyframeController> keyframeController,
        std::map<osg::ref_ptr<osg::Node>, osg::ref_ptr<ControllerType>>& blendControllers,
        const AnimBlendStateData& stateData, const osg::ref_ptr<const SceneUtil::AnimBlendRules>& blendRules,
        const AnimState& active)
    {
        osg::ref_ptr<ControllerType> animController;
        if (blendControllers.contains(node))
        {
            animController = blendControllers.at(node);
            animController->setKeyframeTrack(keyframeController, stateData, blendRules);
        }
        else
        {
            animController = new ControllerType(keyframeController, stateData, blendRules);
            blendControllers.emplace(node, animController);

            if constexpr (std::is_same_v<ControllerType, BoneAnimBlendController>)
                assignBoneBlendCallbackRecursive(animController, node, true);
        }

        keyframeController->mTime = active.mTime;

        osg::Callback* asCallback = animController->getAsCallback();
        if constexpr (std::is_same_v<ControllerType, BoneAnimBlendController>)
        {
            // IMPORTANT: we must gather all transforms at point of change before next update
            // instead of at the root update callback because the root bone may require blending.
            if (animController->getBlendTrigger())
                animController->gatherRecursiveBoneTransforms(static_cast<osgAnimation::Bone*>(node.get()));

            // Register blend callback after the initial animation callback
            node->addUpdateCallback(asCallback);
            mActiveControllers.emplace_back(node, asCallback);

            return keyframeController->getAsCallback();
        }

        return asCallback;
    }

    void Animation::resetActiveGroups()
    {
        // remove all previous external controllers from the scene graph
        for (auto it = mActiveControllers.begin(); it != mActiveControllers.end(); ++it)
        {
            osg::Node* node = it->first;
            node->removeUpdateCallback(it->second);

            // Should be no longer needed with OSG 3.4
            it->second->setNestedCallback(nullptr);
        }

        mActiveControllers.clear();

        mAccumCtrl = nullptr;

        for (size_t blendMask = 0; blendMask < sNumBlendMasks; blendMask++)
        {
            AnimStateMap::const_iterator active = mStates.end();

            AnimStateMap::const_iterator state = mStates.begin();
            for (; state != mStates.end(); ++state)
            {
                if (!state->second.blendMaskContains(blendMask))
                    continue;

                if (active == mStates.end()
                    || active->second.mPriority[(BoneGroup)blendMask] < state->second.mPriority[(BoneGroup)blendMask])
                    active = state;
            }

            mAnimationTimePtr[blendMask]->setTimePtr(
                active == mStates.end() ? std::shared_ptr<float>() : active->second.mTime);

            // add external controllers for the AnimSource active in this blend mask
            if (active != mStates.end())
            {
                std::shared_ptr<AnimSource> animsrc = active->second.mSource;
                const AnimBlendStateData stateData
                    = { .mGroupname = active->second.mGroupname, .mStartKey = active->second.mStartKey };

                for (AnimSource::ControllerMap::iterator it = animsrc->mControllerMap[blendMask].begin();
                     it != animsrc->mControllerMap[blendMask].end(); ++it)
                {
                    osg::ref_ptr<osg::Node> node = getNodeMap().at(
                        it->first); // this should not throw, we already checked for the node existing in addAnimSource

                    const bool useSmoothAnims = Settings::game().mSmoothAnimTransitions;

                    osg::Callback* callback = it->second->getAsCallback();
                    if (useSmoothAnims)
                    {
                        if (dynamic_cast<NifOsg::MatrixTransform*>(node.get()))
                        {
                            callback = handleBlendTransform<NifAnimBlendController>(node, it->second,
                                mAnimBlendControllers, stateData, animsrc->mAnimBlendRules, active->second);
                        }
                        else if (dynamic_cast<osgAnimation::Bone*>(node.get()))
                        {
                            callback = handleBlendTransform<BoneAnimBlendController>(node, it->second,
                                mBoneAnimBlendControllers, stateData, animsrc->mAnimBlendRules, active->second);
                        }
                    }

                    node->addUpdateCallback(callback);
                    mActiveControllers.emplace_back(node, callback);

                    if (blendMask == 0 && node == mAccumRoot)
                    {
                        mAccumCtrl = it->second;

                        // make sure reset is last in the chain of callbacks
                        if (!mResetAccumRootCallback)
                        {
                            mResetAccumRootCallback = new ResetAccumRootCallback;
                            mResetAccumRootCallback->setAccumulate(mAccumulate);
                        }
                        mAccumRoot->addUpdateCallback(mResetAccumRootCallback);
                        mActiveControllers.emplace_back(mAccumRoot, mResetAccumRootCallback);
                    }
                }
            }
        }

        addControllers();
    }

    void Animation::adjustSpeedMult(const std::string& groupname, float speedmult)
    {
        AnimStateMap::iterator state(mStates.find(groupname));
        if (state != mStates.end())
            state->second.mSpeedMult = speedmult;
    }

    bool Animation::isPlaying(std::string_view groupname) const
    {
        AnimStateMap::const_iterator state(mStates.find(groupname));
        if (state != mStates.end())
            return state->second.mPlaying;
        return false;
    }

    bool Animation::getInfo(std::string_view groupname, float* complete, float* speedmult, size_t* loopcount) const
    {
        AnimStateMap::const_iterator iter = mStates.find(groupname);
        if (iter == mStates.end())
        {
            if (complete)
                *complete = 0.0f;
            if (speedmult)
                *speedmult = 0.0f;
            if (loopcount)
                *loopcount = 0;
            return false;
        }

        if (complete)
        {
            if (iter->second.mStopTime > iter->second.mStartTime)
                *complete = (iter->second.getTime() - iter->second.mStartTime)
                    / (iter->second.mStopTime - iter->second.mStartTime);
            else
                *complete = (iter->second.mPlaying ? 0.0f : 1.0f);
        }
        if (speedmult)
            *speedmult = iter->second.mSpeedMult;

        if (loopcount)
            *loopcount = iter->second.mLoopCount;
        return true;
    }

    std::string_view Animation::getActiveGroup(BoneGroup boneGroup) const
    {
        if (auto timePtr = mAnimationTimePtr[boneGroup]->getTimePtr())
            for (auto& state : mStates)
                if (state.second.mTime == timePtr)
                    return state.first;
        return "";
    }

    float Animation::getCurrentTime(std::string_view groupname) const
    {
        AnimStateMap::const_iterator iter = mStates.find(groupname);
        if (iter == mStates.end())
            return -1.f;

        return iter->second.getTime();
    }

    void Animation::disable(std::string_view groupname)
    {
        AnimStateMap::iterator iter = mStates.find(groupname);
        if (iter != mStates.end())
            mStates.erase(iter);
        resetActiveGroups();
    }

    float Animation::getVelocity(std::string_view groupname) const
    {
        if (!mAccumRoot)
            return 0.0f;

        std::map<std::string, float>::const_iterator found = mAnimVelocities.find(groupname);
        if (found != mAnimVelocities.end())
            return found->second;

        // Look in reverse; last-inserted source has priority.
        AnimSourceList::const_reverse_iterator animsrc(mAnimSources.rbegin());
        for (; animsrc != mAnimSources.rend(); ++animsrc)
        {
            const SceneUtil::TextKeyMap& keys = (*animsrc)->getTextKeys();
            if (keys.hasGroupStart(groupname))
                break;
        }
        if (animsrc == mAnimSources.rend())
            return 0.0f;

        float velocity = 0.0f;
        const SceneUtil::TextKeyMap& keys = (*animsrc)->getTextKeys();

        const AnimSource::ControllerMap& ctrls = (*animsrc)->mControllerMap[0];
        for (AnimSource::ControllerMap::const_iterator it = ctrls.begin(); it != ctrls.end(); ++it)
        {
            if (Misc::StringUtils::ciEqual(it->first, mAccumRoot->getName()))
            {
                velocity = calcAnimVelocity(keys, it->second, mAccumulate, groupname);
                break;
            }
        }

        // If there's no velocity, keep looking
        if (!(velocity > 1.0f))
        {
            AnimSourceList::const_reverse_iterator animiter = mAnimSources.rbegin();
            while (*animiter != *animsrc)
                ++animiter;

            while (!(velocity > 1.0f) && ++animiter != mAnimSources.rend())
            {
                const SceneUtil::TextKeyMap& keys2 = (*animiter)->getTextKeys();

                const AnimSource::ControllerMap& ctrls2 = (*animiter)->mControllerMap[0];
                for (AnimSource::ControllerMap::const_iterator it = ctrls2.begin(); it != ctrls2.end(); ++it)
                {
                    if (Misc::StringUtils::ciEqual(it->first, mAccumRoot->getName()))
                    {
                        velocity = calcAnimVelocity(keys2, it->second, mAccumulate, groupname);
                        break;
                    }
                }
            }
        }

        mAnimVelocities.emplace(groupname, velocity);

        return velocity;
    }

    void Animation::updatePosition(float oldtime, float newtime, osg::Vec3f& position)
    {
        // Get the difference from the last update, and move the position
        osg::Vec3f off = osg::componentMultiply(mAccumCtrl->getTranslation(newtime), mAccumulate);
        position += off - osg::componentMultiply(mAccumCtrl->getTranslation(oldtime), mAccumulate);
    }

    osg::Vec3f Animation::runAnimation(float duration)
    {
        osg::Vec3f movement(0.f, 0.f, 0.f);
        AnimStateMap::iterator stateiter = mStates.begin();
        while (stateiter != mStates.end())
        {
            AnimState& state = stateiter->second;
            if (mPlayScriptedOnly && !state.mPriority.contains(MWMechanics::Priority_Scripted))
            {
                ++stateiter;
                continue;
            }

            const SceneUtil::TextKeyMap& textkeys = state.mSource->getTextKeys();
            auto textkey = textkeys.upperBound(state.getTime());

            float timepassed = duration * state.mSpeedMult;
            while (state.mPlaying)
            {
                if (!state.shouldLoop())
                {
                    float targetTime = state.getTime() + timepassed;
                    if (textkey == textkeys.end() || textkey->first > targetTime)
                    {
                        if (mAccumCtrl && state.mTime == mAnimationTimePtr[0]->getTimePtr())
                            updatePosition(state.getTime(), targetTime, movement);
                        state.setTime(std::min(targetTime, state.mStopTime));
                    }
                    else
                    {
                        if (mAccumCtrl && state.mTime == mAnimationTimePtr[0]->getTimePtr())
                            updatePosition(state.getTime(), textkey->first, movement);
                        state.setTime(textkey->first);
                    }

                    state.mPlaying = (state.getTime() < state.mStopTime);
                    timepassed = targetTime - state.getTime();

                    while (textkey != textkeys.end() && textkey->first <= state.getTime())
                    {
                        handleTextKey(state, stateiter->first, textkey, textkeys);
                        ++textkey;
                    }
                }
                if (state.shouldLoop())
                {
                    state.mLoopCount--;
                    state.setTime(state.mLoopStartTime);
                    state.mPlaying = true;

                    textkey = textkeys.lowerBound(state.getTime());
                    while (textkey != textkeys.end() && textkey->first <= state.getTime())
                    {
                        handleTextKey(state, stateiter->first, textkey, textkeys);
                        ++textkey;
                    }

                    if (state.getTime() >= state.mLoopStopTime)
                        break;
                }

                if (timepassed <= 0.0f)
                    break;
            }

            if (!state.mPlaying && state.mAutoDisable)
            {
                mStates.erase(stateiter++);

                resetActiveGroups();
            }
            else
                ++stateiter;
        }

        updateEffects();

        const float epsilon = 0.001f;
        float yawOffset = 0;
        if (mRootController)
        {
            bool enable = std::abs(mLegsYawRadians) > epsilon || std::abs(mBodyPitchRadians) > epsilon;
            mRootController->setEnabled(enable);
            if (enable)
            {
                osg::Quat legYaw = osg::Quat(mLegsYawRadians, osg::Vec3f(0, 0, 1));
                mRootController->setRotate(legYaw * osg::Quat(mBodyPitchRadians, osg::Vec3f(1, 0, 0)));
                yawOffset = mLegsYawRadians;
                // When yawing the root, also update the accumulated movement.
                movement = legYaw * movement;
            }
        }
        if (mSpineController)
        {
            float yaw = mUpperBodyYawRadians - yawOffset;
            bool enable = std::abs(yaw) > epsilon;
            mSpineController->setEnabled(enable);
            if (enable)
            {
                mSpineController->setRotate(osg::Quat(yaw, osg::Vec3f(0, 0, 1)));
                yawOffset = mUpperBodyYawRadians;
            }
        }
        if (mHeadController)
        {
            float yaw = mHeadYawRadians - yawOffset;
            bool enable = (std::abs(mHeadPitchRadians) > epsilon || std::abs(yaw) > epsilon);
            mHeadController->setEnabled(enable);
            if (enable)
                mHeadController->setRotate(
                    osg::Quat(mHeadPitchRadians, osg::Vec3f(1, 0, 0)) * osg::Quat(yaw, osg::Vec3f(0, 0, 1)));
        }

        return movement;
    }

    void Animation::setLoopingEnabled(std::string_view groupname, bool enabled)
    {
        AnimStateMap::iterator state(mStates.find(groupname));
        if (state != mStates.end())
            state->second.mLoopingEnabled = enabled;
    }

    void loadBonesFromFile(
        osg::ref_ptr<osg::Node>& baseNode, VFS::Path::NormalizedView model, Resource::ResourceSystem* resourceSystem)
    {
        const osg::Node* node = resourceSystem->getSceneManager()->getTemplate(model).get();
        osg::ref_ptr<osg::Node> sheathSkeleton(
            const_cast<osg::Node*>(node)); // const-trickery required because there is no const version of NodeVisitor

        GetExtendedBonesVisitor getBonesVisitor;
        sheathSkeleton->accept(getBonesVisitor);
        for (auto& nodePair : getBonesVisitor.mFoundBones)
        {
            SceneUtil::FindByNameVisitor findVisitor(nodePair.second->getName());
            baseNode->accept(findVisitor);

            osg::Group* sheathParent = findVisitor.mFoundNode;
            if (sheathParent)
            {
                osg::Node* copy = static_cast<osg::Node*>(nodePair.first->clone(osg::CopyOp::DEEP_COPY_NODES));
                sheathParent->addChild(copy);
            }
        }
    }

    void injectCustomBones(
        osg::ref_ptr<osg::Node>& node, const std::string& model, Resource::ResourceSystem* resourceSystem)
    {
        if (model.empty())
            return;

        std::string animationPath = model;
        if (animationPath.find("meshes") == 0)
        {
            animationPath.replace(0, 6, "animations");
        }
        animationPath.replace(animationPath.size() - 4, 4, "/");

        for (const VFS::Path::Normalized& name : resourceSystem->getVFS()->getRecursiveDirectoryIterator(animationPath))
        {
            if (Misc::getFileExtension(name) == "nif")
                loadBonesFromFile(node, name, resourceSystem);
        }
    }

    osg::ref_ptr<osg::Node> getModelInstance(Resource::ResourceSystem* resourceSystem, const std::string& model,
        bool baseonly, bool inject, const std::string& defaultSkeleton)
    {
        Resource::SceneManager* sceneMgr = resourceSystem->getSceneManager();
        if (baseonly)
        {
            typedef std::map<std::string, osg::ref_ptr<osg::Node>> Cache;
            static Cache cache;
            Cache::iterator found = cache.find(model);
            if (found == cache.end())
            {
                osg::ref_ptr<osg::Node> created = sceneMgr->getInstance(VFS::Path::toNormalized(model));

                if (inject)
                {
                    injectCustomBones(created, defaultSkeleton, resourceSystem);
                    injectCustomBones(created, model, resourceSystem);
                }

                SceneUtil::CleanObjectRootVisitor removeDrawableVisitor;
                created->accept(removeDrawableVisitor);
                removeDrawableVisitor.remove();

                cache.insert(std::make_pair(model, created));

                return sceneMgr->getInstance(created);
            }
            else
                return sceneMgr->getInstance(found->second);
        }
        else
        {
            osg::ref_ptr<osg::Node> created = sceneMgr->getInstance(VFS::Path::toNormalized(model));

            if (inject)
            {
                injectCustomBones(created, defaultSkeleton, resourceSystem);
                injectCustomBones(created, model, resourceSystem);
            }

            return created;
        }
    }

    void Animation::setObjectRoot(const std::string& model, bool forceskeleton, bool baseonly, bool isCreature)
    {
        osg::ref_ptr<osg::StateSet> previousStateset;
        if (mObjectRoot)
        {
            if (mLightListCallback)
                mObjectRoot->removeCullCallback(mLightListCallback);
            if (mTransparencyUpdater)
                mObjectRoot->removeCullCallback(mTransparencyUpdater);
            previousStateset = mObjectRoot->getStateSet();
            mObjectRoot->getParent(0)->removeChild(mObjectRoot);
        }
        mObjectRoot = nullptr;
        mSkeleton = nullptr;

        mNodeMap.clear();
        mNodeMapCreated = false;
        mActiveControllers.clear();
        mAccumRoot = nullptr;
        mAccumCtrl = nullptr;

        std::string defaultSkeleton;
        bool inject = false;

        if (Settings::game().mUseAdditionalAnimSources && mPtr.getClass().isActor())
        {
            if (isCreature)
            {
                MWWorld::LiveCellRef<ESM::Creature>* ref = mPtr.get<ESM::Creature>();
                if (ref->mBase->mFlags & ESM::Creature::Bipedal)
                {
                    defaultSkeleton = Settings::models().mXbaseanim.get().value();
                    inject = true;
                }
            }
            else
            {
                inject = true;
                MWWorld::LiveCellRef<ESM::NPC>* ref = mPtr.get<ESM::NPC>();
                if (!ref->mBase->mModel.empty())
                {
                    // If NPC has a custom animation model attached, we should inject bones from default skeleton for
                    // given race and gender as well Since it is a quite rare case, there should not be a noticable
                    // performance loss Note: consider that player and werewolves have no custom animation files
                    // attached for now
                    const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
                    const ESM::Race* race = store.get<ESM::Race>().find(ref->mBase->mRace);

                    const bool firstPerson = false;
                    const bool isBeast = (race->mData.mFlags & ESM::Race::Beast) != 0;
                    const bool isFemale = !ref->mBase->isMale();
                    const bool werewolf = false;

                    defaultSkeleton = Misc::ResourceHelpers::correctActorModelPath(
                        VFS::Path::toNormalized(getActorSkeleton(firstPerson, isFemale, isBeast, werewolf)),
                        mResourceSystem->getVFS());
                }
            }
        }

        if (!forceskeleton)
        {
            osg::ref_ptr<osg::Node> created
                = getModelInstance(mResourceSystem, model, baseonly, inject, defaultSkeleton);
            mInsert->addChild(created);
            mObjectRoot = created->asGroup();
            if (!mObjectRoot)
            {
                mInsert->removeChild(created);
                mObjectRoot = new osg::Group;
                mObjectRoot->addChild(created);
                mInsert->addChild(mObjectRoot);
            }
            osg::ref_ptr<SceneUtil::Skeleton> skel = dynamic_cast<SceneUtil::Skeleton*>(mObjectRoot.get());
            if (skel)
                mSkeleton = skel.get();
        }
        else
        {
            osg::ref_ptr<osg::Node> created
                = getModelInstance(mResourceSystem, model, baseonly, inject, defaultSkeleton);
            osg::ref_ptr<SceneUtil::Skeleton> skel = dynamic_cast<SceneUtil::Skeleton*>(created.get());
            if (!skel)
            {
                skel = new SceneUtil::Skeleton;
                skel->addChild(created);
            }
            mSkeleton = skel.get();
            mObjectRoot = skel;
            mInsert->addChild(mObjectRoot);
        }

        // osgAnimation formats with skeletons should have their nodemap be bone instances
        // FIXME: better way to detect osgAnimation here instead of relying on extension?
        mRequiresBoneMap = mSkeleton != nullptr && !Misc::StringUtils::ciEndsWith(model, ".nif");

        if (previousStateset)
            mObjectRoot->setStateSet(previousStateset);

        if (isCreature)
        {
            SceneUtil::RemoveTriBipVisitor removeTriBipVisitor;
            mObjectRoot->accept(removeTriBipVisitor);
            removeTriBipVisitor.remove();
        }

        if (!mLightListCallback)
            mLightListCallback = new SceneUtil::LightListCallback;
        mObjectRoot->addCullCallback(mLightListCallback);
        if (mTransparencyUpdater)
            mObjectRoot->addCullCallback(mTransparencyUpdater);
    }

    osg::Group* Animation::getObjectRoot()
    {
        return mObjectRoot.get();
    }

    osg::Group* Animation::getOrCreateObjectRoot()
    {
        if (mObjectRoot)
            return mObjectRoot.get();

        mObjectRoot = new osg::Group;
        mInsert->addChild(mObjectRoot);
        return mObjectRoot.get();
    }

    void Animation::addSpellCastGlow(const osg::Vec4f& color, float glowDuration)
    {
        if (!mGlowUpdater || (mGlowUpdater->isDone() || (mGlowUpdater->isPermanentGlowUpdater() == true)))
        {
            if (mGlowUpdater && mGlowUpdater->isDone())
                mObjectRoot->removeUpdateCallback(mGlowUpdater);

            if (mGlowUpdater && mGlowUpdater->isPermanentGlowUpdater())
            {
                mGlowUpdater->setColor(color);
                mGlowUpdater->setDuration(glowDuration);
            }
            else
                mGlowUpdater = SceneUtil::addEnchantedGlow(mObjectRoot, mResourceSystem, color, glowDuration);
        }
    }

    void Animation::addExtraLight(osg::ref_ptr<osg::Group> parent, const SceneUtil::LightCommon& esmLight)
    {
        bool exterior = mPtr.isInCell() && mPtr.getCell()->getCell()->isExterior();

        mExtraLightSource = SceneUtil::addLight(parent, esmLight, Mask_Lighting, exterior);
        mExtraLightSource->setActorFade(mAlpha);
    }

    void Animation::addEffect(std::string_view model, std::string_view effectId, bool loop, std::string_view bonename,
        std::string_view texture)
    {
        if (!mObjectRoot.get())
            return;

        // Early out if we already have this effect
        FindVfxCallbacksVisitor visitor(effectId);
        mInsert->accept(visitor);

        for (std::vector<UpdateVfxCallback*>::iterator it = visitor.mCallbacks.begin(); it != visitor.mCallbacks.end();
             ++it)
        {
            UpdateVfxCallback* callback = *it;

            if (loop && !callback->mFinished && callback->mParams.mLoop && callback->mParams.mBoneName == bonename)
                return;
        }

        EffectParams params;
        params.mModelName = model;
        osg::ref_ptr<osg::Group> parentNode;
        if (bonename.empty())
            parentNode = mInsert;
        else
        {
            NodeMap::const_iterator found = getNodeMap().find(bonename);
            if (found == getNodeMap().end())
                throw std::runtime_error("Can't find bone " + std::string{ bonename });

            parentNode = found->second;
        }

        osg::ref_ptr<SceneUtil::PositionAttitudeTransform> trans = new SceneUtil::PositionAttitudeTransform;
        if (!mPtr.getClass().isNpc())
        {
            osg::Vec3f bounds(MWBase::Environment::get().getWorld()->getHalfExtents(mPtr) * 2.f);
            float scale = std::max({ bounds.x(), bounds.y(), bounds.z() / 2.f }) / 64.f;
            if (scale > 1.f)
                trans->setScale(osg::Vec3f(scale, scale, scale));
            float offset = 0.f;
            if (bounds.z() < 128.f)
                offset = bounds.z() - 128.f;
            else if (bounds.z() < bounds.x() + bounds.y())
                offset = 128.f - bounds.z();
            if (MWBase::Environment::get().getWorld()->isFlying(mPtr))
                offset /= 20.f;
            trans->setPosition(osg::Vec3f(0.f, 0.f, offset * scale));
        }
        parentNode->addChild(trans);

        osg::ref_ptr<osg::Node> node
            = mResourceSystem->getSceneManager()->getInstance(VFS::Path::toNormalized(model), trans);

        // Morrowind has a white ambient light attached to the root VFX node of the scenegraph
        node->getOrCreateStateSet()->setAttributeAndModes(
            getVFXLightModelInstance(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        mResourceSystem->getSceneManager()->setUpNormalsRTForStateSet(node->getOrCreateStateSet(), false);
        SceneUtil::FindMaxControllerLengthVisitor findMaxLengthVisitor;
        node->accept(findMaxLengthVisitor);

        node->setNodeMask(Mask_Effect);

        MarkDrawablesVisitor markVisitor(Mask_Effect);
        node->accept(markVisitor);

        params.mMaxControllerLength = findMaxLengthVisitor.getMaxLength();
        params.mLoop = loop;
        params.mEffectId = effectId;
        params.mBoneName = bonename;
        params.mAnimTime = std::make_shared<EffectAnimationTime>();
        trans->addUpdateCallback(new UpdateVfxCallback(params));

        SceneUtil::AssignControllerSourcesVisitor assignVisitor(
            std::shared_ptr<SceneUtil::ControllerSource>(params.mAnimTime));
        node->accept(assignVisitor);

        // Notify that this animation has attached magic effects
        mHasMagicEffects = true;

        overrideFirstRootTexture(texture, mResourceSystem, *node);
    }

    void Animation::removeEffect(std::string_view effectId)
    {
        RemoveCallbackVisitor visitor(effectId);
        mInsert->accept(visitor);
        visitor.remove();
        mHasMagicEffects = visitor.mHasMagicEffects;
    }

    void Animation::removeEffects()
    {
        removeEffect("");
    }

    std::vector<std::string_view> Animation::getLoopingEffects() const
    {
        if (!mHasMagicEffects)
            return {};

        FindVfxCallbacksVisitor visitor;
        mInsert->accept(visitor);

        std::vector<std::string_view> out;

        for (std::vector<UpdateVfxCallback*>::iterator it = visitor.mCallbacks.begin(); it != visitor.mCallbacks.end();
             ++it)
        {
            UpdateVfxCallback* callback = *it;

            if (callback->mParams.mLoop && !callback->mFinished)
                out.push_back(callback->mParams.mEffectId);
        }
        return out;
    }

    void Animation::updateEffects()
    {
        // We do not need to visit scene every frame.
        // We can use a bool flag to check in spellcasting effect found.
        if (!mHasMagicEffects)
            return;

        // TODO: objects without animation still will have
        // transformation nodes with finished callbacks
        RemoveFinishedCallbackVisitor visitor;
        mInsert->accept(visitor);
        visitor.remove();
        mHasMagicEffects = visitor.mHasMagicEffects;
    }

    bool Animation::upperBodyReady() const
    {
        for (AnimStateMap::const_iterator stateiter = mStates.begin(); stateiter != mStates.end(); ++stateiter)
        {
            if (stateiter->second.mPriority.contains(int(MWMechanics::Priority_Hit))
                || stateiter->second.mPriority.contains(int(MWMechanics::Priority_Weapon))
                || stateiter->second.mPriority.contains(int(MWMechanics::Priority_Knockdown))
                || stateiter->second.mPriority.contains(int(MWMechanics::Priority_Death)))
                return false;
        }
        return true;
    }

    const osg::Node* Animation::getNode(std::string_view name) const
    {
        NodeMap::const_iterator found = getNodeMap().find(name);
        if (found == getNodeMap().end())
            return nullptr;
        else
            return found->second;
    }

    void Animation::setAlpha(float alpha)
    {
        if (alpha == mAlpha)
            return;
        mAlpha = alpha;

        // TODO: we use it to fade actors away too, but it would be nice to have a dithering shader instead.
        if (alpha != 1.f)
        {
            if (mTransparencyUpdater == nullptr)
            {
                mTransparencyUpdater = new TransparencyUpdater(alpha);
                mObjectRoot->addCullCallback(mTransparencyUpdater);
            }
            else
                mTransparencyUpdater->setAlpha(alpha);
        }
        else
        {
            mObjectRoot->removeCullCallback(mTransparencyUpdater);
            mTransparencyUpdater = nullptr;
        }
        if (mExtraLightSource)
            mExtraLightSource->setActorFade(alpha);
    }

    void Animation::setLightEffect(float effect)
    {
        if (effect == 0)
        {
            if (mGlowLight)
            {
                mInsert->removeChild(mGlowLight);
                mGlowLight = nullptr;
            }
        }
        else
        {
            // 1 pt of Light magnitude corresponds to 1 foot of radius
            float radius = effect * std::ceil(Constants::UnitsPerFoot);
            // Arbitrary multiplier used to make the obvious cut-off less obvious
            float cutoffMult = 3;

            if (!mGlowLight || (radius * cutoffMult) != mGlowLight->getRadius())
            {
                if (mGlowLight)
                {
                    mInsert->removeChild(mGlowLight);
                    mGlowLight = nullptr;
                }

                osg::ref_ptr<osg::Light> light(new osg::Light);
                light->setDiffuse(osg::Vec4f(0, 0, 0, 0));
                light->setSpecular(osg::Vec4f(0, 0, 0, 0));
                light->setAmbient(osg::Vec4f(1.5f, 1.5f, 1.5f, 1.f));

                bool isExterior = mPtr.isInCell() && mPtr.getCell()->getCell()->isExterior();
                SceneUtil::configureLight(light, radius, isExterior);

                mGlowLight = new SceneUtil::LightSource;
                mGlowLight->setNodeMask(Mask_Lighting);
                mInsert->addChild(mGlowLight);
                mGlowLight->setLight(light);
            }

            mGlowLight->setRadius(radius * cutoffMult);
        }
    }

    void Animation::addControllers()
    {
        mHeadController = addRotateController("bip01 head");
        mSpineController = addRotateController("bip01 spine1");
        mRootController = addRotateController("bip01");
    }

    osg::ref_ptr<RotateController> Animation::addRotateController(std::string_view bone)
    {
        auto iter = getNodeMap().find(bone);
        if (iter == getNodeMap().end())
            return nullptr;
        osg::MatrixTransform* node = iter->second;

        bool foundKeyframeCtrl = false;
        osg::Callback* cb = node->getUpdateCallback();
        while (cb)
        {
            if (dynamic_cast<NifAnimBlendController*>(cb) || dynamic_cast<BoneAnimBlendController*>(cb)
                || dynamic_cast<SceneUtil::KeyframeController*>(cb))
            {
                foundKeyframeCtrl = true;
                break;
            }
            cb = cb->getNestedCallback();
        }
        // Note: AnimBlendController also does the reset so if one is present - we should add the rotation node
        // Without KeyframeController the orientation will not be reseted each frame, so
        // RotateController shouldn't be used for such nodes.
        if (!foundKeyframeCtrl)
            return nullptr;

        osg::ref_ptr<RotateController> controller(new RotateController(mObjectRoot.get()));
        node->addUpdateCallback(controller);
        mActiveControllers.emplace_back(node, controller);
        return controller;
    }

    void Animation::setHeadPitch(float pitchRadians)
    {
        mHeadPitchRadians = pitchRadians;
    }

    void Animation::setHeadYaw(float yawRadians)
    {
        mHeadYawRadians = yawRadians;
    }

    float Animation::getHeadPitch() const
    {
        return mHeadPitchRadians;
    }

    float Animation::getHeadYaw() const
    {
        return mHeadYawRadians;
    }

    void Animation::removeFromScene()
    {
        removeFromSceneImpl();
    }

    void Animation::removeFromSceneImpl()
    {
        if (mGlowLight != nullptr)
            mInsert->removeChild(mGlowLight);

        if (mObjectRoot != nullptr)
            mInsert->removeChild(mObjectRoot);
    }

    MWWorld::MovementDirectionFlags Animation::getSupportedMovementDirections(
        std::span<const std::string_view> prefixes) const
    {
        MWWorld::MovementDirectionFlags result = 0;
        for (const std::string_view animation : mSupportedAnimations)
        {
            if (std::find_if(
                    prefixes.begin(), prefixes.end(), [&](std::string_view v) { return animation.starts_with(v); })
                == prefixes.end())
                continue;
            if (animation.ends_with("forward"))
                result |= MWWorld::MovementDirectionFlag_Forward;
            else if (animation.ends_with("back"))
                result |= MWWorld::MovementDirectionFlag_Back;
            else if (animation.ends_with("left"))
                result |= MWWorld::MovementDirectionFlag_Left;
            else if (animation.ends_with("right"))
                result |= MWWorld::MovementDirectionFlag_Right;
        }
        return result;
    }

    // ------------------------------------------------------

    float Animation::AnimationTime::getValue(osg::NodeVisitor*)
    {
        if (mTimePtr)
            return *mTimePtr;
        return 0.f;
    }

    float EffectAnimationTime::getValue(osg::NodeVisitor*)
    {
        return mTime;
    }

    void EffectAnimationTime::addTime(float duration)
    {
        mTime += duration;
    }

    void EffectAnimationTime::resetTime(float time)
    {
        mTime = time;
    }

    float EffectAnimationTime::getTime() const
    {
        return mTime;
    }

    // --------------------------------------------------------------------------------

    ObjectAnimation::ObjectAnimation(const MWWorld::Ptr& ptr, const std::string& model,
        Resource::ResourceSystem* resourceSystem, bool animated, bool allowLight)
        : Animation(ptr, osg::ref_ptr<osg::Group>(ptr.getRefData().getBaseNode()), resourceSystem)
    {
        if (!model.empty())
        {
            setObjectRoot(model, false, false, false);
            if (animated)
                addAnimSource(model, model);

            if (!ptr.getClass().getEnchantment(ptr).empty())
                mGlowUpdater = SceneUtil::addEnchantedGlow(
                    mObjectRoot, mResourceSystem, ptr.getClass().getEnchantmentColor(ptr));
        }
        if (ptr.getType() == ESM::Light::sRecordId && allowLight)
            addExtraLight(getOrCreateObjectRoot(), SceneUtil::LightCommon(*ptr.get<ESM::Light>()->mBase));
        if (ptr.getType() == ESM4::Light::sRecordId && allowLight)
            addExtraLight(getOrCreateObjectRoot(), SceneUtil::LightCommon(*ptr.get<ESM4::Light>()->mBase));

        if (!allowLight && mObjectRoot)
        {
            RemoveParticlesVisitor visitor;
            mObjectRoot->accept(visitor);
            visitor.remove();
        }

        if (Settings::game().mDayNightSwitches && SceneUtil::hasUserDescription(mObjectRoot, Constants::NightDayLabel))
        {
            AddSwitchCallbacksVisitor visitor;
            mObjectRoot->accept(visitor);
        }

        if (Settings::game().mGraphicHerbalism && ptr.getRefData().getCustomData() != nullptr
            && ObjectAnimation::canBeHarvested())
        {
            const MWWorld::ContainerStore& store = ptr.getClass().getContainerStore(ptr);
            if (!store.hasVisibleItems())
            {
                HarvestVisitor visitor;
                mObjectRoot->accept(visitor);
            }
        }
    }

    bool ObjectAnimation::canBeHarvested() const
    {
        if (mPtr.getType() != ESM::Container::sRecordId)
            return false;

        const MWWorld::LiveCellRef<ESM::Container>* ref = mPtr.get<ESM::Container>();
        if (!(ref->mBase->mFlags & ESM::Container::Organic))
            return false;

        return SceneUtil::hasUserDescription(mObjectRoot, Constants::HerbalismLabel);
    }

    // ------------------------------

    PartHolder::PartHolder(osg::ref_ptr<osg::Node> node)
        : mNode(std::move(node))
    {
    }

    PartHolder::~PartHolder()
    {
        if (mNode.get() && !mNode->getNumParents())
            Log(Debug::Verbose) << "Part \"" << mNode->getName() << "\" has no parents";

        if (mNode.get() && mNode->getNumParents())
        {
            if (mNode->getNumParents() > 1)
                Log(Debug::Verbose) << "Part \"" << mNode->getName() << "\" has multiple (" << mNode->getNumParents()
                                    << ") parents";
            mNode->getParent(0)->removeChild(mNode);
        }
    }
}
