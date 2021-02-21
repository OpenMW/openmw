#include "lightmanager.hpp"

#include <osg/BufferObject>

#include <osgUtil/CullVisitor>

#include <components/sceneutil/util.hpp>

#include <components/misc/stringops.hpp>

#include <components/settings/settings.hpp>

#include <components/debug/debuglog.hpp>

namespace 
{
    /* similar to the boost::hash_combine */
    template <class T>
    inline void hash_combine(std::size_t& seed, const T& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }

    bool sortLights(const SceneUtil::LightManager::LightSourceViewBound* left, const SceneUtil::LightManager::LightSourceViewBound* right)
    {
        return left->mViewBound.center().length2() - left->mViewBound.radius2()*81 < right->mViewBound.center().length2() - right->mViewBound.radius2()*81;
    }
}

namespace SceneUtil
{
    static int sLightId = 0;

    class LightBuffer : public osg::Referenced
    {
    public:
        LightBuffer(int elements, int count=1)
            : mNumElements(elements)
            , mData(new osg::Vec4Array(elements * count))
            , mDirty(false)
        {}

        virtual ~LightBuffer() {}

        osg::ref_ptr<osg::Vec4Array> getData() 
        {
            return mData;
        }

        bool isDirty() const
        {
            return mDirty;
        }

        void dirty()
        {
            mData->dirty();
            mDirty = false;
        }

        int blockSize() const 
        {
            return mData->getNumElements() * sizeof(GL_FLOAT) * osg::Vec4::num_components;
        }

    protected:
        size_t mNumElements;
        osg::ref_ptr<osg::Vec4Array> mData;
        bool mDirty;
    };

    /*
     *  struct:
     *      vec4 diffuse
     *      vec4 ambient
     *      vec4 specular
     *      vec4 direction
     */
    class SunlightBuffer : public LightBuffer
    {
    public:

        enum Type {Diffuse, Ambient, Specular, Direction};

        SunlightBuffer()
            : LightBuffer(4)
        {}

        void setValue(Type type, const osg::Vec4& value)
        {
            if (getValue(type) == value)
                return;

            (*mData)[type] = value;

            mDirty = true;
        }   

        osg::Vec4 getValue(Type type)
        {
            return (*mData)[type];
        }
    };

    /*
     *  struct:
     *      vec4 diffuse
     *      vec4 ambient
     *      vec4 position
     *      vec4 illumination (constant, linear, quadratic)
     */
    class PointLightBuffer : public LightBuffer
    {
    public:

        enum Type {Diffuse, Ambient, Position, Attenuation};

        PointLightBuffer(int count)
            : LightBuffer(4, count)
        {}

        void setValue(int index, Type type, const osg::Vec4& value)
        {
            if (getValue(index, type) == value)
                return;

            (*mData)[mNumElements * index + type] = value;

            mDirty = true;
        }

        osg::Vec4 getValue(int index, Type type)
        {
            return (*mData)[mNumElements * index + type];
        }

        static constexpr int queryBlockSize(int sz)
        {
            return 4 * osg::Vec4::num_components * sizeof(GL_FLOAT) * sz;
        }
    };

    class LightStateCache
    {
    public:
        osg::Light* lastAppliedLight[8];
    };

    LightStateCache* getLightStateCache(size_t contextid)
    {
        static std::vector<LightStateCache> cacheVector;
        if (cacheVector.size() < contextid+1)
            cacheVector.resize(contextid+1);
        return &cacheVector[contextid];
    }

    SunlightStateAttribute::SunlightStateAttribute()
        : mBuffer(new SunlightBuffer)
    {
        osg::ref_ptr<osg::UniformBufferObject> ubo = new osg::UniformBufferObject;
        mBuffer->getData()->setBufferObject(ubo);
        mUbb = new osg::UniformBufferBinding(9 , mBuffer->getData().get(), 0, mBuffer->blockSize());
    }

    SunlightStateAttribute::SunlightStateAttribute(const SunlightStateAttribute &copy, const osg::CopyOp &copyop)
        : osg::StateAttribute(copy, copyop), mBuffer(copy.mBuffer) 
    {}

    int SunlightStateAttribute::compare(const StateAttribute &sa) const
    {
        throw std::runtime_error("LightStateAttribute::compare: unimplemented");
    }

    void SunlightStateAttribute::setFromLight(const osg::Light* light)
    {
        mBuffer->setValue(SunlightBuffer::Diffuse, light->getDiffuse());
        mBuffer->setValue(SunlightBuffer::Ambient, light->getAmbient());
        mBuffer->setValue(SunlightBuffer::Specular, light->getSpecular());
        mBuffer->setValue(SunlightBuffer::Direction, light->getPosition());
    }

    void SunlightStateAttribute::setStateSet(osg::StateSet* stateset, int mode)
    {
        stateset->setAttributeAndModes(mUbb, mode);
        stateset->setAssociatedModes(this, mode);
        mBuffer->dirty();
    }

    class DisableLight : public osg::StateAttribute
    {
    public:
        DisableLight() : mIndex(0) {}
        DisableLight(int index) : mIndex(index) {}

        DisableLight(const DisableLight& copy,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY)
            : osg::StateAttribute(copy,copyop), mIndex(copy.mIndex) {}

        osg::Object* cloneType() const override { return new DisableLight(mIndex); }
        osg::Object* clone(const osg::CopyOp& copyop) const override { return new DisableLight(*this,copyop); }
        bool isSameKindAs(const osg::Object* obj) const override { return dynamic_cast<const DisableLight *>(obj)!=nullptr; }
        const char* libraryName() const override { return "SceneUtil"; }
        const char* className() const override { return "DisableLight"; }
        Type getType() const override { return LIGHT; }

        unsigned int getMember() const override
        {
            return mIndex;
        }

        bool getModeUsage(ModeUsage & usage) const override
        {
            usage.usesMode(GL_LIGHT0 + mIndex);
            return true;
        }

        int compare(const StateAttribute &sa) const override
        {
            throw std::runtime_error("DisableLight::compare: unimplemented");
        }

        void apply(osg::State& state) const override
        {
            int lightNum = GL_LIGHT0 + mIndex;
            glLightfv( lightNum, GL_AMBIENT,               mnullptr.ptr() );
            glLightfv( lightNum, GL_DIFFUSE,               mnullptr.ptr() );
            glLightfv( lightNum, GL_SPECULAR,              mnullptr.ptr() );

            LightStateCache* cache = getLightStateCache(state.getContextID());
            cache->lastAppliedLight[mIndex] = nullptr;
        }

    private:
        size_t mIndex;
        osg::Vec4f mnullptr;
    };

    class LightStateAttribute : public osg::StateAttribute
    {
    public:
        LightStateAttribute() : mBuffer(nullptr) {}
        LightStateAttribute(const std::vector<osg::ref_ptr<osg::Light> >& lights, const osg::ref_ptr<PointLightBuffer>& buffer) : mLights(lights), mBuffer(buffer) {} 

        LightStateAttribute(const LightStateAttribute& copy,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY)
            : osg::StateAttribute(copy,copyop), mLights(copy.mLights), mBuffer(copy.mBuffer) {}

        int compare(const StateAttribute &sa) const override
        {
            throw std::runtime_error("LightStateAttribute::compare: unimplemented");
        }

        META_StateAttribute(NifOsg, LightStateAttribute, osg::StateAttribute::LIGHT)

        void apply(osg::State& state) const override
        {
            if (mLights.empty())
                return;
            osg::Matrix modelViewMatrix = state.getModelViewMatrix();

            state.applyModelViewMatrix(state.getInitialViewMatrix());

            if (mBuffer)
            {
                for (size_t i = 0; i < mLights.size(); ++i)
                {

                    mBuffer->setValue(i, PointLightBuffer::Diffuse, mLights[i]->getDiffuse());
                    mBuffer->setValue(i, PointLightBuffer::Ambient, mLights[i]->getAmbient());
                    mBuffer->setValue(i, PointLightBuffer::Position, mLights[i]->getPosition() * state.getModelViewMatrix());
                    mBuffer->setValue(i, PointLightBuffer::Attenuation,
                                        osg::Vec4(mLights[i]->getConstantAttenuation(), 
                                                  mLights[i]->getLinearAttenuation(), 
                                                  mLights[i]->getQuadraticAttenuation(), 0.0));
                }

                if (mBuffer && mBuffer->isDirty())
                    mBuffer->dirty();
            }

            state.applyModelViewMatrix(modelViewMatrix);
        }

    private:
        std::vector<osg::ref_ptr<osg::Light>> mLights;
        osg::ref_ptr<PointLightBuffer> mBuffer;
    };

    class FFPLightStateAttribute : public osg::StateAttribute
    {
    public:
        FFPLightStateAttribute() : mIndex(0) {}
        FFPLightStateAttribute(size_t index, const std::vector<osg::ref_ptr<osg::Light> >& lights) : mIndex(index), mLights(lights) {}

        FFPLightStateAttribute(const FFPLightStateAttribute& copy,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY)
            : osg::StateAttribute(copy,copyop), mIndex(copy.mIndex), mLights(copy.mLights) {}

        unsigned int getMember() const override
        {
            return mIndex;
        }

        bool getModeUsage(ModeUsage & usage) const override
        {
            for (size_t i=0; i<mLights.size(); ++i)
                usage.usesMode(GL_LIGHT0 + mIndex + i);
            return true;
        }

        int compare(const StateAttribute &sa) const override
        {
            throw std::runtime_error("FFPLightStateAttribute::compare: unimplemented");
        }

        META_StateAttribute(NifOsg, FFPLightStateAttribute, osg::StateAttribute::LIGHT)

        void apply(osg::State& state) const override
        {
            if (mLights.empty())
                return;
            osg::Matrix modelViewMatrix = state.getModelViewMatrix();

            state.applyModelViewMatrix(state.getInitialViewMatrix());

            LightStateCache* cache = getLightStateCache(state.getContextID());

            for (size_t i=0; i<mLights.size(); ++i)
            {
                osg::Light* current = cache->lastAppliedLight[i+mIndex];
                if (current != mLights[i].get())
                {
                    applyLight((GLenum)((int)GL_LIGHT0 + i + mIndex), mLights[i].get());
                    cache->lastAppliedLight[i+mIndex] = mLights[i].get();
                }
            }

            state.applyModelViewMatrix(modelViewMatrix);
        }

        void applyLight(GLenum lightNum, const osg::Light* light) const
        {
            glLightfv( lightNum, GL_AMBIENT,               light->getAmbient().ptr() );
            glLightfv( lightNum, GL_DIFFUSE,               light->getDiffuse().ptr() );
            glLightfv( lightNum, GL_SPECULAR,              light->getSpecular().ptr() );
            glLightfv( lightNum, GL_POSITION,              light->getPosition().ptr() );
            // TODO: enable this once spot lights are supported
            // need to transform SPOT_DIRECTION by the world matrix?
            //glLightfv( lightNum, GL_SPOT_DIRECTION,        light->getDirection().ptr() );
            //glLightf ( lightNum, GL_SPOT_EXPONENT,         light->getSpotExponent() );
            //glLightf ( lightNum, GL_SPOT_CUTOFF,           light->getSpotCutoff() );
            glLightf ( lightNum, GL_CONSTANT_ATTENUATION,  light->getConstantAttenuation() );
            glLightf ( lightNum, GL_LINEAR_ATTENUATION,    light->getLinearAttenuation() );
            glLightf ( lightNum, GL_QUADRATIC_ATTENUATION, light->getQuadraticAttenuation() );
        }

    private:
        size_t mIndex;

        std::vector<osg::ref_ptr<osg::Light>> mLights;
    };

    LightManager* findLightManager(const osg::NodePath& path)
    {
        for (size_t i=0;i<path.size(); ++i)
        {
            if (LightManager* lightManager = dynamic_cast<LightManager*>(path[i]))
                return lightManager;
        }
        return nullptr;
    }

    // Set on a LightSource. Adds the light source to its light manager for the current frame.
    // This allows us to keep track of the current lights in the scene graph without tying creation & destruction to the manager.
    class CollectLightCallback : public osg::NodeCallback
    {
    public:
        CollectLightCallback()
            : mLightManager(nullptr) { }

        CollectLightCallback(const CollectLightCallback& copy, const osg::CopyOp& copyop)
            : osg::NodeCallback(copy, copyop)
            , mLightManager(nullptr) { }

        META_Object(SceneUtil, SceneUtil::CollectLightCallback)

        void operator()(osg::Node* node, osg::NodeVisitor* nv) override
        {
            if (!mLightManager)
            {
                mLightManager = findLightManager(nv->getNodePath());

                if (!mLightManager)
                    throw std::runtime_error("can't find parent LightManager");
            }

            mLightManager->addLight(static_cast<LightSource*>(node), osg::computeLocalToWorld(nv->getNodePath()), nv->getTraversalNumber());

            traverse(node, nv);
        }

    private:
        LightManager* mLightManager;
    };

    // Set on a LightManager. Clears the data from the previous frame.
    class LightManagerUpdateCallback : public osg::NodeCallback
    {
    public:
        LightManagerUpdateCallback()
            { }

        LightManagerUpdateCallback(const LightManagerUpdateCallback& copy, const osg::CopyOp& copyop)
            : osg::NodeCallback(copy, copyop)
            { }

        META_Object(SceneUtil, LightManagerUpdateCallback)

        void operator()(osg::Node* node, osg::NodeVisitor* nv) override
        {
            LightManager* lightManager = static_cast<LightManager*>(node);
            lightManager->update();

            traverse(node, nv);
        }
    };

    class SunlightCallback : public osg::NodeCallback
    {
    public:
        SunlightCallback(LightManager* lightManager) : mLightManager(lightManager) {}

        void operator()(osg::Node* node, osg::NodeVisitor* nv) override
        {
            osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);

            if (mLastFrameNumber != cv->getTraversalNumber())
            {
                mLastFrameNumber = cv->getTraversalNumber();

                auto sun = mLightManager->getSunlight();

                if (!sun)
                    return;

                auto buf = mLightManager->getSunBuffer();

                buf->setValue(SunlightBuffer::Diffuse, sun->getDiffuse());
                buf->setValue(SunlightBuffer::Ambient, sun->getAmbient());
                buf->setValue(SunlightBuffer::Specular, sun->getSpecular());
                buf->setValue(SunlightBuffer::Direction, sun->getPosition() * *cv->getCurrentRenderStage()->getInitialViewMatrix());                           

                if (buf->isDirty())
                    buf->dirty();
            }

            traverse(node, nv);
        }

    private:
        LightManager* mLightManager;
        size_t mLastFrameNumber;
    };

    LightManager::LightManager(bool ffp)
        : mStartLight(0)
        , mLightingMask(~0u)
        , mSun(nullptr)
        , mSunBuffer(nullptr)
        , mFFP(ffp)
    {
        setUpdateCallback(new LightManagerUpdateCallback);

        if (usingFFP())
        {
            for (int i=0; i<getMaxLights(); ++i)
                mDummies.push_back(new FFPLightStateAttribute(i, std::vector<osg::ref_ptr<osg::Light> >()));
            return;
        }

        auto* stateset = getOrCreateStateSet();

        mSunBuffer = new SunlightBuffer();
        osg::ref_ptr<osg::UniformBufferObject> ubo = new osg::UniformBufferObject;
        mSunBuffer->getData()->setBufferObject(ubo);
        osg::ref_ptr<osg::UniformBufferBinding> ubb = new osg::UniformBufferBinding(9 , mSunBuffer->getData().get(), 0, mSunBuffer->blockSize());
        stateset->setAttributeAndModes(ubb.get(), osg::StateAttribute::ON);

        stateset->addUniform(new osg::Uniform("PointLightCount", 0));

        addCullCallback(new SunlightCallback(this));
    }

    LightManager::LightManager(const LightManager &copy, const osg::CopyOp &copyop)
        : osg::Group(copy, copyop)
        , mStartLight(copy.mStartLight)
        , mLightingMask(copy.mLightingMask)
        , mSun(copy.mSun)
        , mSunBuffer(copy.mSunBuffer)
        , mFFP(copy.mFFP)
    {
    }

    bool LightManager::usingFFP() const
    {
        return mFFP;
    }

    int LightManager::getMaxLights() const
    {
        if (usingFFP()) return LightManager::mFFPMaxLights;
        return std::clamp(Settings::Manager::getInt("max lights", "Shaders"), 0, getMaxLightsInScene());
    }

    int LightManager::getMaxLightsInScene() const
    {
        static constexpr int max = 16384 / PointLightBuffer::queryBlockSize(1);        
        return  max;          
    }

    Shader::ShaderManager::DefineMap LightManager::getLightDefines() const
    {
        Shader::ShaderManager::DefineMap defines;

        bool ffp = usingFFP();
        
        defines["ffpLighting"] = ffp ? "1" : "0";
        defines["sunDirection"] = ffp ? "gl_LightSource[0].position" : "Sun.direction";
        defines["sunAmbient"] = ffp ? "gl_LightSource[0].ambient" : "Sun.ambient";
        defines["sunDiffuse"] = ffp ? "gl_LightSource[0].diffuse" : "Sun.diffuse";
        defines["sunSpecular"] = ffp ? "gl_LightSource[0].specular" : "Sun.specular";
        defines["maxLights"] = std::to_string(getMaxLights());
        defines["maxLightsInScene"] = std::to_string(getMaxLightsInScene());

        return defines;
    }

    bool LightManager::queryNonFFPLightingSupport()
    {
        osg::GLExtensions* exts = osg::GLExtensions::Get(0, false);
        if (!exts || !exts->isUniformBufferObjectSupported)
        {
            auto ffpWarning = Misc::StringUtils::format("GL_ARB_uniform_buffer_object not supported:  Falling back to FFP %zu light limit. Can not set lights to %i."
                                                    , LightManager::mFFPMaxLights
                                                    , Settings::Manager::getInt("max lights", "Shaders"));
            Log(Debug::Warning) << ffpWarning;
            return false;
        }
        return true;
    }

    void LightManager::setLightingMask(size_t mask)
    {
        mLightingMask = mask;
    }

    size_t LightManager::getLightingMask() const
    {
        return mLightingMask;
    }

    void LightManager::setStartLight(int start)
    {
        if (!usingFFP()) return;

        mStartLight = start;

        // Set default light state to zero
        // This is necessary because shaders don't respect glDisable(GL_LIGHTX) so in addition to disabling
        // we'll have to set a light state that has no visible effect
        for (int i=start; i<getMaxLights(); ++i)
        {
            osg::ref_ptr<DisableLight> defaultLight (new DisableLight(i));
            getOrCreateStateSet()->setAttributeAndModes(defaultLight, osg::StateAttribute::OFF);
        }
    }

    int LightManager::getStartLight() const
    {
        return mStartLight;
    }

    void LightManager::update()
    {
        mLights.clear();
        mLightsInViewSpace.clear();

        // do an occasional cleanup for orphaned lights
        for (int i=0; i<2; ++i)
        {
            if (mStateSetCache[i].size() > 5000)
                mStateSetCache[i].clear();
        }
    }

    void LightManager::addLight(LightSource* lightSource, const osg::Matrixf& worldMat, size_t frameNum)
    {
        LightSourceTransform l;
        l.mLightSource = lightSource;
        l.mWorldMatrix = worldMat;
        lightSource->getLight(frameNum)->setPosition(osg::Vec4f(worldMat.getTrans().x(),
                                                        worldMat.getTrans().y(),
                                                        worldMat.getTrans().z(), 1.f));
        mLights.push_back(l);
    }

    void LightManager::setSunlight(osg::ref_ptr<osg::Light> sun)
    {
        if (usingFFP()) return;

        mSun = sun;
    }

    osg::ref_ptr<osg::Light> LightManager::getSunlight()
    {
        return mSun;
    }

    osg::ref_ptr<SunlightBuffer> LightManager::getSunBuffer()
    {
        return mSunBuffer;
    }

    osg::ref_ptr<osg::StateSet> LightManager::getLightListStateSet(const LightList &lightList, size_t frameNum)
    {
        // possible optimization: return a StateSet containing all requested lights plus some extra lights (if a suitable one exists)
        size_t hash = 0;
        for (size_t i=0; i<lightList.size();++i)
            hash_combine(hash, lightList[i]->mLightSource->getId());

        LightStateSetMap& stateSetCache = mStateSetCache[frameNum%2];

        LightStateSetMap::iterator found = stateSetCache.find(hash);
        if (found != stateSetCache.end())
            return found->second;
        else
        {
            osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet;
            std::vector<osg::ref_ptr<osg::Light> > lights;
            lights.reserve(lightList.size());
            for (size_t i=0; i<lightList.size();++i)
                lights.emplace_back(lightList[i]->mLightSource->getLight(frameNum));

            if (usingFFP())
            {
                // the first light state attribute handles the actual state setting for all lights
                // it's best to batch these up so that we don't need to touch the modelView matrix more than necessary
                // don't use setAttributeAndModes, that does not support light indices!
                stateset->setAttribute(new FFPLightStateAttribute(mStartLight, std::move(lights)), osg::StateAttribute::ON);

                for (size_t i=0; i<lightList.size(); ++i)
                    stateset->setMode(GL_LIGHT0 + mStartLight + i, osg::StateAttribute::ON);

                // need to push some dummy attributes to ensure proper state tracking
                // lights need to reset to their default when the StateSet is popped
                for (size_t i=1; i<lightList.size(); ++i)
                    stateset->setAttribute(mDummies[i+mStartLight].get(), osg::StateAttribute::ON);
            }
            else
            {
                osg::ref_ptr<PointLightBuffer> buffer = new PointLightBuffer(lights.size());
                osg::ref_ptr<osg::UniformBufferObject> ubo = new osg::UniformBufferObject;
                buffer->getData()->setBufferObject(ubo);
                osg::ref_ptr<osg::UniformBufferBinding> ubb = new osg::UniformBufferBinding(8 ,buffer->getData().get(), 0, buffer->blockSize());
                stateset->addUniform(new osg::Uniform("PointLightCount", (int)lights.size()));
                stateset->setAttributeAndModes(ubb.get(), osg::StateAttribute::ON);

                stateset->setAttribute(new LightStateAttribute(std::move(lights), buffer), osg::StateAttribute::ON);
            }

            stateSetCache.emplace(hash, stateset);
            return stateset;
        }
    }

    const std::vector<LightManager::LightSourceTransform>& LightManager::getLights() const
    {
        return mLights;
    }

    const std::vector<LightManager::LightSourceViewBound>& LightManager::getLightsInViewSpace(osg::Camera *camera, const osg::RefMatrix* viewMatrix, size_t frameNum)
    {
        osg::observer_ptr<osg::Camera> camPtr (camera);
        std::map<osg::observer_ptr<osg::Camera>, LightSourceViewBoundCollection>::iterator it = mLightsInViewSpace.find(camPtr);

        if (it == mLightsInViewSpace.end())
        {
            it = mLightsInViewSpace.insert(std::make_pair(camPtr, LightSourceViewBoundCollection())).first;

            for (std::vector<LightSourceTransform>::iterator lightIt = mLights.begin(); lightIt != mLights.end(); ++lightIt)
            {
                osg::Matrixf worldViewMat = lightIt->mWorldMatrix * (*viewMatrix);
                osg::BoundingSphere viewBound = osg::BoundingSphere(osg::Vec3f(0,0,0), lightIt->mLightSource->getRadius());
                transformBoundingSphere(worldViewMat, viewBound);

                LightSourceViewBound l;
                l.mLightSource = lightIt->mLightSource;
                l.mViewBound = viewBound;
                it->second.push_back(l);
            }
        }
        return it->second;
    }

    LightSource::LightSource()
        : mRadius(0.f)
    {
        setUpdateCallback(new CollectLightCallback);
        mId = sLightId++;
    }

    LightSource::LightSource(const LightSource &copy, const osg::CopyOp &copyop)
        : osg::Node(copy, copyop)
        , mRadius(copy.mRadius)
    {
        mId = sLightId++;

        for (int i=0; i<2; ++i)
            mLight[i] = new osg::Light(*copy.mLight[i].get(), copyop);
    }

    void LightListCallback::operator()(osg::Node *node, osg::NodeVisitor *nv)
    {
        osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);

        bool pushedState = pushLightState(node, cv);
        traverse(node, nv);
        if (pushedState)
            cv->popStateSet();
    }

    bool LightListCallback::pushLightState(osg::Node *node, osgUtil::CullVisitor *cv)
    {
        if (!mLightManager)
        {
            mLightManager = findLightManager(cv->getNodePath());
            if (!mLightManager)
                return false;
        }

        if (!(cv->getTraversalMask() & mLightManager->getLightingMask()))
            return false;

        // Possible optimizations:
        // - cull list of lights by the camera frustum
        // - organize lights in a quad tree


        // update light list if necessary
        // makes sure we don't update it more than once per frame when rendering with multiple cameras
        if (mLastFrameNumber != cv->getTraversalNumber())
        {
            mLastFrameNumber = cv->getTraversalNumber();

            // Don't use Camera::getViewMatrix, that one might be relative to another camera!
            const osg::RefMatrix* viewMatrix = cv->getCurrentRenderStage()->getInitialViewMatrix();
            const std::vector<LightManager::LightSourceViewBound>& lights = mLightManager->getLightsInViewSpace(cv->getCurrentCamera(), viewMatrix, mLastFrameNumber);

            // get the node bounds in view space
            // NB do not node->getBound() * modelView, that would apply the node's transformation twice
            osg::BoundingSphere nodeBound;
            osg::Transform* transform = node->asTransform();
            if (transform)
            {
                for (size_t i=0; i<transform->getNumChildren(); ++i)
                    nodeBound.expandBy(transform->getChild(i)->getBound());
            }
            else
                nodeBound = node->getBound();
            osg::Matrixf mat = *cv->getModelViewMatrix();
            transformBoundingSphere(mat, nodeBound);

            mLightList.clear();
            for (size_t i=0; i<lights.size(); ++i)
            {
                const LightManager::LightSourceViewBound& l = lights[i];

                if (mIgnoredLightSources.count(l.mLightSource))
                    continue;

                if (l.mViewBound.intersects(nodeBound))
                    mLightList.push_back(&l);
            }
        }
        if (!mLightList.empty())
        {
            size_t maxLights = mLightManager->getMaxLights() - mLightManager->getStartLight();

            osg::StateSet* stateset = nullptr;

            if (mLightList.size() > maxLights)
            {
                // remove lights culled by this camera
                LightManager::LightList lightList = mLightList;
                for (LightManager::LightList::iterator it = lightList.begin(); it != lightList.end() && lightList.size() > maxLights; )
                {
                    osg::CullStack::CullingStack& stack = cv->getModelViewCullingStack();

                    osg::BoundingSphere bs = (*it)->mViewBound;
                    bs._radius = bs._radius*2;
                    osg::CullingSet& cullingSet = stack.front();
                    if (cullingSet.isCulled(bs))
                    {
                        it = lightList.erase(it);
                        continue;
                    }
                    else
                        ++it;
                }

                if (lightList.size() > maxLights)
                {
                    // sort by proximity to camera, then get rid of furthest away lights
                    std::sort(lightList.begin(), lightList.end(), sortLights);
                    while (lightList.size() > maxLights)
                        lightList.pop_back();
                }
                stateset = mLightManager->getLightListStateSet(lightList, cv->getTraversalNumber());
            }
            else
                stateset = mLightManager->getLightListStateSet(mLightList, cv->getTraversalNumber());


            cv->pushStateSet(stateset);
            return true;
        }
        return false;
    }

}
