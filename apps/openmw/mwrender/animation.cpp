#include "animation.hpp"

#include <iomanip>
#include <limits>

#include <osg/PositionAttitudeTransform>
#include <osg/TexGen>
#include <osg/TexEnvCombine>
#include <osg/ComputeBoundsVisitor>
#include <osg/MatrixTransform>
#include <osg/io_utils>

#include <components/nifosg/nifloader.hpp>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/resource/texturemanager.hpp>

#include <components/nifosg/nifloader.hpp> // KeyframeHolder

#include <components/vfs/manager.hpp>

#include <components/misc/resourcehelpers.hpp>

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

#include "vismask.hpp"
#include "util.hpp"

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
}

namespace MWRender
{

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
        : mPtr(ptr)
        , mInsert(parentNode)
        , mResourceSystem(resourceSystem)
        , mAccumulate(1.f, 1.f, 0.f)
    {
        for(size_t i = 0;i < sNumGroups;i++)
            mAnimationTimePtr[i].reset(new AnimationTime(this));
    }

    Animation::~Animation()
    {
        if (mObjectRoot)
            mInsert->removeChild(mObjectRoot);
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

    size_t Animation::detectAnimGroup(osg::Node* node)
    {
        static const char sGroupRoots[sNumGroups][32] = {
            "", /* Lower body / character root */
            "Bip01 Spine1", /* Torso */
            "Bip01 L Clavicle", /* Left arm */
            "Bip01 R Clavicle", /* Right arm */
        };

        while(node != mObjectRoot)
        {
            const std::string &name = node->getName();
            for(size_t i = 1;i < sNumGroups;i++)
            {
                if(name == sGroupRoots[i])
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
                throw std::runtime_error("addAnimSource: can't find bone " + bonename);

            osg::Node* node = found->second;

            size_t group = detectAnimGroup(node);

            // clone the controller, because each Animation needs its own ControllerSource
            osg::ref_ptr<NifOsg::KeyframeController> cloned = osg::clone(it->second.get(), osg::CopyOp::DEEP_COPY_ALL);
            cloned->setSource(mAnimationTimePtr[group]);

            animsrc->mControllerMap[group].insert(std::make_pair(bonename, cloned));
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

        for(size_t i = 0;i < sNumGroups;i++)
            mAnimationTimePtr[i]->setAnimName(std::string());

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

        if(evt.compare(off, len, "loop start") == 0)
            state.mLoopStartTime = key->first;
        else if(evt.compare(off, len, "loop stop") == 0)
            state.mLoopStopTime = key->first;

        // TODO: forward to listener?
    }

    void Animation::play(const std::string &groupname, int priority, int groups, bool autodisable, float speedmult,
                         const std::string &start, const std::string &stop, float startpoint, size_t loops, bool loopfallback)
    {
        if(!mObjectRoot || mAnimSources.empty())
            return;

        if(groupname.empty())
        {
            resetActiveGroups();
            return;
        }

        priority = std::max(0, priority);

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
                state.mPlaying = (state.mTime < state.mStopTime);
                state.mPriority = priority;
                state.mGroups = groups;
                state.mAutoDisable = autodisable;
                mStates[groupname] = state;

                NifOsg::TextKeyMap::const_iterator textkey(textkeys.lower_bound(state.mTime));
                if (state.mPlaying)
                {
                    while(textkey != textkeys.end() && textkey->first <= state.mTime)
                    {
                        handleTextKey(state, groupname, textkey, textkeys);
                        ++textkey;
                    }
                }

                if(state.mTime >= state.mLoopStopTime && state.mLoopCount > 0)
                {
                    state.mLoopCount--;
                    state.mTime = state.mLoopStartTime;
                    state.mPlaying = true;
                    if(state.mTime >= state.mLoopStopTime)
                        break;

                    NifOsg::TextKeyMap::const_iterator textkey(textkeys.lower_bound(state.mTime));
                    while(textkey != textkeys.end() && textkey->first <= state.mTime)
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

        state.mTime = state.mStartTime + ((state.mStopTime - state.mStartTime) * startpoint);

        // mLoopStartTime and mLoopStopTime normally get assigned when encountering these keys while playing the animation
        // (see handleTextKey). But if startpoint is already past these keys, we need to assign them now.
        if(state.mTime > state.mStartTime)
        {
            const std::string loopstarttag = groupname+": loop start";
            const std::string loopstoptag = groupname+": loop stop";

            NifOsg::TextKeyMap::const_reverse_iterator key(groupend);
            for (; key != startkey && key != keys.rend(); ++key)
            {
                if (key->first > state.mTime)
                    continue;

                if (key->second == loopstarttag)
                    state.mLoopStartTime = key->first;
                else if (key->second == loopstoptag)
                    state.mLoopStopTime = key->first;
            }
        }

        return true;
    }

    void Animation::resetActiveGroups()
    {
        // remove all previous external controllers from the scene graph
        for (AnimSourceControllerMap::iterator it = mAnimSourceControllers.begin(); it != mAnimSourceControllers.end(); ++it)
        {
            osg::Node* node = it->first;
            node->removeUpdateCallback(it->second);
        }
        if (mResetAccumRootCallback && mAccumRoot)
            mAccumRoot->removeUpdateCallback(mResetAccumRootCallback);
        mAnimSourceControllers.clear();

        mAccumCtrl = NULL;

        for(size_t grp = 0;grp < sNumGroups;grp++)
        {
            AnimStateMap::const_iterator active = mStates.end();

            AnimStateMap::const_iterator state = mStates.begin();
            for(;state != mStates.end();++state)
            {
                if(!(state->second.mGroups&(1<<grp)))
                    continue;

                if(active == mStates.end() || active->second.mPriority < state->second.mPriority)
                    active = state;
            }

            mAnimationTimePtr[grp]->setAnimName((active == mStates.end()) ?
                                                 std::string() : active->first);

            // add external controllers for the AnimSource active in this group
            if (active != mStates.end())
            {
                boost::shared_ptr<AnimSource> animsrc = active->second.mSource;

                for (AnimSource::ControllerMap::iterator it = animsrc->mControllerMap[grp].begin(); it != animsrc->mControllerMap[grp].end(); ++it)
                {
                    osg::ref_ptr<osg::Node> node = mNodeMap.at(it->first); // this should not throw, we already checked for the node existing in addAnimSource

                    node->addUpdateCallback(it->second);
                    mAnimSourceControllers[node] = it->second;

                    if (grp == 0 && node == mAccumRoot)
                    {
                        mAccumCtrl = it->second;

                        if (!mResetAccumRootCallback)
                        {
                            mResetAccumRootCallback = new ResetAccumRootCallback;
                            mResetAccumRootCallback->setAccumulate(mAccumulate);
                        }
                        mAccumRoot->addUpdateCallback(mResetAccumRootCallback);
                    }
                }
            }
        }
    }

    void Animation::changeGroups(const std::string &groupname, int groups)
    {
        AnimStateMap::iterator stateiter = mStates.find(groupname);
        if(stateiter != mStates.end())
        {
            if(stateiter->second.mGroups != groups)
            {
                stateiter->second.mGroups = groups;
                resetActiveGroups();
            }
            return;
        }
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
                *complete = (iter->second.mTime - iter->second.mStartTime) /
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

        return iter->second.mTime;
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
            NifOsg::TextKeyMap::const_iterator textkey(textkeys.upper_bound(state.mTime));

            float timepassed = duration * state.mSpeedMult;
            while(state.mPlaying)
            {
                float targetTime;

                if(state.mTime >= state.mLoopStopTime && state.mLoopCount > 0)
                    goto handle_loop;

                targetTime = state.mTime + timepassed;
                if(textkey == textkeys.end() || textkey->first > targetTime)
                {
                    if(mAccumCtrl && stateiter->first == mAnimationTimePtr[0]->getAnimName())
                        updatePosition(state.mTime, targetTime, movement);
                    state.mTime = std::min(targetTime, state.mStopTime);
                }
                else
                {
                    if(mAccumCtrl && stateiter->first == mAnimationTimePtr[0]->getAnimName())
                        updatePosition(state.mTime, textkey->first, movement);
                    state.mTime = textkey->first;
                }

                state.mPlaying = (state.mTime < state.mStopTime);
                timepassed = targetTime - state.mTime;

                while(textkey != textkeys.end() && textkey->first <= state.mTime)
                {
                    handleTextKey(state, stateiter->first, textkey, textkeys);
                    ++textkey;
                }

                if(state.mTime >= state.mLoopStopTime && state.mLoopCount > 0)
                {
                handle_loop:
                    state.mLoopCount--;
                    state.mTime = state.mLoopStartTime;
                    state.mPlaying = true;

                    textkey = textkeys.lower_bound(state.mTime);
                    while(textkey != textkeys.end() && textkey->first <= state.mTime)
                    {
                        handleTextKey(state, stateiter->first, textkey, textkeys);
                        ++textkey;
                    }

                    if(state.mTime >= state.mLoopStopTime)
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

        return movement;
    }

    void Animation::setObjectRoot(const std::string &model, bool forceskeleton)
    {
        if (mObjectRoot)
        {
            mObjectRoot->getParent(0)->removeChild(mObjectRoot);
        }
        mObjectRoot = NULL;

        mNodeMap.clear();
        mAnimSourceControllers.clear();
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

        NodeMapVisitor visitor;
        mObjectRoot->accept(visitor);
        mNodeMap = visitor.getNodeMap();
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

        float realRadius = esmLight->mData.mRadius;

        lightSource->setRadius(realRadius);
        light->setLinearAttenuation(10.f/(esmLight->mData.mRadius*2.f));
        //light->setLinearAttenuation(0.05);
        light->setConstantAttenuation(0.f);

        light->setDiffuse(SceneUtil::colourFromRGB(esmLight->mData.mColor));
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
        // Early out if we already have this effect
        for (std::vector<EffectParams>::iterator it = mEffects.begin(); it != mEffects.end(); ++it)
            if (it->mLoop && loop && it->mEffectId == effectId && it->mBoneName == bonename)
                return;

        EffectParams params;
        params.mModelName = model;
        osg::ref_ptr<osg::Group> parentNode;
        if (bonename.empty())
            parentNode = mObjectRoot->asGroup();
        else
        {
            SceneUtil::FindByNameVisitor visitor(bonename);
            mObjectRoot->accept(visitor);
            if (!visitor.mFoundNode)
                throw std::runtime_error("Can't find bone " + bonename);
            parentNode = visitor.mFoundNode;
        }
        osg::ref_ptr<osg::Node> node = mResourceSystem->getSceneManager()->createInstance(model, parentNode);
        params.mObjects = PartHolderPtr(new PartHolder(node));

        SceneUtil::FindMaxControllerLengthVisitor findMaxLengthVisitor;
        node->accept(findMaxLengthVisitor);

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

    bool Animation::hasNode(const std::string &name)
    {
        std::string lowerName = Misc::StringUtils::lowerCase(name);
        return (mNodeMap.find(lowerName) != mNodeMap.end());
    }

    float Animation::AnimationTime::getValue(osg::NodeVisitor*)
    {
        // FIXME: hold a pointer instead of searching every frame
        AnimStateMap::const_iterator iter = mAnimation->mStates.find(mAnimationName);
        if(iter != mAnimation->mStates.end())
            return iter->second.mTime;
        return 0.0f;
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

    ObjectAnimation::ObjectAnimation(const MWWorld::Ptr &ptr, const std::string &model, Resource::ResourceSystem* resourceSystem, bool allowLight)
        : Animation(ptr, osg::ref_ptr<osg::Group>(ptr.getRefData().getBaseNode()), resourceSystem)
    {
        if (!model.empty())
        {
            setObjectRoot(model, false);

            if (!ptr.getClass().getEnchantment(ptr).empty())
                addGlow(mObjectRoot, getEnchantmentColor(ptr));

            if (ptr.getTypeName() == typeid(ESM::Light).name() && allowLight)
                addExtraLight(getOrCreateObjectRoot(), ptr.get<ESM::Light>()->mBase);
        }
    }

}
