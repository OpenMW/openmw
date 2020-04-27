#include "animation.hpp"

#include <iomanip>
#include <limits>

#include <osg/MatrixTransform>
#include <osg/BlendFunc>
#include <osg/Material>
#include <osg/PositionAttitudeTransform>
#include <osg/Switch>

#include <osgParticle/ParticleSystem>
#include <osgParticle/ParticleProcessor>

#include <components/debug/debuglog.hpp>

#include <components/resource/scenemanager.hpp>
#include <components/resource/keyframemanager.hpp>
#include <components/resource/resourcesystem.hpp>

#include <components/misc/constants.hpp>
#include <components/misc/resourcehelpers.hpp>

#include <components/nifosg/nifloader.hpp> // KeyframeHolder
#include <components/nifosg/controller.hpp>

#include <components/vfs/manager.hpp>

#include <components/sceneutil/actorutil.hpp>
#include <components/sceneutil/statesetupdater.hpp>
#include <components/sceneutil/visitor.hpp>
#include <components/sceneutil/vismask.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/lightutil.hpp>
#include <components/sceneutil/skeleton.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/sceneutil/util.hpp>

#include <components/settings/settings.hpp>

#include <components/shader/shadermanager.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwmechanics/character.hpp" // FIXME: for MWMechanics::Priority

#include "util.hpp"
#include "rotatecontroller.hpp"

namespace
{

    /// Removes all particle systems and related nodes in a subgraph.
    class RemoveParticlesVisitor : public osg::NodeVisitor
    {
    public:
        RemoveParticlesVisitor()
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        { }

        virtual void apply(osg::Node &node)
        {
            if (dynamic_cast<osgParticle::ParticleProcessor*>(&node))
                mToRemove.push_back(&node);

            traverse(node);
        }

        virtual void apply(osg::Drawable& drw)
        {
            if (osgParticle::ParticleSystem* partsys = dynamic_cast<osgParticle::ParticleSystem*>(&drw))
                mToRemove.push_back(partsys);
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
        std::vector<osg::ref_ptr<osg::Node> > mToRemove;
    };

    class DayNightCallback : public osg::NodeCallback
    {
    public:
        DayNightCallback() : mCurrentState(0)
        {
        }

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            unsigned int state = MWBase::Environment::get().getWorld()->getNightDayMode();
            const unsigned int newState = node->asGroup()->getNumChildren() > state ? state : 0;

            if (newState != mCurrentState)
            {
                mCurrentState = newState;
                node->asSwitch()->setSingleChildOn(mCurrentState);
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
        { }

        virtual void apply(osg::Switch &switchNode)
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

        virtual void apply(osg::Switch& node)
        {
            if (node.getName() == Constants::HerbalismLabel)
            {
                node.setSingleChildOn(1);
            }

            traverse(node);
        }
    };

    NifOsg::TextKeyMap::const_iterator findGroupStart(const NifOsg::TextKeyMap &keys, const std::string &groupname)
    {
        NifOsg::TextKeyMap::const_iterator iter(keys.begin());
        for(;iter != keys.end();++iter)
        {
            if(iter->second.compare(0, groupname.size(), groupname) == 0 &&
               iter->second.compare(groupname.size(), 2, ": ") == 0)
                break;
        }
        return iter;
    }

    float calcAnimVelocity(const std::multimap<float, std::string>& keys,
                                      NifOsg::KeyframeController *nonaccumctrl, const osg::Vec3f& accum, const std::string &groupname)
    {
        const std::string start = groupname+": start";
        const std::string loopstart = groupname+": loop start";
        const std::string loopstop = groupname+": loop stop";
        const std::string stop = groupname+": stop";
        float starttime = std::numeric_limits<float>::max();
        float stoptime = 0.0f;

        // Pick the last Loop Stop key and the last Loop Start key.
        // This is required because of broken text keys in AshVampire.nif.
        // It has *two* WalkForward: Loop Stop keys at different times, the first one is used for stopping playback
        // but the animation velocity calculation uses the second one.
        // As result the animation velocity calculation is not correct, and this incorrect velocity must be replicated,
        // because otherwise the Creature's Speed (dagoth uthol) would not be sufficient to move fast enough.
        NifOsg::TextKeyMap::const_reverse_iterator keyiter(keys.rbegin());
        while(keyiter != keys.rend())
        {
            if(keyiter->second == start || keyiter->second == loopstart)
            {
                starttime = keyiter->first;
                break;
            }
            ++keyiter;
        }
        keyiter = keys.rbegin();
        while(keyiter != keys.rend())
        {
            if (keyiter->second == stop)
                stoptime = keyiter->first;
            else if (keyiter->second == loopstop)
            {
                stoptime = keyiter->first;
                break;
            }
            ++keyiter;
        }

        if(stoptime > starttime)
        {
            osg::Vec3f startpos = osg::componentMultiply(nonaccumctrl->getTranslation(starttime), accum);
            osg::Vec3f endpos = osg::componentMultiply(nonaccumctrl->getTranslation(stoptime), accum);

            return (startpos-endpos).length() / (stoptime - starttime);
        }

        return 0.0f;
    }

    /// @brief Base class for visitors that remove nodes from a scene graph.
    /// Subclasses need to fill the mToRemove vector.
    /// To use, node->accept(removeVisitor); removeVisitor.remove();
    class RemoveVisitor : public osg::NodeVisitor
    {
    public:
        RemoveVisitor()
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        {
        }

        void remove()
        {
            for (RemoveVec::iterator it = mToRemove.begin(); it != mToRemove.end(); ++it)
            {
                if (!it->second->removeChild(it->first))
                    Log(Debug::Error) << "Error removing " << it->first->getName();
            }
        }

    protected:
        // <node to remove, parent node to remove it from>
        typedef std::vector<std::pair<osg::Node*, osg::Group*> > RemoveVec;
        std::vector<std::pair<osg::Node*, osg::Group*> > mToRemove;
    };

    class GetExtendedBonesVisitor : public osg::NodeVisitor
    {
    public:
        GetExtendedBonesVisitor()
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        {
        }

        void apply(osg::Node& node)
        {
            if (SceneUtil::hasUserDescription(&node, "CustomBone"))
            {
                mFoundBones.emplace_back(&node, node.getParent(0));
                return;
            }

            traverse(node);
        }

        std::vector<std::pair<osg::Node*, osg::Group*> > mFoundBones;
    };

    class RemoveFinishedCallbackVisitor : public RemoveVisitor
    {
    public:
        bool mHasMagicEffects;

        RemoveFinishedCallbackVisitor()
            : RemoveVisitor()
            , mHasMagicEffects(false)
        {
        }

        virtual void apply(osg::Node &node)
        {
            traverse(node);
        }

        virtual void apply(osg::Group &group)
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
                        mToRemove.push_back(std::make_pair(group.asNode(), group.getParent(0)));
                    else
                        mHasMagicEffects = true;
                }
            }
        }

        virtual void apply(osg::MatrixTransform &node)
        {
            traverse(node);
        }

        virtual void apply(osg::Geometry&)
        {
        }
    };

    class RemoveCallbackVisitor : public RemoveVisitor
    {
    public:
        bool mHasMagicEffects;

        RemoveCallbackVisitor()
            : RemoveVisitor()
            , mHasMagicEffects(false)
            , mEffectId(-1)
        {
        }

        RemoveCallbackVisitor(int effectId)
            : RemoveVisitor()
            , mHasMagicEffects(false)
            , mEffectId(effectId)
        {
        }

        virtual void apply(osg::Node &node)
        {
            traverse(node);
        }

        virtual void apply(osg::Group &group)
        {
            traverse(group);

            osg::Callback* callback = group.getUpdateCallback();
            if (callback)
            {
                MWRender::UpdateVfxCallback* vfxCallback = dynamic_cast<MWRender::UpdateVfxCallback*>(callback);
                if (vfxCallback)
                {
                    bool toRemove = mEffectId < 0 || vfxCallback->mParams.mEffectId == mEffectId;
                    if (toRemove)
                        mToRemove.push_back(std::make_pair(group.asNode(), group.getParent(0)));
                    else
                        mHasMagicEffects = true;
                }
            }
        }

        virtual void apply(osg::MatrixTransform &node)
        {
            traverse(node);
        }

        virtual void apply(osg::Geometry&)
        {
        }

    private:
        int mEffectId;
    };

    class FindVfxCallbacksVisitor : public osg::NodeVisitor
    {
    public:

        std::vector<MWRender::UpdateVfxCallback*> mCallbacks;

        FindVfxCallbacksVisitor()
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mEffectId(-1)
        {
        }

        FindVfxCallbacksVisitor(int effectId)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mEffectId(effectId)
        {
        }

        virtual void apply(osg::Node &node)
        {
            traverse(node);
        }

        virtual void apply(osg::Group &group)
        {
            osg::Callback* callback = group.getUpdateCallback();
            if (callback)
            {
                MWRender::UpdateVfxCallback* vfxCallback = dynamic_cast<MWRender::UpdateVfxCallback*>(callback);
                if (vfxCallback)
                {
                    if (mEffectId < 0 || vfxCallback->mParams.mEffectId == mEffectId)
                    {
                        mCallbacks.push_back(vfxCallback);
                    }
                }
            }
            traverse(group);
        }

        virtual void apply(osg::MatrixTransform &node)
        {
            traverse(node);
        }

        virtual void apply(osg::Geometry&)
        {
        }

    private:
        int mEffectId;
    };

    // Removes all drawables from a graph.
    class CleanObjectRootVisitor : public RemoveVisitor
    {
    public:
        virtual void apply(osg::Drawable& drw)
        {
            applyDrawable(drw);
        }

        virtual void apply(osg::Group& node)
        {
            applyNode(node);
        }
        virtual void apply(osg::MatrixTransform& node)
        {
            applyNode(node);
        }
        virtual void apply(osg::Node& node)
        {
            applyNode(node);
        }

        void applyNode(osg::Node& node)
        {
            if (node.getStateSet())
                node.setStateSet(nullptr);

            if (node.getNodeMask() == 0x1 && node.getNumParents() == 1)
                mToRemove.push_back(std::make_pair(&node, node.getParent(0)));
            else
                traverse(node);
        }
        void applyDrawable(osg::Node& node)
        {
            osg::NodePath::iterator parent = getNodePath().end()-2;
            // We know that the parent is a Group because only Groups can have children.
            osg::Group* parentGroup = static_cast<osg::Group*>(*parent);

            // Try to prune nodes that would be empty after the removal
            if (parent != getNodePath().begin())
            {
                // This could be extended to remove the parent's parent, and so on if they are empty as well.
                // But for NIF files, there won't be a benefit since only TriShapes can be set to STATIC dataVariance.
                osg::Group* parentParent = static_cast<osg::Group*>(*(parent - 1));
                if (parentGroup->getNumChildren() == 1 && parentGroup->getDataVariance() == osg::Object::STATIC)
                {
                    mToRemove.push_back(std::make_pair(parentGroup, parentParent));
                    return;
                }
            }

            mToRemove.push_back(std::make_pair(&node, parentGroup));
        }
    };

    class RemoveTriBipVisitor : public RemoveVisitor
    {
    public:
        virtual void apply(osg::Drawable& drw)
        {
            applyImpl(drw);
        }

        virtual void apply(osg::Group& node)
        {
            traverse(node);
        }
        virtual void apply(osg::MatrixTransform& node)
        {
            traverse(node);
        }

        void applyImpl(osg::Node& node)
        {
            const std::string toFind = "tri bip";
            if (Misc::StringUtils::ciCompareLen(node.getName(), toFind, toFind.size()) == 0)
            {
                osg::Group* parent = static_cast<osg::Group*>(*(getNodePath().end()-2));
                // Not safe to remove in apply(), since the visitor is still iterating the child list
                mToRemove.push_back(std::make_pair(&node, parent));
            }
        }
    };
}

namespace MWRender
{
    class TransparencyUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        TransparencyUpdater(const float alpha, osg::ref_ptr<osg::Uniform> shadowUniform)
            : mAlpha(alpha)
            , mShadowUniform(shadowUniform)
        {
        }

        void setAlpha(const float alpha)
        {
            mAlpha = alpha;
        }

    protected:
        virtual void setDefaults(osg::StateSet* stateset)
        {
            osg::BlendFunc* blendfunc (new osg::BlendFunc);
            stateset->setAttributeAndModes(blendfunc, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
            // TODO: don't do this anymore once custom shadow renderbin is handling it
            if (mShadowUniform)
                stateset->addUniform(mShadowUniform);

            // FIXME: overriding diffuse/ambient/emissive colors
            osg::Material* material = new osg::Material;
            material->setColorMode(osg::Material::OFF);
            material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,mAlpha));
            material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,1));
            stateset->setAttributeAndModes(material, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
            stateset->addUniform(new osg::Uniform("colorMode", 0), osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
        }

        virtual void apply(osg::StateSet* stateset, osg::NodeVisitor* /*nv*/)
        {
            osg::Material* material = static_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
            material->setAlpha(osg::Material::FRONT_AND_BACK, mAlpha);
        }

    private:
        float mAlpha;
        osg::ref_ptr<osg::Uniform> mShadowUniform;
    };

    struct Animation::AnimSource
    {
        osg::ref_ptr<const NifOsg::KeyframeHolder> mKeyframes;

        typedef std::map<std::string, osg::ref_ptr<NifOsg::KeyframeController> > ControllerMap;

        ControllerMap mControllerMap[Animation::sNumBlendMasks];

        const std::multimap<float, std::string>& getTextKeys() const;
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
                node->setNodeMask(SceneUtil::Mask_Disabled);
                mFinished = true;
            }
        }
    }

    class ResetAccumRootCallback : public osg::NodeCallback
    {
    public:
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            osg::MatrixTransform* transform = static_cast<osg::MatrixTransform*>(node);

            osg::Matrix mat = transform->getMatrix();
            osg::Vec3f position = mat.getTrans();
            position = osg::componentMultiply(mResetAxes, position);
            mat.setTrans(position);
            transform->setMatrix(mat);

            traverse(node, nv);
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

    Animation::Animation(const MWWorld::Ptr &ptr, osg::ref_ptr<osg::Group> parentNode, Resource::ResourceSystem* resourceSystem)
        : mInsert(parentNode)
        , mSkeleton(nullptr)
        , mNodeMapCreated(false)
        , mPtr(ptr)
        , mResourceSystem(resourceSystem)
        , mAccumulate(1.f, 1.f, 0.f)
        , mTextKeyListener(nullptr)
        , mHeadYawRadians(0.f)
        , mHeadPitchRadians(0.f)
        , mHasMagicEffects(false)
        , mAlpha(1.f)
    {
        for(size_t i = 0;i < sNumBlendMasks;i++)
            mAnimationTimePtr[i].reset(new AnimationTime);

        mLightListCallback = new SceneUtil::LightListCallback;
    }

    Animation::~Animation()
    {
        Animation::setLightEffect(0.f);

        if (mObjectRoot)
            mInsert->removeChild(mObjectRoot);
    }

    MWWorld::ConstPtr Animation::getPtr() const
    {
        return mPtr;
    }

    MWWorld::Ptr Animation::getPtr()
    {
        return mPtr;
    }

    void Animation::setActive(int active)
    {
        if (mSkeleton)
            mSkeleton->setActive(static_cast<SceneUtil::Skeleton::ActiveType>(active));
    }

    void Animation::updatePtr(const MWWorld::Ptr &ptr)
    {
        mPtr = ptr;
    }

    void Animation::setAccumulation(const osg::Vec3f& accum)
    {
        mAccumulate = accum;

        if (mResetAccumRootCallback)
            mResetAccumRootCallback->setAccumulate(mAccumulate);
    }

    size_t Animation::detectBlendMask(const osg::Node* node) const
    {
        static const char sBlendMaskRoots[sNumBlendMasks][32] = {
            "", /* Lower body / character root */
            "Bip01 Spine1", /* Torso */
            "Bip01 L Clavicle", /* Left arm */
            "Bip01 R Clavicle", /* Right arm */
        };

        while(node != mObjectRoot)
        {
            const std::string &name = node->getName();
            for(size_t i = 1;i < sNumBlendMasks;i++)
            {
                if(name == sBlendMaskRoots[i])
                    return i;
            }

            assert(node->getNumParents() > 0);

            node = node->getParent(0);
        }

        return 0;
    }

    const std::multimap<float, std::string> &Animation::AnimSource::getTextKeys() const
    {
        return mKeyframes->mTextKeys;
    }

    void Animation::loadAllAnimationsInFolder(const std::string &model, const std::string &baseModel)
    {
        const std::map<std::string, VFS::File*>& index = mResourceSystem->getVFS()->getIndex();

        std::string animationPath = model;
        if (animationPath.find("meshes") == 0)
        {
            animationPath.replace(0, 6, "animations");
        }
        animationPath.replace(animationPath.size()-3, 3, "/");

        mResourceSystem->getVFS()->normalizeFilename(animationPath);

        std::map<std::string, VFS::File*>::const_iterator found = index.lower_bound(animationPath);
        while (found != index.end())
        {
            const std::string& name = found->first;
            if (name.size() >= animationPath.size() && name.substr(0, animationPath.size()) == animationPath)
            {
                size_t pos = name.find_last_of('.');
                if (pos != std::string::npos && name.compare(pos, name.size()-pos, ".kf") == 0)
                    addSingleAnimSource(name, baseModel);
            }
            else
                break;
            ++found;
        }
    }

    void Animation::addAnimSource(const std::string &model, const std::string& baseModel)
    {
        std::string kfname = model;
        Misc::StringUtils::lowerCaseInPlace(kfname);

        if(kfname.size() > 4 && kfname.compare(kfname.size()-4, 4, ".nif") == 0)
            kfname.replace(kfname.size()-4, 4, ".kf");
        else
            return;

        addSingleAnimSource(kfname, baseModel);

        static const bool useAdditionalSources = Settings::Manager::getBool ("use additional anim sources", "Game");
        if (useAdditionalSources)
            loadAllAnimationsInFolder(kfname, baseModel);
    }

    void Animation::addSingleAnimSource(const std::string &kfname, const std::string& baseModel)
    {
        if(!mResourceSystem->getVFS()->exists(kfname))
            return;

        std::shared_ptr<AnimSource> animsrc;
        animsrc.reset(new AnimSource);
        animsrc->mKeyframes = mResourceSystem->getKeyframeManager()->get(kfname);

        if (!animsrc->mKeyframes || animsrc->mKeyframes->mTextKeys.empty() || animsrc->mKeyframes->mKeyframeControllers.empty())
            return;

        const NodeMap& nodeMap = getNodeMap();

        for (NifOsg::KeyframeHolder::KeyframeControllerMap::const_iterator it = animsrc->mKeyframes->mKeyframeControllers.begin();
             it != animsrc->mKeyframes->mKeyframeControllers.end(); ++it)
        {
            std::string bonename = Misc::StringUtils::lowerCase(it->first);
            NodeMap::const_iterator found = nodeMap.find(bonename);
            if (found == nodeMap.end())
            {
                Log(Debug::Warning) << "Warning: addAnimSource: can't find bone '" + bonename << "' in " << baseModel << " (referenced by " << kfname << ")";
                continue;
            }

            osg::Node* node = found->second;

            size_t blendMask = detectBlendMask(node);

            // clone the controller, because each Animation needs its own ControllerSource
            osg::ref_ptr<NifOsg::KeyframeController> cloned = new NifOsg::KeyframeController(*it->second, osg::CopyOp::SHALLOW_COPY);
            cloned->setSource(mAnimationTimePtr[blendMask]);

            animsrc->mControllerMap[blendMask].insert(std::make_pair(bonename, cloned));
        }

        mAnimSources.push_back(animsrc);

        SceneUtil::AssignControllerSourcesVisitor assignVisitor(mAnimationTimePtr[0]);
        mObjectRoot->accept(assignVisitor);

        if (!mAccumRoot)
        {
            NodeMap::const_iterator found = nodeMap.find("bip01");
            if (found == nodeMap.end())
                found = nodeMap.find("root bone");

            if (found != nodeMap.end())
                mAccumRoot = found->second;
        }
    }

    void Animation::clearAnimSources()
    {
        mStates.clear();

        for(size_t i = 0;i < sNumBlendMasks;i++)
            mAnimationTimePtr[i]->setTimePtr(std::shared_ptr<float>());

        mAccumCtrl = nullptr;

        mAnimSources.clear();

        mAnimVelocities.clear();
    }

    bool Animation::hasAnimation(const std::string &anim) const
    {
        AnimSourceList::const_iterator iter(mAnimSources.begin());
        for(;iter != mAnimSources.end();++iter)
        {
            const NifOsg::TextKeyMap &keys = (*iter)->getTextKeys();
            if(findGroupStart(keys, anim) != keys.end())
                return true;
        }

        return false;
    }

    float Animation::getStartTime(const std::string &groupname) const
    {
        for(AnimSourceList::const_reverse_iterator iter(mAnimSources.rbegin()); iter != mAnimSources.rend(); ++iter)
        {
            const NifOsg::TextKeyMap &keys = (*iter)->getTextKeys();

            NifOsg::TextKeyMap::const_iterator found = findGroupStart(keys, groupname);
            if(found != keys.end())
                return found->first;
        }
        return -1.f;
    }

    float Animation::getTextKeyTime(const std::string &textKey) const
    {
        for(AnimSourceList::const_reverse_iterator iter(mAnimSources.rbegin()); iter != mAnimSources.rend(); ++iter)
        {
            const NifOsg::TextKeyMap &keys = (*iter)->getTextKeys();

            for(NifOsg::TextKeyMap::const_iterator iterKey(keys.begin()); iterKey != keys.end(); ++iterKey)
            {
                if(iterKey->second.compare(0, textKey.size(), textKey) == 0)
                    return iterKey->first;
            }
        }

        return -1.f;
    }

    void Animation::handleTextKey(AnimState &state, const std::string &groupname, const std::multimap<float, std::string>::const_iterator &key,
                       const std::multimap<float, std::string>& map)
    {
        const std::string &evt = key->second;

        size_t off = groupname.size()+2;
        size_t len = evt.size() - off;

        if(evt.compare(0, groupname.size(), groupname) == 0 &&
           evt.compare(groupname.size(), 2, ": ") == 0)
        {
            if(evt.compare(off, len, "loop start") == 0)
                state.mLoopStartTime = key->first;
            else if(evt.compare(off, len, "loop stop") == 0)
                state.mLoopStopTime = key->first;
        }

        if (mTextKeyListener)
        {
            try
            {
                mTextKeyListener->handleTextKey(groupname, key, map);
            }
            catch (std::exception& e)
            {
                Log(Debug::Error) << "Error handling text key " << evt << ": " << e.what();
            }
        }
    }

    void Animation::play(const std::string &groupname, const AnimPriority& priority, int blendMask, bool autodisable, float speedmult,
                         const std::string &start, const std::string &stop, float startpoint, size_t loops, bool loopfallback)
    {
        if(!mObjectRoot || mAnimSources.empty())
            return;

        if(groupname.empty())
        {
            resetActiveGroups();
            return;
        }

        AnimStateMap::iterator stateiter = mStates.begin();
        while(stateiter != mStates.end())
        {
            if(stateiter->second.mPriority == priority)
                mStates.erase(stateiter++);
            else
                ++stateiter;
        }

        stateiter = mStates.find(groupname);
        if(stateiter != mStates.end())
        {
            stateiter->second.mPriority = priority;
            resetActiveGroups();
            return;
        }

        /* Look in reverse; last-inserted source has priority. */
        AnimState state;
        AnimSourceList::reverse_iterator iter(mAnimSources.rbegin());
        for(;iter != mAnimSources.rend();++iter)
        {
            const NifOsg::TextKeyMap &textkeys = (*iter)->getTextKeys();
            if(reset(state, textkeys, groupname, start, stop, startpoint, loopfallback))
            {
                state.mSource = *iter;
                state.mSpeedMult = speedmult;
                state.mLoopCount = loops;
                state.mPlaying = (state.getTime() < state.mStopTime);
                state.mPriority = priority;
                state.mBlendMask = blendMask;
                state.mAutoDisable = autodisable;
                mStates[groupname] = state;

                if (state.mPlaying)
                {
                    NifOsg::TextKeyMap::const_iterator textkey(textkeys.lower_bound(state.getTime()));
                    while(textkey != textkeys.end() && textkey->first <= state.getTime())
                    {
                        handleTextKey(state, groupname, textkey, textkeys);
                        ++textkey;
                    }
                }

                if(state.getTime() >= state.mLoopStopTime && state.mLoopCount > 0)
                {
                    state.mLoopCount--;
                    state.setTime(state.mLoopStartTime);
                    state.mPlaying = true;
                    if(state.getTime() >= state.mLoopStopTime)
                        break;

                    NifOsg::TextKeyMap::const_iterator textkey(textkeys.lower_bound(state.getTime()));
                    while(textkey != textkeys.end() && textkey->first <= state.getTime())
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

    bool Animation::reset(AnimState &state, const NifOsg::TextKeyMap &keys, const std::string &groupname, const std::string &start, const std::string &stop, float startpoint, bool loopfallback)
    {
        // Look for text keys in reverse. This normally wouldn't matter, but for some reason undeadwolf_2.nif has two
        // separate walkforward keys, and the last one is supposed to be used.
        NifOsg::TextKeyMap::const_reverse_iterator groupend(keys.rbegin());
        for(;groupend != keys.rend();++groupend)
        {
            if(groupend->second.compare(0, groupname.size(), groupname) == 0 &&
               groupend->second.compare(groupname.size(), 2, ": ") == 0)
                break;
        }

        std::string starttag = groupname+": "+start;
        NifOsg::TextKeyMap::const_reverse_iterator startkey(groupend);
        while(startkey != keys.rend() && startkey->second != starttag)
            ++startkey;
        if(startkey == keys.rend() && start == "loop start")
        {
            starttag = groupname+": start";
            startkey = groupend;
            while(startkey != keys.rend() && startkey->second != starttag)
                ++startkey;
        }
        if(startkey == keys.rend())
            return false;

        const std::string stoptag = groupname+": "+stop;
        NifOsg::TextKeyMap::const_reverse_iterator stopkey(groupend);
        while(stopkey != keys.rend()
              // We have to ignore extra garbage at the end.
              // The Scrib's idle3 animation has "Idle3: Stop." instead of "Idle3: Stop".
              // Why, just why? :(
              && (stopkey->second.size() < stoptag.size() || stopkey->second.compare(0,stoptag.size(), stoptag) != 0))
            ++stopkey;
        if(stopkey == keys.rend())
            return false;

        if(startkey->first > stopkey->first)
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

        // mLoopStartTime and mLoopStopTime normally get assigned when encountering these keys while playing the animation
        // (see handleTextKey). But if startpoint is already past these keys, or start time is == stop time, we need to assign them now.
        const std::string loopstarttag = groupname+": loop start";
        const std::string loopstoptag = groupname+": loop stop";

        NifOsg::TextKeyMap::const_reverse_iterator key(groupend);
        for (; key != startkey && key != keys.rend(); ++key)
        {
            if (key->first > state.getTime())
                continue;

            if (key->second == loopstarttag)
                state.mLoopStartTime = key->first;
            else if (key->second == loopstoptag)
                state.mLoopStopTime = key->first;
        }

        return true;
    }

    void Animation::setTextKeyListener(Animation::TextKeyListener *listener)
    {
        mTextKeyListener = listener;
    }

    const Animation::NodeMap &Animation::getNodeMap() const
    {
        if (!mNodeMapCreated && mObjectRoot)
        {
            SceneUtil::NodeMapVisitor visitor(mNodeMap);
            mObjectRoot->accept(visitor);
            mNodeMapCreated = true;
        }
        return mNodeMap;
    }

    void Animation::resetActiveGroups()
    {
        // remove all previous external controllers from the scene graph
        for (ControllerMap::iterator it = mActiveControllers.begin(); it != mActiveControllers.end(); ++it)
        {
            osg::Node* node = it->first;
            node->removeUpdateCallback(it->second);

            // Should be no longer needed with OSG 3.4
            it->second->setNestedCallback(nullptr);
        }

        mActiveControllers.clear();

        mAccumCtrl = nullptr;

        for(size_t blendMask = 0;blendMask < sNumBlendMasks;blendMask++)
        {
            AnimStateMap::const_iterator active = mStates.end();

            AnimStateMap::const_iterator state = mStates.begin();
            for(;state != mStates.end();++state)
            {
                if(!(state->second.mBlendMask&(1<<blendMask)))
                    continue;

                if(active == mStates.end() || active->second.mPriority[(BoneGroup)blendMask] < state->second.mPriority[(BoneGroup)blendMask])
                    active = state;
            }

            mAnimationTimePtr[blendMask]->setTimePtr(active == mStates.end() ? std::shared_ptr<float>() : active->second.mTime);

            // add external controllers for the AnimSource active in this blend mask
            if (active != mStates.end())
            {
                std::shared_ptr<AnimSource> animsrc = active->second.mSource;

                for (AnimSource::ControllerMap::iterator it = animsrc->mControllerMap[blendMask].begin(); it != animsrc->mControllerMap[blendMask].end(); ++it)
                {
                    osg::ref_ptr<osg::Node> node = getNodeMap().at(it->first); // this should not throw, we already checked for the node existing in addAnimSource

                    node->addUpdateCallback(it->second);
                    mActiveControllers.insert(std::make_pair(node, it->second));

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
                        mActiveControllers.insert(std::make_pair(mAccumRoot, mResetAccumRootCallback));
                    }
                }
            }
        }
        addControllers();
    }

    void Animation::adjustSpeedMult(const std::string &groupname, float speedmult)
    {
        AnimStateMap::iterator state(mStates.find(groupname));
        if(state != mStates.end())
            state->second.mSpeedMult = speedmult;
    }

    bool Animation::isPlaying(const std::string &groupname) const
    {
        AnimStateMap::const_iterator state(mStates.find(groupname));
        if(state != mStates.end())
            return state->second.mPlaying;
        return false;
    }

    bool Animation::getInfo(const std::string &groupname, float *complete, float *speedmult) const
    {
        AnimStateMap::const_iterator iter = mStates.find(groupname);
        if(iter == mStates.end())
        {
            if(complete) *complete = 0.0f;
            if(speedmult) *speedmult = 0.0f;
            return false;
        }

        if(complete)
        {
            if(iter->second.mStopTime > iter->second.mStartTime)
                *complete = (iter->second.getTime() - iter->second.mStartTime) /
                            (iter->second.mStopTime - iter->second.mStartTime);
            else
                *complete = (iter->second.mPlaying ? 0.0f : 1.0f);
        }
        if(speedmult) *speedmult = iter->second.mSpeedMult;
        return true;
    }

    float Animation::getCurrentTime(const std::string &groupname) const
    {
        AnimStateMap::const_iterator iter = mStates.find(groupname);
        if(iter == mStates.end())
            return -1.f;

        return iter->second.getTime();
    }

    size_t Animation::getCurrentLoopCount(const std::string& groupname) const
    {
        AnimStateMap::const_iterator iter = mStates.find(groupname);
        if(iter == mStates.end())
            return 0;

        return iter->second.mLoopCount;
    }

    void Animation::disable(const std::string &groupname)
    {
        AnimStateMap::iterator iter = mStates.find(groupname);
        if(iter != mStates.end())
            mStates.erase(iter);
        resetActiveGroups();
    }

    float Animation::getVelocity(const std::string &groupname) const
    {
        if (!mAccumRoot)
            return 0.0f;

        std::map<std::string, float>::const_iterator found = mAnimVelocities.find(groupname);
        if (found != mAnimVelocities.end())
            return found->second;

        // Look in reverse; last-inserted source has priority.
        AnimSourceList::const_reverse_iterator animsrc(mAnimSources.rbegin());
        for(;animsrc != mAnimSources.rend();++animsrc)
        {
            const NifOsg::TextKeyMap &keys = (*animsrc)->getTextKeys();
            if(findGroupStart(keys, groupname) != keys.end())
                break;
        }
        if(animsrc == mAnimSources.rend())
            return 0.0f;

        float velocity = 0.0f;
        const NifOsg::TextKeyMap &keys = (*animsrc)->getTextKeys();

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
        if(!(velocity > 1.0f))
        {
            AnimSourceList::const_reverse_iterator animiter = mAnimSources.rbegin();
            while(*animiter != *animsrc)
                ++animiter;

            while(!(velocity > 1.0f) && ++animiter != mAnimSources.rend())
            {
                const NifOsg::TextKeyMap &keys2 = (*animiter)->getTextKeys();

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

        mAnimVelocities.insert(std::make_pair(groupname, velocity));

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
        // If we have scripted animations, play only them
        bool hasScriptedAnims = false;
        for (AnimStateMap::iterator stateiter = mStates.begin(); stateiter != mStates.end(); stateiter++)
        {
            if (stateiter->second.mPriority.contains(int(MWMechanics::Priority_Persistent)) && stateiter->second.mPlaying)
            {
                hasScriptedAnims = true;
                break;
            }
        }

        osg::Vec3f movement(0.f, 0.f, 0.f);
        AnimStateMap::iterator stateiter = mStates.begin();
        while(stateiter != mStates.end())
        {
            AnimState &state = stateiter->second;
            if (hasScriptedAnims && !state.mPriority.contains(int(MWMechanics::Priority_Persistent)))
            {
                ++stateiter;
                continue;
            }

            const NifOsg::TextKeyMap &textkeys = state.mSource->getTextKeys();
            NifOsg::TextKeyMap::const_iterator textkey(textkeys.upper_bound(state.getTime()));

            float timepassed = duration * state.mSpeedMult;
            while(state.mPlaying)
            {
                if (!state.shouldLoop())
                {
                    float targetTime = state.getTime() + timepassed;
                    if(textkey == textkeys.end() || textkey->first > targetTime)
                    {
                        if(mAccumCtrl && state.mTime == mAnimationTimePtr[0]->getTimePtr())
                            updatePosition(state.getTime(), targetTime, movement);
                        state.setTime(std::min(targetTime, state.mStopTime));
                    }
                    else
                    {
                        if(mAccumCtrl && state.mTime == mAnimationTimePtr[0]->getTimePtr())
                            updatePosition(state.getTime(), textkey->first, movement);
                        state.setTime(textkey->first);
                    }

                    state.mPlaying = (state.getTime() < state.mStopTime);
                    timepassed = targetTime - state.getTime();

                    while(textkey != textkeys.end() && textkey->first <= state.getTime())
                    {
                        handleTextKey(state, stateiter->first, textkey, textkeys);
                        ++textkey;
                    }
                }
                if(state.shouldLoop())
                {
                    state.mLoopCount--;
                    state.setTime(state.mLoopStartTime);
                    state.mPlaying = true;

                    textkey = textkeys.lower_bound(state.getTime());
                    while(textkey != textkeys.end() && textkey->first <= state.getTime())
                    {
                        handleTextKey(state, stateiter->first, textkey, textkeys);
                        ++textkey;
                    }

                    if(state.getTime() >= state.mLoopStopTime)
                        break;
                }

                if(timepassed <= 0.0f)
                    break;
            }

            if(!state.mPlaying && state.mAutoDisable)
            {
                mStates.erase(stateiter++);

                resetActiveGroups();
            }
            else
                ++stateiter;
        }

        updateEffects();

        if (mHeadController)
        {
            const float epsilon = 0.001f;
            bool enable = (std::abs(mHeadPitchRadians) > epsilon || std::abs(mHeadYawRadians) > epsilon);
            mHeadController->setEnabled(enable);
            if (enable)
                mHeadController->setRotate(osg::Quat(mHeadPitchRadians, osg::Vec3f(1,0,0)) * osg::Quat(mHeadYawRadians, osg::Vec3f(0,0,1)));
        }

        // Scripted animations should not cause movement
        if (hasScriptedAnims)
            return osg::Vec3f(0, 0, 0);

        return movement;
    }

    void Animation::setLoopingEnabled(const std::string &groupname, bool enabled)
    {
        AnimStateMap::iterator state(mStates.find(groupname));
        if(state != mStates.end())
            state->second.mLoopingEnabled = enabled;
    }

    void loadBonesFromFile(osg::ref_ptr<osg::Node>& baseNode, const std::string &model, Resource::ResourceSystem* resourceSystem)
    {
        const osg::Node* node = resourceSystem->getSceneManager()->getTemplate(model).get();
        osg::ref_ptr<osg::Node> sheathSkeleton (const_cast<osg::Node*>(node)); // const-trickery required because there is no const version of NodeVisitor

        GetExtendedBonesVisitor getBonesVisitor;
        sheathSkeleton->accept(getBonesVisitor);
        for (auto& nodePair : getBonesVisitor.mFoundBones)
        {
            SceneUtil::FindByNameVisitor findVisitor (nodePair.second->getName());
            baseNode->accept(findVisitor);

            osg::Group* sheathParent = findVisitor.mFoundNode;
            if (sheathParent)
            {
                osg::Node* copy = osg::clone(nodePair.first, osg::CopyOp::DEEP_COPY_NODES);
                sheathParent->addChild(copy);
            }
        }
    }

    void injectCustomBones(osg::ref_ptr<osg::Node>& node, const std::string& model, Resource::ResourceSystem* resourceSystem)
    {
        if (model.empty())
            return;

        const std::map<std::string, VFS::File*>& index = resourceSystem->getVFS()->getIndex();

        std::string animationPath = model;
        if (animationPath.find("meshes") == 0)
        {
            animationPath.replace(0, 6, "animations");
        }
        animationPath.replace(animationPath.size()-4, 4, "/");

        resourceSystem->getVFS()->normalizeFilename(animationPath);

        std::map<std::string, VFS::File*>::const_iterator found = index.lower_bound(animationPath);
        while (found != index.end())
        {
            const std::string& name = found->first;
            if (name.size() >= animationPath.size() && name.substr(0, animationPath.size()) == animationPath)
            {
                size_t pos = name.find_last_of('.');
                if (pos != std::string::npos && name.compare(pos, name.size()-pos, ".nif") == 0)
                    loadBonesFromFile(node, name, resourceSystem);
            }
            else
                break;
            ++found;
        }
    }

    osg::ref_ptr<osg::Node> getModelInstance(Resource::ResourceSystem* resourceSystem, const std::string& model, bool baseonly, bool inject, const std::string& defaultSkeleton)
    {
        Resource::SceneManager* sceneMgr = resourceSystem->getSceneManager();
        if (baseonly)
        {
            typedef std::map<std::string, osg::ref_ptr<osg::Node> > Cache;
            static Cache cache;
            Cache::iterator found = cache.find(model);
            if (found == cache.end())
            {
                osg::ref_ptr<osg::Node> created = sceneMgr->getInstance(model);

                if (inject)
                {
                    injectCustomBones(created, defaultSkeleton, resourceSystem);
                    injectCustomBones(created, model, resourceSystem);
                }

                SceneUtil::CleanObjectRootVisitor removeDrawableVisitor;
                created->accept(removeDrawableVisitor);
                removeDrawableVisitor.remove();

                cache.insert(std::make_pair(model, created));

                return sceneMgr->createInstance(created);
            }
            else
                return sceneMgr->createInstance(found->second);
        }
        else
        {
            osg::ref_ptr<osg::Node> created = sceneMgr->getInstance(model);

            if (inject)
            {
                injectCustomBones(created, defaultSkeleton, resourceSystem);
                injectCustomBones(created, model, resourceSystem);
            }

            return created;
        }
    }

    void Animation::setObjectRoot(const std::string &model, bool forceskeleton, bool baseonly, bool isCreature)
    {
        osg::ref_ptr<osg::StateSet> previousStateset;
        if (mObjectRoot)
        {
            if (mLightListCallback)
                mObjectRoot->removeCullCallback(mLightListCallback);
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

        static const bool useAdditionalSources = Settings::Manager::getBool ("use additional anim sources", "Game");
        std::string defaultSkeleton;
        bool inject = false;

        if (useAdditionalSources && mPtr.getClass().isActor())
        {
            if (isCreature)
            {
                MWWorld::LiveCellRef<ESM::Creature> *ref = mPtr.get<ESM::Creature>();
                if(ref->mBase->mFlags & ESM::Creature::Bipedal)
                {
                    defaultSkeleton = "meshes\\xbase_anim.nif";
                    inject = true;
                }
            }
            else
            {
                inject = true;
                MWWorld::LiveCellRef<ESM::NPC> *ref = mPtr.get<ESM::NPC>();
                if (!ref->mBase->mModel.empty())
                {
                    // If NPC has a custom animation model attached, we should inject bones from default skeleton for given race and gender as well
                    // Since it is a quite rare case, there should not be a noticable performance loss
                    // Note: consider that player and werewolves have no custom animation files attached for now
                    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
                    const ESM::Race *race = store.get<ESM::Race>().find(ref->mBase->mRace);

                    bool isBeast = (race->mData.mFlags & ESM::Race::Beast) != 0;
                    bool isFemale = !ref->mBase->isMale();

                    defaultSkeleton = SceneUtil::getActorSkeleton(false, isFemale, isBeast, false);
                    defaultSkeleton = Misc::ResourceHelpers::correctActorModelPath(defaultSkeleton, mResourceSystem->getVFS());
                }
            }
        }

        if (!forceskeleton)
        {
            osg::ref_ptr<osg::Node> created = getModelInstance(mResourceSystem, model, baseonly, inject, defaultSkeleton);
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
            osg::ref_ptr<osg::Node> created = getModelInstance(mResourceSystem, model, baseonly, inject, defaultSkeleton);
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

    void Animation::addSpellCastGlow(const ESM::MagicEffect *effect, float glowDuration)
    {
        osg::Vec4f glowColor(1,1,1,1);
        glowColor.x() = effect->mData.mRed / 255.f;
        glowColor.y() = effect->mData.mGreen / 255.f;
        glowColor.z() = effect->mData.mBlue / 255.f;

        if (!mGlowUpdater || (mGlowUpdater->isDone() || (mGlowUpdater->isPermanentGlowUpdater() == true)))
        {
            if (mGlowUpdater && mGlowUpdater->isDone())
                mObjectRoot->removeUpdateCallback(mGlowUpdater);

            if (mGlowUpdater && mGlowUpdater->isPermanentGlowUpdater())
            {
                mGlowUpdater->setColor(glowColor);
                mGlowUpdater->setDuration(glowDuration);
            }
            else
                mGlowUpdater = SceneUtil::addEnchantedGlow(mObjectRoot, mResourceSystem, glowColor, glowDuration);
        }
    }

    void Animation::addExtraLight(osg::ref_ptr<osg::Group> parent, const ESM::Light *esmLight)
    {
        bool exterior = mPtr.isInCell() && mPtr.getCell()->getCell()->isExterior();

        SceneUtil::addLight(parent, esmLight, exterior);
    }

    void Animation::addEffect (const std::string& model, int effectId, bool loop, const std::string& bonename, const std::string& texture)
    {
        if (!mObjectRoot.get())
            return;

        // Early out if we already have this effect
        FindVfxCallbacksVisitor visitor(effectId);
        mInsert->accept(visitor);

        for (std::vector<UpdateVfxCallback*>::iterator it = visitor.mCallbacks.begin(); it != visitor.mCallbacks.end(); ++it)
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
            NodeMap::const_iterator found = getNodeMap().find(Misc::StringUtils::lowerCase(bonename));
            if (found == getNodeMap().end())
                throw std::runtime_error("Can't find bone " + bonename);

            parentNode = found->second;
        }

        osg::ref_ptr<osg::PositionAttitudeTransform> trans = new osg::PositionAttitudeTransform;
        if (!mPtr.getClass().isNpc())
        {
            osg::Vec3f bounds (MWBase::Environment::get().getWorld()->getHalfExtents(mPtr) * 2.f / Constants::UnitsPerFoot);
            float scale = std::max({ bounds.x()/3.f, bounds.y()/3.f, bounds.z()/6.f });
            trans->setScale(osg::Vec3f(scale, scale, scale));
        }
        parentNode->addChild(trans);

        osg::ref_ptr<osg::Node> node = mResourceSystem->getSceneManager()->getInstance(model, trans);
        node->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

        SceneUtil::FindMaxControllerLengthVisitor findMaxLengthVisitor;
        node->accept(findMaxLengthVisitor);

        // FreezeOnCull doesn't work so well with effect particles, that tend to have moving emitters
        SceneUtil::DisableFreezeOnCullVisitor disableFreezeOnCullVisitor;
        node->accept(disableFreezeOnCullVisitor);
        node->setNodeMask(SceneUtil::Mask_Effect);

        params.mMaxControllerLength = findMaxLengthVisitor.getMaxLength();
        params.mLoop = loop;
        params.mEffectId = effectId;
        params.mBoneName = bonename;
        params.mAnimTime = std::shared_ptr<EffectAnimationTime>(new EffectAnimationTime);
        trans->addUpdateCallback(new UpdateVfxCallback(params));

        SceneUtil::AssignControllerSourcesVisitor assignVisitor(std::shared_ptr<SceneUtil::ControllerSource>(params.mAnimTime));
        node->accept(assignVisitor);

        // Notify that this animation has attached magic effects
        mHasMagicEffects = true;

        overrideFirstRootTexture(texture, mResourceSystem, node);
    }

    void Animation::removeEffect(int effectId)
    {
        RemoveCallbackVisitor visitor(effectId);
        mInsert->accept(visitor);
        visitor.remove();
        mHasMagicEffects = visitor.mHasMagicEffects;
    }

    void Animation::removeEffects()
    {
        removeEffect(-1);
    }

    void Animation::getLoopingEffects(std::vector<int> &out) const
    {
        if (!mHasMagicEffects)
            return;

        FindVfxCallbacksVisitor visitor;
        mInsert->accept(visitor);

        for (std::vector<UpdateVfxCallback*>::iterator it = visitor.mCallbacks.begin(); it != visitor.mCallbacks.end(); ++it)
        {
            UpdateVfxCallback* callback = *it;

            if (callback->mParams.mLoop && !callback->mFinished)
                out.push_back(callback->mParams.mEffectId);
        }
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

    const osg::Node* Animation::getNode(const std::string &name) const
    {
        std::string lowerName = Misc::StringUtils::lowerCase(name);
        NodeMap::const_iterator found = getNodeMap().find(lowerName);
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
                mTransparencyUpdater = new TransparencyUpdater(alpha, mResourceSystem->getSceneManager()->getShaderManager().getShadowMapAlphaTestEnableUniform());
                mObjectRoot->addUpdateCallback(mTransparencyUpdater);
            }
            else
                mTransparencyUpdater->setAlpha(alpha);
        }
        else
        {
            mObjectRoot->removeUpdateCallback(mTransparencyUpdater);
            mTransparencyUpdater = nullptr;
            mObjectRoot->setStateSet(nullptr);
        }

        setRenderBin();
    }

    void Animation::setRenderBin()
    {
        if (mAlpha != 1.f)
        {
            osg::StateSet* stateset = mObjectRoot->getOrCreateStateSet();
            stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
            stateset->setRenderBinMode(osg::StateSet::OVERRIDE_RENDERBIN_DETAILS);
        }
        else if (osg::StateSet* stateset = mObjectRoot->getStateSet())
            stateset->setRenderBinToInherit();
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

                osg::ref_ptr<osg::Light> light (new osg::Light);
                light->setDiffuse(osg::Vec4f(0,0,0,0));
                light->setSpecular(osg::Vec4f(0,0,0,0));
                light->setAmbient(osg::Vec4f(1.5f,1.5f,1.5f,1.f));

                bool isExterior = mPtr.isInCell() && mPtr.getCell()->getCell()->isExterior();
                SceneUtil::configureLight(light, radius, isExterior);

                mGlowLight = new SceneUtil::LightSource;
                mGlowLight->setNodeMask(SceneUtil::Mask_Lighting);
                mInsert->addChild(mGlowLight);
                mGlowLight->setLight(light);
            }

            mGlowLight->setRadius(radius * cutoffMult);
        }
    }

    void Animation::addControllers()
    {
        mHeadController = nullptr;

        if (mPtr.getClass().isBipedal(mPtr))
        {
            NodeMap::const_iterator found = getNodeMap().find("bip01 head");
            if (found != getNodeMap().end())
            {
                osg::MatrixTransform* node = found->second;

                bool foundKeyframeCtrl = false;
                osg::Callback* cb = node->getUpdateCallback();
                while (cb)
                {
                    if (dynamic_cast<NifOsg::KeyframeController*>(cb))
                    {
                        foundKeyframeCtrl = true;
                        break;
                    }
                    cb = cb->getNestedCallback();
                }

                if (foundKeyframeCtrl)
                {
                    mHeadController = new RotateController(mObjectRoot.get());
                    node->addUpdateCallback(mHeadController);
                    mActiveControllers.insert(std::make_pair(node, mHeadController));
                }
            }
        }
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

    ObjectAnimation::ObjectAnimation(const MWWorld::Ptr &ptr, const std::string &model, Resource::ResourceSystem* resourceSystem, bool animated, bool allowLight)
        : Animation(ptr, osg::ref_ptr<osg::Group>(ptr.getRefData().getBaseNode()), resourceSystem)
    {
        if (!model.empty())
        {
            setObjectRoot(model, false, false, false);
            if (animated)
                addAnimSource(model, model);

            if (!ptr.getClass().getEnchantment(ptr).empty())
                mGlowUpdater = SceneUtil::addEnchantedGlow(mObjectRoot, mResourceSystem, ptr.getClass().getEnchantmentColor(ptr));
        }
        if (ptr.getTypeName() == typeid(ESM::Light).name() && allowLight)
            addExtraLight(getOrCreateObjectRoot(), ptr.get<ESM::Light>()->mBase);

        if (!allowLight && mObjectRoot)
        {
            RemoveParticlesVisitor visitor;
            mObjectRoot->accept(visitor);
            visitor.remove();
        }

        if (SceneUtil::hasUserDescription(mObjectRoot, Constants::NightDayLabel))
        {
            AddSwitchCallbacksVisitor visitor;
            mObjectRoot->accept(visitor);
        }

        if (ptr.getRefData().getCustomData() != nullptr && canBeHarvested())
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
        if (mPtr.getTypeName() != typeid(ESM::Container).name())
            return false;

        const MWWorld::LiveCellRef<ESM::Container>* ref = mPtr.get<ESM::Container>();
        if (!(ref->mBase->mFlags & ESM::Container::Organic))
            return false;

        return SceneUtil::hasUserDescription(mObjectRoot, Constants::HerbalismLabel);
    }

    Animation::AnimState::~AnimState()
    {

    }

    // ------------------------------

    PartHolder::PartHolder(osg::ref_ptr<osg::Node> node)
        : mNode(node)
    {
    }

    PartHolder::~PartHolder()
    {
        if (mNode.get() && !mNode->getNumParents())
            Log(Debug::Verbose) << "Part \"" << mNode->getName() << "\" has no parents" ;

        if (mNode.get() && mNode->getNumParents())
        {
            if (mNode->getNumParents() > 1)
                Log(Debug::Verbose) << "Part \"" << mNode->getName() << "\" has multiple (" << mNode->getNumParents() << ") parents";
            mNode->getParent(0)->removeChild(mNode);
        }
    }
}
