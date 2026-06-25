#include "lightmanager.hpp"

#include <algorithm>
#include <cstring>

#include <osgUtil/CullVisitor>

#include <components/debug/debuglog.hpp>
#include <components/misc/constants.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/glextensions.hpp>
#include <components/sceneutil/util.hpp>
#include <components/shader/shadermanager.hpp>

#include "memorybarrier.hpp"

namespace
{
    void configurePosition(osg::Matrixf& mat, const osg::Vec4& pos)
    {
        mat(0, 0) = pos.x();
        mat(0, 1) = pos.y();
        mat(0, 2) = pos.z();
    }

    void configureAmbient(osg::Matrixf& mat, const osg::Vec4& color)
    {
        mat(1, 0) = color.r();
        mat(1, 1) = color.g();
        mat(1, 2) = color.b();
    }

    void configureDiffuse(osg::Matrixf& mat, const osg::Vec4& color)
    {
        mat(2, 0) = color.r();
        mat(2, 1) = color.g();
        mat(2, 2) = color.b();
    }

    void configureSpecular(osg::Matrixf& mat, const osg::Vec4& color)
    {
        mat(3, 0) = color.r();
        mat(3, 1) = color.g();
        mat(3, 2) = color.b();
        mat(3, 3) = color.a();
    }

    void configureAttenuation(osg::Matrixf& mat, float c, float l, float q, float r)
    {
        mat(0, 3) = c;
        mat(1, 3) = l;
        mat(2, 3) = q;
        mat(3, 3) = r;
    }
}

namespace SceneUtil
{
    static int sLightId = 0;

    void configureStateSetSunOverride(const osg::Light* light, osg::StateSet* stateset, int mode)
    {
        stateset->addUniform(new osg::Uniform("sun.position", light->getPosition()), mode);
        stateset->addUniform(new osg::Uniform("sun.diffuse", light->getDiffuse()), mode);
        stateset->addUniform(new osg::Uniform("sun.ambient", light->getAmbient()), mode);
        stateset->addUniform(new osg::Uniform("sun.specular", light->getSpecular()), mode);
    }

    void configureSunAmbientOverride(const osg::Vec4f& ambient, osg::StateSet* stateset)
    {
        stateset->getOrCreateUniform("sun.ambient", osg::Uniform::FLOAT_VEC4)->set(ambient);
    }

    LightManager* findLightManager(const osg::NodePath& path)
    {
        for (size_t i = 0; i < path.size(); ++i)
        {
            if (LightManager* lightManager = dynamic_cast<LightManager*>(path[i]))
                return lightManager;
        }
        return nullptr;
    }

    // Set on a LightSource. Adds the light source to its light manager for the current frame.
    // This allows us to keep track of the current lights in the scene graph without tying creation & destruction to the
    // manager.
    class CollectLightCallback : public NodeCallback<CollectLightCallback>
    {
    public:
        CollectLightCallback()
            : mLightManager(nullptr)
        {
        }

        CollectLightCallback(const CollectLightCallback& copy, const osg::CopyOp& copyop)
            : NodeCallback<CollectLightCallback>(copy, copyop)
            , mLightManager(nullptr)
        {
        }

        META_Object(SceneUtil, CollectLightCallback)

        void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            if (!mLightManager)
            {
                mLightManager = findLightManager(nv->getNodePath());

                if (!mLightManager)
                    throw std::runtime_error("can't find parent LightManager");
            }

            mLightManager->addLight(
                static_cast<LightSource*>(node), osg::computeLocalToWorld(nv->getNodePath()), nv->getTraversalNumber());

            traverse(node, nv);
        }

    private:
        LightManager* mLightManager;
    };

    // Set on a LightManager. Clears the data from the previous frame.
    class LightManagerUpdateCallback : public SceneUtil::NodeCallback<LightManagerUpdateCallback>
    {
    public:
        void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            LightManager* lightManager = static_cast<LightManager*>(node);
            lightManager->update(nv->getTraversalNumber());

            traverse(node, nv);
        }
    };

    LightManagerCullCallback::LightManagerCullCallback(const LightSettings& settings)
        : mGridSizeX(settings.mClusteredGridSize.x())
        , mGridSizeY(settings.mClusteredGridSize.y())
        , mGridSizeZ(settings.mClusteredGridSize.z())
        , mNumClusters(mGridSizeX * mGridSizeY * mGridSizeZ)
        , mWorkGroupSize(settings.mClusteredWorkGroupSize)
    {
    }

    void LightManagerCullCallback::operator()(LightManager* node, osgUtil::CullVisitor* cv)
    {
        if (!(cv->getTraversalMask() & node->getLightingMask()))
        {
            traverse(node, cv);
            return;
        }

        const size_t frame = cv->getTraversalNumber();
        const size_t frameId = frame % 2;

        auto& cache = mCache[cv->getCurrentCamera()];

        float clusterFar = node->getPointLightFadeEnd();

        // TODO: We will need to rethink this once distant lights are a thing.
        if (clusterFar == 0.f)
            clusterFar = Constants::CellSizeInUnits;

        bool rebuildCluster = false;

        if (node->getClusteredLighting())
        {
            auto& clusterNode = cache.mClusterComputeNode[frameId];
            auto& cullNode = cache.mCullComputeNode[frameId];

            if (!clusterNode || !cullNode)
            {
                if (!node->getResourceSystem())
                    throw std::runtime_error("Resource system must be defined");

                auto& shaderManager = node->getResourceSystem()->getSceneManager()->getShaderManager();

                clusterNode = new osg::DispatchCompute;
                clusterNode->setCullingActive(false);
                clusterNode->getOrCreateStateSet()->addUniform(
                    new osg::Uniform("inverseProjectionMatrix", osg::Matrixf{}));
                clusterNode->getOrCreateStateSet()->setAttribute(
                    shaderManager.getProgram(
                        nullptr, shaderManager.getShader("core/lighting/cluster.comp", {}, osg::Shader::COMPUTE)),
                    osg::StateAttribute::ON);
                clusterNode->setDrawCallback(new MemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT));

                cullNode = new osg::DispatchCompute;
                cullNode->setCullingActive(false);
                cullNode->getOrCreateStateSet()->setAttribute(
                    shaderManager.getProgram(nullptr,
                        shaderManager.getShader("core/lighting/cull.comp",
                            {
                                { "workGroupSize", std::to_string(mWorkGroupSize) },
                                { "maxLightsPerCluster", std::to_string(mMaxLightsPerCluster) },
                            },
                            osg::Shader::COMPUTE)),
                    osg::StateAttribute::ON);

                cullNode->setComputeGroups(mNumClusters, 1, 1);
                cullNode->setDrawCallback(new MemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT));
            }

            // Ensure we rebuild the cluster grid only when the projection matrix changes
            rebuildCluster = cache.clusterFar != clusterFar || cache.mProjection[frameId] != *cv->getProjectionMatrix();

            cache.clusterFar = clusterFar;
            cache.mProjection[frameId] = *cv->getProjectionMatrix();

            if (rebuildCluster)
            {
                // When reverse-z is enabled ensure we pass in the "unreversed" projection matrix.
                // This just makes all the maths constant and easier to deal with in general.
                // We also work with a projection matrix with a far plane detached from the current camera.
                double fovy, aspectRatio, near, _;
                cache.mProjection[frameId].getPerspective(fovy, aspectRatio, near, _);

                const osg::Matrixd projection = osg::Matrixd::perspective(fovy, aspectRatio, near, clusterFar);

                clusterNode->setComputeGroups(mGridSizeX, mGridSizeY, mGridSizeZ);
                clusterNode->getStateSet()
                    ->getUniform("inverseProjectionMatrix")
                    ->set(osg::Matrixf::inverse(projection));
            }
        }

        auto& stateset = cache.mStateSet[frameId];

        if (!stateset)
        {
            stateset = new osg::StateSet;
            stateset->addUniform(new osg::Uniform("sun.position", osg::Vec4f{}));
            stateset->addUniform(new osg::Uniform("sun.diffuse", osg::Vec4f{}));
            stateset->addUniform(new osg::Uniform("sun.ambient", osg::Vec4f{}));
            stateset->addUniform(new osg::Uniform("sun.specular", osg::Vec4f{}));

            if (node->getClusteredLighting())
            {
                stateset->addUniform(new osg::Uniform("clusterFar", clusterFar));
                stateset->addUniform(new osg::Uniform("gridSize",
                    osg::Vec3f(static_cast<float>(mGridSizeX), static_cast<float>(mGridSizeY),
                        static_cast<float>(mGridSizeZ))));

                const int maxLightIndices = mMaxLightsPerCluster * mNumClusters;

                osg::ref_ptr<osg::UByteArray> clusterData = new osg::UByteArray(sizeof(Cluster) * mNumClusters);
                clusterData->setBufferObject(new osg::ShaderStorageBufferObject);
                cache.mClusterSSBB
                    = new osg::ShaderStorageBufferBinding(1, clusterData, 0, clusterData->getTotalDataSize());

                for (size_t i = 0; i < cache.mPointLightSSBB.size(); ++i)
                {
                    cache.mGPULights[i] = new osg::BufferTemplate<std::vector<PointLight>>();
                    cache.mGPULights[i]->setBufferObject(new osg::ShaderStorageBufferObject);

                    cache.mPointLightSSBB[i] = new osg::ShaderStorageBufferBinding(
                        2, cache.mGPULights[i], 0, cache.mGPULights[i]->getTotalDataSize());

                    osg::ref_ptr<osg::UIntArray> gridData = new osg::UIntArray(mNumClusters * 2);
                    gridData->setBufferObject(new osg::ShaderStorageBufferObject);
                    cache.mLightGridSSBB[i]
                        = new osg::ShaderStorageBufferBinding(3, gridData, 0, gridData->getTotalDataSize());

                    osg::ref_ptr<osg::UIntArray> indexData = new osg::UIntArray(maxLightIndices);
                    indexData->setBufferObject(new osg::ShaderStorageBufferObject);
                    cache.mLightIndexListSSBB[i]
                        = new osg::ShaderStorageBufferBinding(4, indexData, 0, indexData->getTotalDataSize());

                    osg::ref_ptr<osg::UIntArray> counterData = new osg::UIntArray(1);
                    counterData->setBufferObject(new osg::ShaderStorageBufferObject);
                    cache.mLightIndexCounterSSBB[i]
                        = new osg::ShaderStorageBufferBinding(5, counterData, 0, counterData->getTotalDataSize());
                }
            }
        }

        if (frame != cache.mLastFrameNumber)
        {
            cache.mLastFrameNumber = frame;

            const auto& sun = node->getSunlight();

            // Don't use Camera::getViewMatrix, that one might be relative to another camera!
            const osg::RefMatrix* viewMatrix = cv->getCurrentRenderStage()->getInitialViewMatrix();

            stateset->getUniform("sun.position")->set(sun->getPosition() * (*viewMatrix));
            stateset->getUniform("sun.diffuse")->set(sun->getDiffuse());
            stateset->getUniform("sun.ambient")->set(sun->getAmbient());
            stateset->getUniform("sun.specular")->set(sun->getSpecular());

            if (node->getClusteredLighting())
            {
                if (rebuildCluster)
                {
                    stateset->getUniform("clusterFar")->set(clusterFar);
                    stateset->getUniform("gridSize")
                        ->set(osg::Vec3f(static_cast<float>(mGridSizeX), static_cast<float>(mGridSizeY),
                            static_cast<float>(mGridSizeZ)));
                }

                stateset->setAttribute(cache.mPointLightSSBB[frameId]);
                stateset->setAttribute(cache.mClusterSSBB);
                stateset->setAttribute(cache.mLightGridSSBB[frameId]);
                stateset->setAttribute(cache.mLightIndexListSSBB[frameId]);
                stateset->setAttribute(cache.mLightIndexCounterSSBB[frameId]);

                cache.mGPULights[frameId]->getData().clear();

                for (const auto& bound : node->getLightsInViewSpace(cv, viewMatrix, frame))
                {
                    if (bound.mCulled)
                        continue;

                    const auto& light = bound.mLightSource->getLight(frame);
                    auto gpuLight = PointLight{
                        .mPosition = light->getPosition() * (*viewMatrix),
                        .mDiffuse = light->getDiffuse(),
                        .mAmbient = light->getAmbient(),
                        .mSpecular = light->getSpecular(),
                        .mConstant = light->getConstantAttenuation(),
                        .mLinear = light->getLinearAttenuation(),
                        .mQuadratic = light->getQuadraticAttenuation(),
                        .mRadius = bound.mLightSource->getRadius() * node->getPointLightRadiusMultiplier(),
                    };

                    cache.mGPULights[frame % 2]->getData().push_back(gpuLight);
                }

                // Always add a dummy light, SSBO can't have zero size
                auto& lights = cache.mGPULights[frameId]->getData();
                if (lights.empty())
                    lights.emplace_back();

                cache.mPointLightSSBB[frameId]->setSize(cache.mGPULights[frameId]->getTotalDataSize());
                cache.mGPULights[frameId]->dirty();

                static_cast<osg::UIntArray*>(cache.mLightIndexCounterSSBB[frameId]->getBufferData())->at(0) = 0;
                static_cast<osg::UIntArray*>(cache.mLightIndexCounterSSBB[frameId]->getBufferData())->dirty();
            }
        }

        cv->pushStateSet(stateset);
        if (rebuildCluster)
            cache.mClusterComputeNode[frameId]->accept(*cv);
        if (node->getClusteredLighting())
            cache.mCullComputeNode[frameId]->accept(*cv);
        traverse(node, cv);
        cv->popStateSet();

        if (node->getPPLightsBuffer() && cv->getCurrentCamera()->getName() == Constants::SceneCamera)
            node->getPPLightsBuffer()->updateCount(frame);
    }

    LightManager::LightManager(const LightSettings& settings, Resource::ResourceSystem* resourceSystem)
        : mResourceSystem(resourceSystem)
        , mLightingMask(~0u)
        , mSun(nullptr)
        , mPointLightRadiusMultiplier(1.f)
        , mPointLightFadeEnd(0.f)
        , mPointLightFadeStart(0.f)
    {
        osg::GLExtensions* exts = SceneUtil::glExtensionsReady() ? &SceneUtil::getGLExtensions() : nullptr;
        // In theory the two extensions should allow us to use SSBO and std430 layout in version 120
        //  1. GL_ARB_shader_storage_buffer_object
        //  2. GL_ARB_shading_language_420pack
        // However, this is not the case in practice and is known to be broken on at least Mesa drivers.
        bool supportsSSBO = exts && static_cast<int>(exts->glslLanguageVersion * 100) >= 430;

        mSupportsClustered = supportsSSBO;

        setUpdateCallback(new LightManagerUpdateCallback);

        mCullCallback = new LightManagerCullCallback(settings);
        addCullCallback(mCullCallback);

        static bool hasLoggedWarnings = false;

        if (settings.mClusteredLighting && !hasLoggedWarnings)
        {
            if (!supportsSSBO)
                Log(Debug::Warning) << "GLSL 430 or higher not supported: disabling clustered lighting";
            hasLoggedWarnings = true;
        }

        if (settings.mClusteredLighting && mSupportsClustered)
            initClustered();
        else
            initPerObjectUniform(settings.mMaxLights);

        getOrCreateStateSet()->addUniform(new osg::Uniform("PointLightCount", 0));

        updateSettings(settings.mLightRadiusMultiplier, settings.mMaximumLightDistance, settings.mLightFadeStart);
    }

    LightManager::LightManager(const LightManager& copy, const osg::CopyOp& copyop)
        : osg::Group(copy, copyop)
        , mResourceSystem(copy.mResourceSystem)
        , mLightingMask(copy.mLightingMask)
        , mSun(copy.mSun)
        , mClusteredLighting(copy.mClusteredLighting)
        , mPointLightRadiusMultiplier(copy.mPointLightRadiusMultiplier)
        , mPointLightFadeEnd(copy.mPointLightFadeEnd)
        , mPointLightFadeStart(copy.mPointLightFadeStart)
        , mMaxLights(copy.mMaxLights)
        , mSupportsClustered(copy.mSupportsClustered)
        , mPPLightBuffer(copy.mPPLightBuffer)
    {
    }

    bool LightManager::getClusteredLighting() const
    {
        return mClusteredLighting;
    }

    int LightManager::getMaxLights() const
    {
        return mMaxLights;
    }

    void LightManager::setMaxLights(int value)
    {
        mMaxLights = value;
    }

    Shader::ShaderManager::DefineMap LightManager::getLightDefines() const
    {
        Shader::ShaderManager::DefineMap defines;

        defines["maxLights"] = std::to_string(getMaxLights());
        defines["lightingMethodClustered"] = mClusteredLighting ? "1" : "0";
        defines["useGPUShader4"] = std::to_string(mClusteredLighting);
        defines["simpleLighting"] = "0";

        return defines;
    }

    void LightManager::processChangedSettings(
        float lightRadiusMultiplier, float maximumLightDistance, float lightFadeStart)
    {
        updateSettings(lightRadiusMultiplier, maximumLightDistance, lightFadeStart);
    }

    void LightManager::updateMaxLights(int maxLights)
    {
        setMaxLights(maxLights);

        getStateSet()->removeUniform("LightBuffer");
        getStateSet()->addUniform(generateLightBufferUniform());
    }

    void LightManager::updateSettings(float lightRadiusMultiplier, float maximumLightDistance, float lightFadeStart)
    {
        mPointLightRadiusMultiplier = lightRadiusMultiplier;
        mPointLightFadeEnd = maximumLightDistance;
        if (mPointLightFadeEnd > 0)
            mPointLightFadeStart = mPointLightFadeEnd * lightFadeStart;
        mCullCallback->reset();
    }

    void LightManager::enableClustered(bool enabled)
    {
        if (!enabled)
        {
            initPerObjectUniform(mMaxLights);
        }
        else
        {
            initClustered();
        }
    }

    void LightManager::initPerObjectUniform(int targetLights)
    {
        mClusteredLighting = false;
        setMaxLights(targetLights);

        getOrCreateStateSet()->addUniform(generateLightBufferUniform());

        mCullCallback->reset();
    }

    void LightManager::initClustered()
    {
        mClusteredLighting = true;
        mCullCallback->reset();
    }

    void LightManager::setLightingMask(size_t mask)
    {
        mLightingMask = mask;
    }

    size_t LightManager::getLightingMask() const
    {
        return mLightingMask;
    }

    void LightManager::update(size_t frameNum)
    {
        if (mPPLightBuffer)
            mPPLightBuffer->clear(frameNum);

        mLights.clear();
        mLightsInViewSpace.clear();
    }

    void LightManager::addLight(LightSource* lightSource, const osg::Matrixf& worldMat, size_t frameNum)
    {
        LightSourceTransform l;
        l.mLightSource = lightSource;
        l.mWorldMatrix = worldMat;
        osg::Vec3f pos = worldMat.getTrans();
        lightSource->getLight(frameNum)->setPosition(osg::Vec4f(pos, 1.f));

        mLights.push_back(l);
    }

    void LightManager::setSunlight(osg::ref_ptr<osg::Light> sun)
    {
        mSun = sun;
    }

    osg::ref_ptr<osg::Light> LightManager::getSunlight()
    {
        return mSun;
    }

    osg::ref_ptr<osg::StateSet> LightManager::getLightListStateSet(
        const LightList& lightList, size_t frameNum, const osg::RefMatrix* viewMatrix)
    {
        osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet;
        osg::ref_ptr<osg::Uniform> data = generateLightBufferUniform();

        for (size_t i = 0; i < lightList.size(); ++i)
        {
            auto* light = lightList[i]->mLightSource->getLight(frameNum);
            osg::Matrixf lightMat;
            configurePosition(lightMat, light->getPosition() * (*viewMatrix));
            configureAmbient(lightMat, light->getAmbient());
            configureDiffuse(lightMat, light->getDiffuse());
            configureSpecular(lightMat, light->getSpecular());
            configureAttenuation(lightMat, light->getConstantAttenuation(), light->getLinearAttenuation(),
                light->getQuadraticAttenuation(),
                lightList[i]->mLightSource->getRadius() * mPointLightRadiusMultiplier);

            data->setElement(static_cast<unsigned int>(i), lightMat);
        }

        stateset->addUniform(data);
        stateset->addUniform(new osg::Uniform("PointLightCount", static_cast<int>(lightList.size())));

        return stateset;
    }

    const std::vector<LightManager::LightSourceViewBound>& LightManager::getLightsInViewSpace(
        osgUtil::CullVisitor* cv, const osg::RefMatrix* viewMatrix, size_t frameNum)
    {
        osg::Camera* camera = cv->getCurrentCamera();

        osg::observer_ptr<osg::Camera> camPtr(camera);
        auto it = mLightsInViewSpace.find(camPtr);

        if (it == mLightsInViewSpace.end())
        {
            it = mLightsInViewSpace.insert(std::make_pair(camPtr, LightSourceViewBoundCollection())).first;

            for (const auto& transform : mLights)
            {
                osg::Matrixf worldViewMat = transform.mWorldMatrix * (*viewMatrix);

                float radius = transform.mLightSource->getRadius() * mPointLightRadiusMultiplier;

                osg::BoundingSphere viewBound(osg::Vec3f(), radius);
                transformBoundingSphere(worldViewMat, viewBound);

                if (transform.mLightSource->getLastAppliedFrame() != frameNum && mPointLightFadeEnd != 0.f)
                {
                    const float fadeDelta = mPointLightFadeEnd - mPointLightFadeStart;
                    const float viewDelta = viewBound.center().length() - mPointLightFadeStart;
                    float fade = 1 - std::clamp(viewDelta / fadeDelta, 0.f, 1.f);
                    if (fade == 0.f)
                        continue;

                    auto* light = transform.mLightSource->getLight(frameNum);
                    light->setDiffuse(light->getDiffuse() * fade);
                    light->setSpecular(light->getSpecular() * fade);
                    transform.mLightSource->setLastAppliedFrame(frameNum);
                }

                LightSourceViewBound l;
                l.mLightSource = transform.mLightSource;
                l.mViewBound = viewBound;
                it->second.push_back(l);
            }

            const bool fillPPBuffer = mPPLightBuffer && it->first->getName() == Constants::SceneCamera;

            if (mClusteredLighting || fillPPBuffer)
            {
                auto sorter = [](const LightManager::LightSourceViewBound& left,
                                  const LightManager::LightSourceViewBound& right) {
                    return left.mViewBound.center().length2() - left.mViewBound.radius2()
                        < right.mViewBound.center().length2() - right.mViewBound.radius2();
                };

                std::sort(it->second.begin(), it->second.end(), sorter);

                osg::CullingSet& cullingSet = cv->getModelViewCullingStack().front();
                for (auto& bound : it->second)
                {
                    const auto* light = bound.mLightSource->getLight(frameNum);
                    const float radius = bound.mLightSource->getRadius() * mPointLightRadiusMultiplier;
                    osg::BoundingSphere frustumBound = bound.mViewBound;
                    frustumBound.radius() = radius * 2.f;

                    bound.mCulled = cullingSet.isCulled(frustumBound);

                    if (bound.mCulled || bound.mLightSource->getEmpty() || light->getDiffuse().x() < 0.f)
                        continue;

                    if (fillPPBuffer)
                        getPPLightsBuffer()->setLight(frameNum, light, radius);
                }
            }
        }

        return it->second;
    }

    osg::ref_ptr<osg::Uniform> LightManager::generateLightBufferUniform()
    {
        osg::ref_ptr<osg::Uniform> uniform = new osg::Uniform(osg::Uniform::FLOAT_MAT4, "LightBuffer", getMaxLights());

        return uniform;
    }

    void LightManager::setCollectPPLights(bool enabled)
    {
        if (enabled)
            mPPLightBuffer = std::make_shared<PPLightBuffer>();
        else
            mPPLightBuffer = nullptr;
    }

    LightSource::LightSource()
        : mRadius(0.f)
        , mActorFade(1.f)
        , mLastAppliedFrame(0)
    {
        setUpdateCallback(new CollectLightCallback);
        mId = sLightId++;
    }

    LightSource::LightSource(const LightSource& copy, const osg::CopyOp& copyop)
        : osg::Node(copy, copyop)
        , mRadius(copy.mRadius)
        , mActorFade(copy.mActorFade)
        , mLastAppliedFrame(copy.mLastAppliedFrame)
    {
        mId = sLightId++;

        for (size_t i = 0; i < mLight.size(); ++i)
            mLight[i] = new osg::Light(*copy.mLight[i].get(), copyop);
    }

    void LightListCallback::operator()(osg::Node* node, osgUtil::CullVisitor* cv)
    {
        bool pushedState = pushLightState(node, cv);
        traverse(node, cv);
        if (pushedState)
            cv->popStateSet();
    }

    bool LightListCallback::pushLightState(osg::Node* node, osgUtil::CullVisitor* cv)
    {
        if (!mLightManager)
        {
            mLightManager = findLightManager(cv->getNodePath());
            if (!mLightManager)
                return false;
        }

        // This cull callback SHOULD REALLY NOT EXIST when clustered shading is active!
        if (mLightManager->getClusteredLighting())
            return false;

        if (!(cv->getTraversalMask() & mLightManager->getLightingMask()))
            return false;

        // Possible optimizations:
        // - organize lights in a quad tree

        // Don't use Camera::getViewMatrix, that one might be relative to another camera!
        const osg::RefMatrix* viewMatrix = cv->getCurrentRenderStage()->getInitialViewMatrix();

        // Update light list if necessary
        // This makes sure we don't update it more than once per frame when rendering with multiple cameras
        if (mLastFrameNumber != cv->getTraversalNumber())
        {
            mLastFrameNumber = cv->getTraversalNumber();

            // Get the node bounds in view space
            // NB: do not node->getBound() * modelView, that would apply the node's transformation twice
            osg::BoundingSphere nodeBound;
            const osg::Transform* transform = node->asTransform();
            if (transform)
            {
                for (unsigned int i = 0; i < transform->getNumChildren(); ++i)
                    nodeBound.expandBy(transform->getChild(i)->getBound());
            }
            else
                nodeBound = node->getBound();

            transformBoundingSphere(*cv->getModelViewMatrix(), nodeBound);

            const std::vector<LightManager::LightSourceViewBound>& lights
                = mLightManager->getLightsInViewSpace(cv, viewMatrix, mLastFrameNumber);

            mLightList.clear();
            for (const LightManager::LightSourceViewBound& light : lights)
            {
                if (mIgnoredLightSources.contains(light.mLightSource))
                    continue;

                if (light.mViewBound.intersects(nodeBound))
                    mLightList.push_back(&light);
            }

            const size_t maxLights = mLightManager->getMaxLights();

            if (mLightList.size() > maxLights)
            {
                // Sort by proximity to object: prefer closer lights with larger radius
                std::sort(mLightList.begin(), mLightList.end(),
                    [&](const SceneUtil::LightManager::LightSourceViewBound* left,
                        const SceneUtil::LightManager::LightSourceViewBound* right) {
                        const float leftDist = (nodeBound.center() - left->mViewBound.center()).length2();
                        const float rightDist = (nodeBound.center() - right->mViewBound.center()).length2();
                        // A tricky way to compare normalized distance. This avoids division by near zero
                        return left->mViewBound.radius() * rightDist > right->mViewBound.radius() * leftDist;
                    });

                mLightList.resize(maxLights);
            }
        }

        if (!mLightList.empty())
        {
            cv->pushStateSet(mLightManager->getLightListStateSet(mLightList, mLastFrameNumber, viewMatrix));
            return true;
        }
        return false;
    }
}
