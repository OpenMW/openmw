#include "animation.hpp"

#include <iomanip>
#include <limits>

#include <osg/PositionAttitudeTransform>
#include <osg/TexGen>
#include <osg/TexEnvCombine>
#include <osg/ComputeBoundsVisitor>
#include <osg/MatrixTransform>
#include <osg/Geode>
#include <osg/BlendFunc>
#include <osg/Material>
#include <osg/Version>

#include <osgParticle/ParticleSystem>

#include <components/nifosg/nifloader.hpp>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/resource/texturemanager.hpp>

#include <components/nifosg/nifloader.hpp> // KeyframeHolder
#include <components/nifosg/controller.hpp>

#include <components/vfs/manager.hpp>

#include <components/sceneutil/statesetupdater.hpp>
#include <components/sceneutil/visitor.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/util.hpp>
#include <components/sceneutil/lightcontroller.hpp>
#include <components/sceneutil/skeleton.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/fallback.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwmechanics/character.hpp" // FIXME: for MWMechanics::Priority

#include "vismask.hpp"
#include "util.hpp"
#include "rotatecontroller.hpp"

namespace
{

    class GlowUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        GlowUpdater(osg::Vec4f color, const std::vector<osg::ref_ptr<osg::Texture2D> >& textures)
            : mTexUnit(1) // FIXME: might not always be 1
            , mColor(color)
            , mTextures(textures)
        {
        }

        virtual void setDefaults(osg::StateSet *stateset)
        {
            stateset->setTextureMode(mTexUnit, GL_TEXTURE_2D, osg::StateAttribute::ON);

            osg::TexGen* texGen = new osg::TexGen;
            texGen->setMode(osg::TexGen::SPHERE_MAP);

            stateset->setTextureAttributeAndModes(mTexUnit, texGen, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);

            osg::TexEnvCombine* texEnv = new osg::TexEnvCombine;
            texEnv->setSource0_RGB(osg::TexEnvCombine::CONSTANT);
            texEnv->setConstantColor(mColor);
            texEnv->setCombine_RGB(osg::TexEnvCombine::INTERPOLATE);
            texEnv->setSource2_RGB(osg::TexEnvCombine::TEXTURE);
            texEnv->setOperand2_RGB(osg::TexEnvCombine::SRC_COLOR);

            stateset->setTextureAttributeAndModes(mTexUnit, texEnv, osg::StateAttribute::ON);
        }

        virtual void apply(osg::StateSet *stateset, osg::NodeVisitor *nv)
        {
            float time = nv->getFrameStamp()->getSimulationTime();
            int index = (int)(time*16) % mTextures.size();
            stateset->setTextureAttribute(mTexUnit, mTextures[index], osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
        }

    private:
        int mTexUnit;
        osg::Vec4f mColor;
        std::vector<osg::ref_ptr<osg::Texture2D> > mTextures;
    };

    class NodeMapVisitor : public osg::NodeVisitor
    {
    public:
        NodeMapVisitor() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}

        void apply(osg::MatrixTransform& trans)
        {
            mMap[Misc::StringUtils::lowerCase(trans.getName())] = &trans;
            traverse(trans);
        }

        typedef std::map<std::string, osg::ref_ptr<osg::MatrixTransform> > NodeMap;

        const NodeMap& getNodeMap() const
        {
            return mMap;
        }

    private:
        NodeMap mMap;
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
                it->second->removeChild(it->first);
        }

    protected:
        // <node to remove, parent node to remove it from>
        typedef std::vector<std::pair<osg::Node*, osg::Group*> > RemoveVec;
        std::vector<std::pair<osg::Node*, osg::Group*> > mToRemove;
    };

    // Removes all drawables from a graph.
    class RemoveDrawableVisitor : public RemoveVisitor
    {
    public:
        virtual void apply(osg::Geode &geode)
        {
            applyImpl(geode);
        }

#if OSG_VERSION_GREATER_OR_EQUAL(3,3,3)
        virtual void apply(osg::Drawable& drw)
        {
            applyImpl(drw);
        }
#endif

        void applyImpl(osg::Node& node)
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
        virtual void apply(osg::Geode &node)
        {
            applyImpl(node);
        }

#if OSG_VERSION_GREATER_OR_EQUAL(3,3,3)
        virtual void apply(osg::Drawable& drw)
        {
            applyImpl(drw);
        }
#endif

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

    struct Animation::AnimSource
    {
        osg::ref_ptr<const NifOsg::KeyframeHolder> mKeyframes;

        typedef std::map<std::string, osg::ref_ptr<NifOsg::KeyframeController> > ControllerMap;

        ControllerMap mControllerMap[Animation::sNumBlendMasks];

        const std::multimap<float, std::string>& getTextKeys();
    };

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
        , mPtr(ptr)
        , mResourceSystem(resourceSystem)
        , mAccumulate(1.f, 1.f, 0.f)
        , mTextKeyListener(NULL)
        , mHeadYawRadians(0.f)
        , mHeadPitchRadians(0.f)
        , mAlpha(1.f)
    {
        for(size_t i = 0;i < sNumBlendMasks;i++)
            mAnimationTimePtr[i].reset(new AnimationTime);
    }

    Animation::~Animation()
    {
        setLightEffect(0.f);

        if (mObjectRoot)
            mInsert->removeChild(mObjectRoot);
    }

    MWWorld::Ptr Animation::getPtr()
    {
        return mPtr;
    }

    void Animation::setActive(bool active)
    {
        if (SceneUtil::Skeleton* skel = dynamic_cast<SceneUtil::Skeleton*>(mObjectRoot.get()))
        {
            skel->setActive(active);
        }
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

    size_t Animation::detectBlendMask(osg::Node* node)
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

    const std::multimap<float, std::string> &Animation::AnimSource::getTextKeys()
    {
        return mKeyframes->mTextKeys;
    }

    void Animation::addAnimSource(const std::string &model)
    {
        std::string kfname = model;
        Misc::StringUtils::toLower(kfname);

        if(kfname.size() > 4 && kfname.compare(kfname.size()-4, 4, ".nif") == 0)
            kfname.replace(kfname.size()-4, 4, ".kf");

        if(!mResourceSystem->getVFS()->exists(kfname))
            return;

        boost::shared_ptr<AnimSource> animsrc;
        animsrc.reset(new AnimSource);
        animsrc->mKeyframes = mResourceSystem->getSceneManager()->getKeyframes(kfname);

        if (animsrc->mKeyframes->mTextKeys.empty() || animsrc->mKeyframes->mKeyframeControllers.empty())
            return;

        for (NifOsg::KeyframeHolder::KeyframeControllerMap::const_iterator it = animsrc->mKeyframes->mKeyframeControllers.begin();
             it != animsrc->mKeyframes->mKeyframeControllers.end(); ++it)
        {
            std::string bonename = Misc::StringUtils::lowerCase(it->first);
            NodeMap::const_iterator found = mNodeMap.find(bonename);
            if (found == mNodeMap.end())
            {
                std::cerr << "addAnimSource: can't find bone '" + bonename << "' in " << model << " (referenced by " << kfname << ")" << std::endl;
                continue;
            }

            osg::Node* node = found->second;

            size_t blendMask = detectBlendMask(node);

            // clone the controller, because each Animation needs its own ControllerSource
            osg::ref_ptr<NifOsg::KeyframeController> cloned = osg::clone(it->second.get(), osg::CopyOp::DEEP_COPY_ALL);
            cloned->setSource(mAnimationTimePtr[blendMask]);

            animsrc->mControllerMap[blendMask].insert(std::make_pair(bonename, cloned));
        }

        mAnimSources.push_back(animsrc);

        SceneUtil::AssignControllerSourcesVisitor assignVisitor(mAnimationTimePtr[0]);
        mObjectRoot->accept(assignVisitor);

        if (!mAccumRoot)
        {
            NodeMap::const_iterator found = mNodeMap.find("root bone");
            if (found == mNodeMap.end())
                found = mNodeMap.find("bip01");

            if (found != mNodeMap.end())
                mAccumRoot = found->second;
        }
    }

    void Animation::clearAnimSources()
    {
        mStates.clear();

        for(size_t i = 0;i < sNumBlendMasks;i++)
            mAnimationTimePtr[i]->setTimePtr(boost::shared_ptr<float>());

        mAccumCtrl = NULL;

        mAnimSources.clear();
    }

    bool Animation::hasAnimation(const std::string &anim)
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
        for(AnimSourceList::const_iterator iter(mAnimSources.begin()); iter != mAnimSources.end(); ++iter)
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
        for(AnimSourceList::const_iterator iter(mAnimSources.begin()); iter != mAnimSources.end(); ++iter)
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
            mTextKeyListener->handleTextKey(groupname, key, map);
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

                NifOsg::TextKeyMap::const_iterator textkey(textkeys.lower_bound(state.getTime()));
                if (state.mPlaying)
                {
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
        if(iter == mAnimSources.rend())
            std::cerr<< "Failed to find animation "<<groupname<<" for "<<mPtr.getCellRef().getRefId() <<std::endl;

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
              && (stopkey->second.size() < stoptag.size() || stopkey->second.substr(0,stoptag.size()) != stoptag))
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

    void Animation::resetActiveGroups()
    {
        // remove all previous external controllers from the scene graph
        for (ControllerMap::iterator it = mActiveControllers.begin(); it != mActiveControllers.end(); ++it)
        {
            osg::Node* node = it->first;
            node->removeUpdateCallback(it->second);

            // Should be no longer needed with OSG 3.4
            it->second->setNestedCallback(NULL);
        }

        mActiveControllers.clear();

        mAccumCtrl = NULL;

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

            mAnimationTimePtr[blendMask]->setTimePtr(active == mStates.end() ? boost::shared_ptr<float>() : active->second.mTime);

            // add external controllers for the AnimSource active in this blend mask
            if (active != mStates.end())
            {
                boost::shared_ptr<AnimSource> animsrc = active->second.mSource;

                for (AnimSource::ControllerMap::iterator it = animsrc->mControllerMap[blendMask].begin(); it != animsrc->mControllerMap[blendMask].end(); ++it)
                {
                    osg::ref_ptr<osg::Node> node = mNodeMap.at(it->first); // this should not throw, we already checked for the node existing in addAnimSource

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

    void Animation::stopLooping(const std::string& groupname)
    {
        AnimStateMap::iterator stateiter = mStates.find(groupname);
        if(stateiter != mStates.end())
        {
            stateiter->second.mLoopCount = 0;
            return;
        }
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
                const NifOsg::TextKeyMap &keys = (*animiter)->getTextKeys();

                const AnimSource::ControllerMap& ctrls = (*animiter)->mControllerMap[0];
                for (AnimSource::ControllerMap::const_iterator it = ctrls.begin(); it != ctrls.end(); ++it)
                {
                    if (Misc::StringUtils::ciEqual(it->first, mAccumRoot->getName()))
                    {
                        velocity = calcAnimVelocity(keys, it->second, mAccumulate, groupname);
                        break;
                    }
                }
            }
        }

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
        while(stateiter != mStates.end())
        {
            AnimState &state = stateiter->second;
            const NifOsg::TextKeyMap &textkeys = state.mSource->getTextKeys();
            NifOsg::TextKeyMap::const_iterator textkey(textkeys.upper_bound(state.getTime()));

            float timepassed = duration * state.mSpeedMult;
            while(state.mPlaying)
            {
                float targetTime;

                if(state.getTime() >= state.mLoopStopTime && state.mLoopCount > 0)
                    goto handle_loop;

                targetTime = state.getTime() + timepassed;
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

                if(state.getTime() >= state.mLoopStopTime && state.mLoopCount > 0)
                {
                handle_loop:
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

        updateEffects(duration);

        if (mHeadController)
        {
            const float epsilon = 0.001f;
            bool enable = (std::abs(mHeadPitchRadians) > epsilon || std::abs(mHeadYawRadians) > epsilon);
            mHeadController->setEnabled(enable);
            if (enable)
                mHeadController->setRotate(osg::Quat(mHeadPitchRadians, osg::Vec3f(1,0,0)) * osg::Quat(mHeadYawRadians, osg::Vec3f(0,0,1)));
        }

        return movement;
    }

    void Animation::setObjectRoot(const std::string &model, bool forceskeleton, bool baseonly, bool isCreature)
    {
        osg::ref_ptr<osg::StateSet> previousStateset;
        if (mObjectRoot)
        {
            previousStateset = mObjectRoot->getStateSet();
            mObjectRoot->getParent(0)->removeChild(mObjectRoot);
        }
        mObjectRoot = NULL;

        mNodeMap.clear();
        mActiveControllers.clear();
        mAccumRoot = NULL;
        mAccumCtrl = NULL;

        if (!forceskeleton)
            mObjectRoot = mResourceSystem->getSceneManager()->createInstance(model, mInsert);
        else
        {
            osg::ref_ptr<osg::Node> newObjectRoot = mResourceSystem->getSceneManager()->createInstance(model);
            if (!dynamic_cast<SceneUtil::Skeleton*>(newObjectRoot.get()))
            {
                osg::ref_ptr<SceneUtil::Skeleton> skel = new SceneUtil::Skeleton;
                skel->addChild(newObjectRoot);
                newObjectRoot = skel;
            }
            mInsert->addChild(newObjectRoot);
            mObjectRoot = newObjectRoot;
        }

        if (previousStateset)
            mObjectRoot->setStateSet(previousStateset);

        if (baseonly)
        {
            RemoveDrawableVisitor removeDrawableVisitor;
            mObjectRoot->accept(removeDrawableVisitor);
            removeDrawableVisitor.remove();
        }

        if (isCreature)
        {
            RemoveTriBipVisitor removeTriBipVisitor;
            mObjectRoot->accept(removeTriBipVisitor);
            removeTriBipVisitor.remove();
        }

        NodeMapVisitor visitor;
        mObjectRoot->accept(visitor);
        mNodeMap = visitor.getNodeMap();

        mObjectRoot->addCullCallback(new SceneUtil::LightListCallback);
    }

    osg::Group* Animation::getObjectRoot()
    {
        return static_cast<osg::Group*>(mObjectRoot.get());
    }

    osg::Group* Animation::getOrCreateObjectRoot()
    {
        if (mObjectRoot)
            return static_cast<osg::Group*>(mObjectRoot.get());

        mObjectRoot = new osg::Group;
        mInsert->addChild(mObjectRoot);
        return static_cast<osg::Group*>(mObjectRoot.get());
    }

    void Animation::addGlow(osg::ref_ptr<osg::Node> node, osg::Vec4f glowColor)
    {
        std::vector<osg::ref_ptr<osg::Texture2D> > textures;
        for (int i=0; i<32; ++i)
        {
            std::stringstream stream;
            stream << "textures/magicitem/caust";
            stream << std::setw(2);
            stream << std::setfill('0');
            stream << i;
            stream << ".dds";

            textures.push_back(mResourceSystem->getTextureManager()->getTexture2D(stream.str(), osg::Texture2D::REPEAT, osg::Texture2D::REPEAT));
        }

        osg::ref_ptr<GlowUpdater> glowupdater (new GlowUpdater(glowColor, textures));
        node->addUpdateCallback(glowupdater);
    }

    // TODO: Should not be here
    osg::Vec4f Animation::getEnchantmentColor(MWWorld::Ptr item)
    {
        osg::Vec4f result(1,1,1,1);
        std::string enchantmentName = item.getClass().getEnchantment(item);
        if (enchantmentName.empty())
            return result;
        const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(enchantmentName);
        assert (enchantment->mEffects.mList.size());
        const ESM::MagicEffect* magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(
                enchantment->mEffects.mList.front().mEffectID);
        result.x() = magicEffect->mData.mRed / 255.f;
        result.y() = magicEffect->mData.mGreen / 255.f;
        result.z() = magicEffect->mData.mBlue / 255.f;
        return result;
    }

    void Animation::addExtraLight(osg::ref_ptr<osg::Group> parent, const ESM::Light *esmLight)
    {
        SceneUtil::FindByNameVisitor visitor("AttachLight");
        parent->accept(visitor);

        osg::Group* attachTo = NULL;
        if (visitor.mFoundNode)
        {
            attachTo = visitor.mFoundNode;
        }
        else
        {
            osg::ComputeBoundsVisitor computeBound;
            computeBound.setTraversalMask(~Mask_ParticleSystem);
            parent->accept(computeBound);

            // PositionAttitudeTransform seems to be slightly faster than MatrixTransform
            osg::ref_ptr<osg::PositionAttitudeTransform> trans(new osg::PositionAttitudeTransform);
            trans->setPosition(computeBound.getBoundingBox().center());

            parent->addChild(trans);

            attachTo = trans;
        }

        osg::ref_ptr<SceneUtil::LightSource> lightSource = new SceneUtil::LightSource;
        osg::Light* light = new osg::Light;
        lightSource->setLight(light);
        lightSource->setNodeMask(Mask_Lighting);

        const MWWorld::Fallback* fallback = MWBase::Environment::get().getWorld()->getFallback();

        float radius = esmLight->mData.mRadius;
        lightSource->setRadius(radius);

        static bool outQuadInLin = fallback->getFallbackBool("LightAttenuation_OutQuadInLin");
        static bool useQuadratic = fallback->getFallbackBool("LightAttenuation_UseQuadratic");
        static float quadraticValue = fallback->getFallbackFloat("LightAttenuation_QuadraticValue");
        static float quadraticRadiusMult = fallback->getFallbackFloat("LightAttenuation_QuadraticRadiusMult");
        static bool useLinear = fallback->getFallbackBool("LightAttenuation_UseLinear");
        static float linearRadiusMult = fallback->getFallbackFloat("LightAttenuation_LinearRadiusMult");
        static float linearValue = fallback->getFallbackFloat("LightAttenuation_LinearValue");

        bool exterior = mPtr.isInCell() && mPtr.getCell()->getCell()->isExterior();

        SceneUtil::configureLight(light, radius, exterior, outQuadInLin, useQuadratic, quadraticValue,
                                  quadraticRadiusMult, useLinear, linearRadiusMult, linearValue);

        osg::Vec4f diffuse = SceneUtil::colourFromRGB(esmLight->mData.mColor);
        if (esmLight->mData.mFlags & ESM::Light::Negative)
        {
            diffuse *= -1;
            diffuse.a() = 1;
        }
        light->setDiffuse(diffuse);
        light->setAmbient(osg::Vec4f(0,0,0,1));
        light->setSpecular(osg::Vec4f(0,0,0,0));

        osg::ref_ptr<SceneUtil::LightController> ctrl (new SceneUtil::LightController);
        ctrl->setDiffuse(light->getDiffuse());
        if (esmLight->mData.mFlags & ESM::Light::Flicker)
            ctrl->setType(SceneUtil::LightController::LT_Flicker);
        if (esmLight->mData.mFlags & ESM::Light::FlickerSlow)
            ctrl->setType(SceneUtil::LightController::LT_FlickerSlow);
        if (esmLight->mData.mFlags & ESM::Light::Pulse)
            ctrl->setType(SceneUtil::LightController::LT_Pulse);
        if (esmLight->mData.mFlags & ESM::Light::PulseSlow)
            ctrl->setType(SceneUtil::LightController::LT_PulseSlow);

        lightSource->addUpdateCallback(ctrl);

        attachTo->addChild(lightSource);
    }

    void Animation::addEffect (const std::string& model, int effectId, bool loop, const std::string& bonename, std::string texture)
    {
        if (!mObjectRoot.get())
            return;

        // Early out if we already have this effect
        for (std::vector<EffectParams>::iterator it = mEffects.begin(); it != mEffects.end(); ++it)
            if (it->mLoop && loop && it->mEffectId == effectId && it->mBoneName == bonename)
                return;

        EffectParams params;
        params.mModelName = model;
        osg::ref_ptr<osg::Group> parentNode;
        if (bonename.empty())
            parentNode = mInsert;
        else
        {
            NodeMap::iterator found = mNodeMap.find(Misc::StringUtils::lowerCase(bonename));
            if (found == mNodeMap.end())
                throw std::runtime_error("Can't find bone " + bonename);

            parentNode = found->second;
        }
        osg::ref_ptr<osg::Node> node = mResourceSystem->getSceneManager()->createInstance(model, parentNode);

        node->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

        params.mObjects = PartHolderPtr(new PartHolder(node));

        SceneUtil::FindMaxControllerLengthVisitor findMaxLengthVisitor;
        node->accept(findMaxLengthVisitor);

        // FreezeOnCull doesn't work so well with effect particles, that tend to have moving emitters
        SceneUtil::DisableFreezeOnCullVisitor disableFreezeOnCullVisitor;
        node->accept(disableFreezeOnCullVisitor);

        params.mMaxControllerLength = findMaxLengthVisitor.getMaxLength();

        node->setNodeMask(Mask_Effect);

        params.mLoop = loop;
        params.mEffectId = effectId;
        params.mBoneName = bonename;

        params.mAnimTime = boost::shared_ptr<EffectAnimationTime>(new EffectAnimationTime);

        SceneUtil::AssignControllerSourcesVisitor assignVisitor(boost::shared_ptr<SceneUtil::ControllerSource>(params.mAnimTime));
        node->accept(assignVisitor);

        overrideTexture(texture, mResourceSystem, node);

        // TODO: in vanilla morrowind the effect is scaled based on the host object's bounding box.

        mEffects.push_back(params);
    }

    void Animation::removeEffect(int effectId)
    {
        for (std::vector<EffectParams>::iterator it = mEffects.begin(); it != mEffects.end(); ++it)
        {
            if (it->mEffectId == effectId)
            {
                mEffects.erase(it);
                return;
            }
        }
    }

    void Animation::getLoopingEffects(std::vector<int> &out)
    {
        for (std::vector<EffectParams>::iterator it = mEffects.begin(); it != mEffects.end(); ++it)
        {
            if (it->mLoop)
                out.push_back(it->mEffectId);
        }
    }

    void Animation::updateEffects(float duration)
    {
        for (std::vector<EffectParams>::iterator it = mEffects.begin(); it != mEffects.end(); )
        {
            it->mAnimTime->addTime(duration);

            if (it->mAnimTime->getTime() >= it->mMaxControllerLength)
            {
                if (it->mLoop)
                {
                    // Start from the beginning again; carry over the remainder
                    // Not sure if this is actually needed, the controller function might already handle loops
                    float remainder = it->mAnimTime->getTime() - it->mMaxControllerLength;
                    it->mAnimTime->resetTime(remainder);
                }
                else
                {
                    it = mEffects.erase(it);
                    continue;
                }
            }
            ++it;
        }
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
        NodeMap::const_iterator found = mNodeMap.find(lowerName);
        if (found == mNodeMap.end())
            return NULL;
        else
            return found->second;
    }

    void Animation::setAlpha(float alpha)
    {
        if (alpha == mAlpha)
            return;
        mAlpha = alpha;

        if (alpha != 1.f)
        {
            osg::StateSet* stateset (new osg::StateSet);

            osg::BlendFunc* blendfunc (new osg::BlendFunc);
            stateset->setAttributeAndModes(blendfunc, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);

            // FIXME: overriding diffuse/ambient/emissive colors
            osg::Material* material (new osg::Material);
            material->setColorMode(osg::Material::OFF);
            material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,alpha));
            material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,1));
            stateset->setAttributeAndModes(material, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);

            mObjectRoot->setStateSet(stateset);
        }
        else
        {
            mObjectRoot->setStateSet(NULL);
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
                mGlowLight = NULL;
            }
        }
        else
        {
            if (!mGlowLight)
            {
                mGlowLight = new SceneUtil::LightSource;
                mGlowLight->setLight(new osg::Light);
                mGlowLight->setNodeMask(Mask_Lighting);
                osg::Light* light = mGlowLight->getLight();
                light->setDiffuse(osg::Vec4f(0,0,0,0));
                light->setSpecular(osg::Vec4f(0,0,0,0));
                light->setAmbient(osg::Vec4f(1.5f,1.5f,1.5f,1.f));
                mInsert->addChild(mGlowLight);
            }

            effect += 3;
            osg::Light* light = mGlowLight->getLight();
            mGlowLight->setRadius(effect * 66.f);
            light->setLinearAttenuation(0.5f/effect);
        }
    }

    void Animation::addControllers()
    {
        mHeadController = NULL;

        if (mPtr.getClass().isBipedal(mPtr))
        {
            NodeMap::iterator found = mNodeMap.find("bip01 head");
            if (found != mNodeMap.end() && dynamic_cast<osg::MatrixTransform*>(found->second.get()))
            {
                osg::Node* node = found->second;
                mHeadController = new RotateController(mObjectRoot.get());
                node->addUpdateCallback(mHeadController);
                mActiveControllers.insert(std::make_pair(node, mHeadController));
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
                addAnimSource(model);

            if (!ptr.getClass().getEnchantment(ptr).empty())
                addGlow(mObjectRoot, getEnchantmentColor(ptr));
        }
        if (ptr.getTypeName() == typeid(ESM::Light).name() && allowLight)
            addExtraLight(getOrCreateObjectRoot(), ptr.get<ESM::Light>()->mBase);
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
        if (mNode->getNumParents())
            mNode->getParent(0)->removeChild(mNode);
    }

}
