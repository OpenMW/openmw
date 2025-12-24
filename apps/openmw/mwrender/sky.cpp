#include "sky.hpp"

#include <osg/Depth>
#include <osg/PositionAttitudeTransform>

#include <osgParticle/BoxPlacer>
#include <osgParticle/ModularEmitter>
#include <osgParticle/ModularProgram>
#include <osgParticle/Operator>
#include <osgParticle/ParticleSystemUpdater>

#include <components/settings/values.hpp>

#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/sceneutil/rtt.hpp>
#include <components/sceneutil/shadow.hpp>
#include <components/sceneutil/visitor.hpp>

#include <components/resource/imagemanager.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/vfs/manager.hpp>

#include <components/misc/resourcehelpers.hpp>
#include <components/stereo/stereomanager.hpp>

#include <components/nifosg/particle.hpp>

#include "../mwworld/datetimemanager.hpp"
#include "../mwworld/weather.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "renderbin.hpp"
#include "skyutil.hpp"
#include "util.hpp"
#include "vismask.hpp"

namespace
{
    class WrapAroundOperator : public osgParticle::Operator
    {
    public:
        WrapAroundOperator(osg::Camera* camera, const osg::Vec3& wrapRange)
            : osgParticle::Operator()
            , mCamera(camera)
            , mWrapRange(wrapRange)
            , mHalfWrapRange(mWrapRange / 2.0)
        {
            mPreviousCameraPosition = getCameraPosition();
        }

        osg::Object* cloneType() const override { return nullptr; }

        osg::Object* clone(const osg::CopyOp& op) const override { return nullptr; }

        void operate(osgParticle::Particle* particle, double dt) override {}

        void operateParticles(osgParticle::ParticleSystem* ps, double dt) override
        {
            osg::Vec3 position = getCameraPosition();
            osg::Vec3 positionDifference = position - mPreviousCameraPosition;

            osg::Matrix toWorld, toLocal;

            std::vector<osg::Matrix> worldMatrices = ps->getWorldMatrices();

            if (!worldMatrices.empty())
            {
                toWorld = worldMatrices[0];
                toLocal.invert(toWorld);
            }

            for (int i = 0; i < ps->numParticles(); ++i)
            {
                osgParticle::Particle* p = ps->getParticle(i);
                p->setPosition(toWorld.preMult(p->getPosition()));
                p->setPosition(p->getPosition() - positionDifference);

                for (int j = 0; j < 3; ++j) // wrap-around in all 3 dimensions
                {
                    osg::Vec3 pos = p->getPosition();

                    if (pos[j] < -mHalfWrapRange[j])
                        pos[j] = mHalfWrapRange[j] + fmod(pos[j] - mHalfWrapRange[j], mWrapRange[j]);
                    else if (pos[j] > mHalfWrapRange[j])
                        pos[j] = fmod(pos[j] + mHalfWrapRange[j], mWrapRange[j]) - mHalfWrapRange[j];

                    p->setPosition(pos);
                }

                p->setPosition(toLocal.preMult(p->getPosition()));
            }

            mPreviousCameraPosition = position;
        }

    protected:
        osg::Camera* mCamera;
        osg::Vec3 mPreviousCameraPosition;
        osg::Vec3 mWrapRange;
        osg::Vec3 mHalfWrapRange;

        osg::Vec3 getCameraPosition() { return mCamera->getInverseViewMatrix().getTrans(); }
    };

    class WeatherAlphaOperator : public osgParticle::Operator
    {
    public:
        WeatherAlphaOperator(float& alpha, bool rain)
            : mAlpha(alpha)
            , mIsRain(rain)
        {
        }

        osg::Object* cloneType() const override { return nullptr; }

        osg::Object* clone(const osg::CopyOp& op) const override { return nullptr; }

        void operate(osgParticle::Particle* particle, double dt) override
        {
            constexpr float rainThreshold = 0.6f; // Rain_Threshold?
            float alpha = mIsRain ? mAlpha * rainThreshold : mAlpha;
            particle->setAlphaRange(osgParticle::rangef(alpha, alpha));
        }

    private:
        float& mAlpha;
        bool mIsRain;
    };

    // Updater for alpha value on a node's StateSet. Assumes the node has an existing Material StateAttribute.
    class AlphaFader : public SceneUtil::StateSetUpdater
    {
    public:
        /// @param alpha the variable alpha value is recovered from
        AlphaFader(const float& alpha)
            : mAlpha(alpha)
        {
        }

        void setDefaults(osg::StateSet* stateset) override
        {
            // need to create a deep copy of StateAttributes we will modify
            osg::Material* mat = static_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
            stateset->setAttribute(osg::clone(mat, osg::CopyOp::DEEP_COPY_ALL), osg::StateAttribute::ON);
        }

        void apply(osg::StateSet* stateset, osg::NodeVisitor* nv) override
        {
            osg::Material* mat = static_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
            mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, mAlpha));
        }

    protected:
        const float& mAlpha;
    };

    // Helper for adding AlphaFaders to a subgraph
    class SetupVisitor : public osg::NodeVisitor
    {
    public:
        SetupVisitor(const float& alpha)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
            , mAlpha(alpha)
        {
        }

        void apply(osg::Node& node) override
        {
            if (osg::StateSet* stateset = node.getStateSet())
            {
                if (stateset->getAttribute(osg::StateAttribute::MATERIAL))
                {
                    SceneUtil::CompositeStateSetUpdater* composite = nullptr;
                    osg::Callback* callback = node.getUpdateCallback();

                    while (callback)
                    {
                        composite = dynamic_cast<SceneUtil::CompositeStateSetUpdater*>(callback);
                        if (composite)
                            break;

                        callback = callback->getNestedCallback();
                    }

                    osg::ref_ptr<AlphaFader> alphaFader = new AlphaFader(mAlpha);

                    if (composite)
                        composite->addController(alphaFader);
                    else
                        node.addUpdateCallback(alphaFader);
                }
            }

            traverse(node);
        }

    private:
        const float& mAlpha;
    };

    class SkyRTT : public SceneUtil::RTTNode
    {
    public:
        SkyRTT(osg::Vec2f size, osg::Group* earlyRenderBinRoot)
            : RTTNode(static_cast<int>(size.x()), static_cast<int>(size.y()), 0, false, 1, StereoAwareness::Aware,
                MWRender::shouldAddMSAAIntermediateTarget())
            , mEarlyRenderBinRoot(earlyRenderBinRoot)
        {
            setDepthBufferInternalFormat(GL_DEPTH24_STENCIL8);
        }

        void setDefaults(osg::Camera* camera) override
        {
            camera->setReferenceFrame(osg::Camera::RELATIVE_RF);
            camera->setName("SkyCamera");
            camera->setNodeMask(MWRender::Mask_RenderToTexture);
            camera->setCullMask(MWRender::Mask_Sky);
            camera->addChild(mEarlyRenderBinRoot);
            SceneUtil::ShadowManager::instance().disableShadowsForStateSet(*camera->getOrCreateStateSet());
        }

    private:
        osg::ref_ptr<osg::Group> mEarlyRenderBinRoot;
    };

}

namespace MWRender
{
    SkyManager::SkyManager(osg::Group* parentNode, osg::Group* rootNode, osg::Camera* camera,
        Resource::SceneManager* sceneManager, bool enableSkyRTT)
        : mSceneManager(sceneManager)
        , mCamera(camera)
        , mAtmosphereNightRoll(0.f)
        , mCreated(false)
        , mIsStorm(false)
        , mTimescaleClouds(Fallback::Map::getBool("Weather_Timescale_Clouds"))
        , mCloudAnimationTimer(0.f)
        , mStormParticleDirection(MWWorld::Weather::defaultDirection())
        , mStormDirection(MWWorld::Weather::defaultDirection())
        , mClouds()
        , mNextClouds()
        , mCloudBlendFactor(0.f)
        , mCloudSpeed(0.f)
        , mStarsOpacity(0.f)
        , mRainSpeed(0.f)
        , mRainDiameter(0.f)
        , mRainMinHeight(0.f)
        , mRainMaxHeight(0.f)
        , mRainEntranceSpeed(1.f)
        , mRainMaxRaindrops(0)
        , mRainRipplesEnabled(Fallback::Map::getBool("Weather_Rain_Ripples"))
        , mSnowRipplesEnabled(Fallback::Map::getBool("Weather_Snow_Ripples"))
        , mWindSpeed(0.f)
        , mBaseWindSpeed(0.f)
        , mEnabled(true)
        , mSunglareEnabled(true)
        , mPrecipitationAlpha(0.f)
        , mDirtyParticlesEffect(false)
    {
        mSkyRootNode = new CameraRelativeTransform;
        mSkyRootNode->setName("Sky Root");
        // Assign empty program to specify we don't want shaders when we are rendering in FFP pipeline
        if (!mSceneManager->getForceShaders())
            mSkyRootNode->getOrCreateStateSet()->setAttributeAndModes(new osg::Program(),
                osg::StateAttribute::OVERRIDE | osg::StateAttribute::PROTECTED | osg::StateAttribute::ON);
        mSceneManager->setUpNormalsRTForStateSet(mSkyRootNode->getOrCreateStateSet(), false);
        SceneUtil::ShadowManager::instance().disableShadowsForStateSet(*mSkyRootNode->getOrCreateStateSet());
        parentNode->addChild(mSkyRootNode);

        mEarlyRenderBinRoot = new osg::Group;
        // render before the world is rendered
        mEarlyRenderBinRoot->getOrCreateStateSet()->setRenderBinDetails(RenderBin_Sky, "RenderBin");
        // Prevent unwanted clipping by water reflection camera's clipping plane
        mEarlyRenderBinRoot->getOrCreateStateSet()->setMode(GL_CLIP_PLANE0, osg::StateAttribute::OFF);

        if (enableSkyRTT)
        {
            mSkyRTT = new SkyRTT(Settings::fog().mSkyRttResolution, mEarlyRenderBinRoot);
            mSkyRootNode->addChild(mSkyRTT);
        }

        mSkyNode = new osg::Group;
        mSkyNode->setNodeMask(Mask_Sky);
        mSkyNode->addChild(mEarlyRenderBinRoot);
        mSkyRootNode->addChild(mSkyNode);

        mUnderwaterSwitch = new UnderwaterSwitchCallback(mSkyRootNode);

        mPrecipitationOcclusion = Settings::shaders().mWeatherParticleOcclusion;
        mPrecipitationOccluder = std::make_unique<PrecipitationOccluder>(mSkyRootNode, parentNode, rootNode, camera);
    }

    void SkyManager::create()
    {
        assert(!mCreated);

        bool forceShaders = mSceneManager->getForceShaders();

        mAtmosphereDay = mSceneManager->getInstance(Settings::models().mSkyatmosphere.get(), mEarlyRenderBinRoot);
        ModVertexAlphaVisitor modAtmosphere(ModVertexAlphaVisitor::Atmosphere);
        mAtmosphereDay->accept(modAtmosphere);

        mAtmosphereUpdater = new AtmosphereUpdater;
        mAtmosphereDay->addUpdateCallback(mAtmosphereUpdater);

        mAtmosphereNightNode = new osg::PositionAttitudeTransform;
        mAtmosphereNightNode->setNodeMask(0);
        mEarlyRenderBinRoot->addChild(mAtmosphereNightNode);

        osg::ref_ptr<osg::Node> atmosphereNight;
        if (mSceneManager->getVFS()->exists(Settings::models().mSkynight02.get()))
            atmosphereNight = mSceneManager->getInstance(Settings::models().mSkynight02.get(), mAtmosphereNightNode);
        else
            atmosphereNight = mSceneManager->getInstance(Settings::models().mSkynight01.get(), mAtmosphereNightNode);
        atmosphereNight->getOrCreateStateSet()->setAttributeAndModes(
            createAlphaTrackingUnlitMaterial(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

        ModVertexAlphaVisitor modStars(ModVertexAlphaVisitor::Stars);
        atmosphereNight->accept(modStars);
        mAtmosphereNightUpdater = new AtmosphereNightUpdater(mSceneManager->getImageManager(), forceShaders);
        atmosphereNight->addUpdateCallback(mAtmosphereNightUpdater);

        mSun = std::make_unique<Sun>(mEarlyRenderBinRoot, *mSceneManager);
        mSun->setSunglare(mSunglareEnabled);
        mMasser = std::make_unique<Moon>(
            mEarlyRenderBinRoot, *mSceneManager, Fallback::Map::getFloat("Moons_Masser_Size") / 125, Moon::Type_Masser);
        mSecunda = std::make_unique<Moon>(mEarlyRenderBinRoot, *mSceneManager,
            Fallback::Map::getFloat("Moons_Secunda_Size") / 125, Moon::Type_Secunda);

        mCloudNode = new osg::Group;
        mEarlyRenderBinRoot->addChild(mCloudNode);

        mCloudMesh = new osg::PositionAttitudeTransform;
        osg::ref_ptr<osg::Node> cloudMeshChild
            = mSceneManager->getInstance(Settings::models().mSkyclouds.get(), mCloudMesh);
        mCloudUpdater = new CloudUpdater(forceShaders);
        mCloudUpdater->setOpacity(1.f);
        cloudMeshChild->addUpdateCallback(mCloudUpdater);
        mCloudMesh->addChild(cloudMeshChild);

        mNextCloudMesh = new osg::PositionAttitudeTransform;
        osg::ref_ptr<osg::Node> nextCloudMeshChild
            = mSceneManager->getInstance(Settings::models().mSkyclouds.get(), mNextCloudMesh);
        mNextCloudUpdater = new CloudUpdater(forceShaders);
        mNextCloudUpdater->setOpacity(0.f);
        nextCloudMeshChild->addUpdateCallback(mNextCloudUpdater);
        mNextCloudMesh->setNodeMask(0);
        mNextCloudMesh->addChild(nextCloudMeshChild);

        mCloudNode->addChild(mCloudMesh);
        mCloudNode->addChild(mNextCloudMesh);

        ModVertexAlphaVisitor modClouds(ModVertexAlphaVisitor::Clouds);
        mCloudMesh->accept(modClouds);
        mNextCloudMesh->accept(modClouds);

        if (mSceneManager->getForceShaders())
        {
            Shader::ShaderManager::DefineMap defines = {};
            Stereo::shaderStereoDefines(defines);
            auto program = mSceneManager->getShaderManager().getProgram("sky", defines);
            mEarlyRenderBinRoot->getOrCreateStateSet()->addUniform(new osg::Uniform("pass", -1));
            mEarlyRenderBinRoot->getOrCreateStateSet()->setAttributeAndModes(
                program, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        }

        osg::ref_ptr<osg::Depth> depth = new SceneUtil::AutoDepth;
        depth->setWriteMask(false);
        mEarlyRenderBinRoot->getOrCreateStateSet()->setAttributeAndModes(depth);
        mEarlyRenderBinRoot->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
        mEarlyRenderBinRoot->getOrCreateStateSet()->setMode(GL_FOG, osg::StateAttribute::OFF);

        mMoonScriptColor = Fallback::Map::getColour("Moons_Script_Color");

        mCreated = true;
    }

    void SkyManager::createRain()
    {
        if (mRainNode)
            return;

        mRainNode = new osg::Group;

        mRainParticleSystem = new NifOsg::ParticleSystem;
        osg::Vec3 rainRange = osg::Vec3(mRainDiameter, mRainDiameter, (mRainMinHeight + mRainMaxHeight) / 2.f);

        mRainParticleSystem->setParticleAlignment(osgParticle::ParticleSystem::FIXED);
        mRainParticleSystem->setAlignVectorX(osg::Vec3f(0.1f, 0, 0));
        mRainParticleSystem->setAlignVectorY(osg::Vec3f(0, 0, 1));

        osg::ref_ptr<osg::StateSet> stateset = mRainParticleSystem->getOrCreateStateSet();

        constexpr VFS::Path::NormalizedView raindropImage("textures/tx_raindrop_01.dds");
        osg::ref_ptr<osg::Texture2D> raindropTex
            = new osg::Texture2D(mSceneManager->getImageManager()->getImage(raindropImage));
        raindropTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        raindropTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

        stateset->setTextureAttributeAndModes(0, raindropTex);
        stateset->setNestRenderBins(false);
        stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        stateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
        stateset->setMode(GL_BLEND, osg::StateAttribute::ON);

        osg::ref_ptr<osg::Material> mat = new osg::Material;
        mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1, 1, 1, 1));
        mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(1, 1, 1, 1));
        mat->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
        stateset->setAttributeAndModes(mat);

        osgParticle::Particle& particleTemplate = mRainParticleSystem->getDefaultParticleTemplate();
        particleTemplate.setSizeRange(osgParticle::rangef(5.f, 15.f));
        particleTemplate.setAlphaRange(osgParticle::rangef(1.f, 1.f));
        particleTemplate.setLifeTime(1);

        osg::ref_ptr<osgParticle::ModularEmitter> emitter = new osgParticle::ModularEmitter;
        emitter->setParticleSystem(mRainParticleSystem);

        osg::ref_ptr<osgParticle::BoxPlacer> placer = new osgParticle::BoxPlacer;
        placer->setXRange(-rainRange.x() / 2, rainRange.x() / 2);
        placer->setYRange(-rainRange.y() / 2, rainRange.y() / 2);
        placer->setZRange(-rainRange.z() / 2, rainRange.z() / 2);
        emitter->setPlacer(placer);
        mPlacer = placer;

        // FIXME: vanilla engine does not use a particle system to handle rain, it uses a NIF-file with 20 raindrops in
        // it. It spawns the (maxRaindrops-getParticleSystem()->numParticles())*dt/rainEntranceSpeed batches every frame
        // (near 1-2). Since the rain is a regular geometry, it produces water ripples, also in theory it can be removed
        // if collides with something.
        osg::ref_ptr<RainCounter> counter = new RainCounter;
        counter->setNumberOfParticlesPerSecondToCreate(mRainMaxRaindrops / mRainEntranceSpeed * 20);
        emitter->setCounter(counter);
        mCounter = counter;

        osg::ref_ptr<RainShooter> shooter = new RainShooter;
        mRainShooter = shooter;
        emitter->setShooter(shooter);

        osg::ref_ptr<osgParticle::ParticleSystemUpdater> updater = new osgParticle::ParticleSystemUpdater;
        updater->addParticleSystem(mRainParticleSystem);

        osg::ref_ptr<osgParticle::ModularProgram> program = new osgParticle::ModularProgram;
        program->addOperator(new WrapAroundOperator(mCamera, rainRange));
        program->addOperator(new WeatherAlphaOperator(mPrecipitationAlpha, true));
        program->setParticleSystem(mRainParticleSystem);
        mRainNode->addChild(program);

        mRainNode->addChild(emitter);
        mRainNode->addChild(mRainParticleSystem);
        mRainNode->addChild(updater);

        // Note: if we ever switch to regular geometry rain, it'll need to use an AlphaFader.
        mRainNode->addCullCallback(mUnderwaterSwitch);
        mRainNode->setNodeMask(Mask_WeatherParticles);

        mRainParticleSystem->setUserValue("simpleLighting", true);
        mRainParticleSystem->setUserValue("particleOcclusion", true);
        mSceneManager->recreateShaders(mRainNode);

        mSkyNode->addChild(mRainNode);
        if (mPrecipitationOcclusion)
            mPrecipitationOccluder->enable();
    }

    void SkyManager::destroyRain()
    {
        if (!mRainNode)
            return;

        mSkyNode->removeChild(mRainNode);
        mRainNode = nullptr;
        mPlacer = nullptr;
        mCounter = nullptr;
        mRainParticleSystem = nullptr;
        mRainShooter = nullptr;
        mPrecipitationOccluder->disable();
    }

    SkyManager::~SkyManager()
    {
        if (mSkyRootNode)
        {
            mSkyRootNode->getParent(0)->removeChild(mSkyRootNode);
            mSkyRootNode = nullptr;
        }
    }

    int SkyManager::getMasserPhase() const
    {
        if (!mCreated)
            return 0;
        return mMasser->getPhaseInt();
    }

    int SkyManager::getSecundaPhase() const
    {
        if (!mCreated)
            return 0;
        return mSecunda->getPhaseInt();
    }

    bool SkyManager::isEnabled()
    {
        return mEnabled;
    }

    bool SkyManager::hasRain() const
    {
        return mRainNode != nullptr;
    }

    bool SkyManager::getRainRipplesEnabled() const
    {
        if (!mEnabled)
            return false;

        if (hasRain())
            return mRainRipplesEnabled;

        if (mParticleNode && mCurrentParticleEffect == Settings::models().mWeathersnow.get())
            return mSnowRipplesEnabled;

        return false;
    }

    float SkyManager::getPrecipitationAlpha() const
    {
        return mPrecipitationAlpha;
    }

    void SkyManager::update(float duration)
    {
        if (!mEnabled)
            return;

        switchUnderwaterRain();

        if (mIsStorm && mParticleNode)
        {
            osg::Quat quat;
            quat.makeRotate(MWWorld::Weather::defaultDirection(), mStormParticleDirection);
            // Morrowind deliberately rotates the blizzard mesh, so so should we.
            if (mCurrentParticleEffect == Settings::models().mWeatherblizzard.get())
                quat.makeRotate(osg::Vec3f(-1, 0, 0), mStormParticleDirection);
            mParticleNode->setAttitude(quat);
        }

        const float timeScale = MWBase::Environment::get().getWorld()->getTimeManager()->getGameTimeScale();

        // UV Scroll the clouds
        float cloudDelta = duration * mCloudSpeed / 400.f;
        if (mTimescaleClouds)
            cloudDelta *= timeScale / 60.f;

        mCloudAnimationTimer += cloudDelta;
        if (mCloudAnimationTimer >= 4.f)
            mCloudAnimationTimer -= 4.f;

        mNextCloudUpdater->setTextureCoord(mCloudAnimationTimer);
        mCloudUpdater->setTextureCoord(mCloudAnimationTimer);

        // morrowind rotates each cloud mesh independently
        osg::Quat rotation;
        rotation.makeRotate(MWWorld::Weather::defaultDirection(), mStormDirection);
        mCloudMesh->setAttitude(rotation);

        if (mNextCloudMesh->getNodeMask())
        {
            rotation.makeRotate(MWWorld::Weather::defaultDirection(), mNextStormDirection);
            mNextCloudMesh->setAttitude(rotation);
        }

        // rotate the stars by 360 degrees every 4 days
        mAtmosphereNightRoll += timeScale * duration * osg::DegreesToRadians(360.f) / (3600 * 96.f);
        if (mAtmosphereNightNode->getNodeMask() != 0)
            mAtmosphereNightNode->setAttitude(osg::Quat(mAtmosphereNightRoll, osg::Vec3f(0, 0, 1)));
        mPrecipitationOccluder->update();
    }

    void SkyManager::setEnabled(bool enabled)
    {
        if (enabled && !mCreated)
            create();

        const osg::Node::NodeMask mask = enabled ? Mask_Sky : 0u;

        mEarlyRenderBinRoot->setNodeMask(mask);
        mSkyNode->setNodeMask(mask);

        if (!enabled && mParticleNode && mParticleEffect)
        {
            mCurrentParticleEffect.clear();
            mDirtyParticlesEffect = true;
        }

        mEnabled = enabled;
    }

    void SkyManager::setMoonColour(bool red)
    {
        if (!mCreated)
            return;
        mSecunda->setColor(red ? mMoonScriptColor : osg::Vec4f(1, 1, 1, 1));
    }

    void SkyManager::updateRainParameters()
    {
        if (mRainShooter)
        {
            float angle = -std::atan(mWindSpeed / 50.f);
            mRainShooter->setVelocity(osg::Vec3f(0, mRainSpeed * std::sin(angle), -mRainSpeed / std::cos(angle)));
            mRainShooter->setAngle(angle);

            osg::Vec3 rainRange = osg::Vec3(mRainDiameter, mRainDiameter, (mRainMinHeight + mRainMaxHeight) / 2.f);

            mPlacer->setXRange(-rainRange.x() / 2, rainRange.x() / 2);
            mPlacer->setYRange(-rainRange.y() / 2, rainRange.y() / 2);
            mPlacer->setZRange(-rainRange.z() / 2, rainRange.z() / 2);

            mCounter->setNumberOfParticlesPerSecondToCreate(mRainMaxRaindrops / mRainEntranceSpeed * 20);
            mPrecipitationOccluder->updateRange(rainRange);
        }
    }

    void SkyManager::switchUnderwaterRain()
    {
        if (!mRainParticleSystem)
            return;

        bool freeze = mUnderwaterSwitch->isUnderwater();
        mRainParticleSystem->setFrozen(freeze);
    }

    void SkyManager::setWeather(const WeatherResult& weather)
    {
        if (!mCreated)
            return;

        mRainEntranceSpeed = weather.mRainEntranceSpeed;
        mRainMaxRaindrops = weather.mRainMaxRaindrops;
        mRainDiameter = weather.mRainDiameter;
        mRainMinHeight = weather.mRainMinHeight;
        mRainMaxHeight = weather.mRainMaxHeight;
        mRainSpeed = weather.mRainSpeed;
        mWindSpeed = weather.mWindSpeed;
        mBaseWindSpeed = weather.mBaseWindSpeed;

        if (mRainEffect != weather.mRainEffect)
        {
            mRainEffect = weather.mRainEffect;
            if (!mRainEffect.empty())
            {
                createRain();
            }
            else
            {
                destroyRain();
            }
        }

        updateRainParameters();

        mIsStorm = weather.mIsStorm;

        if (mIsStorm)
            mStormDirection = weather.mStormDirection;

        if (mDirtyParticlesEffect || (mCurrentParticleEffect != weather.mParticleEffect))
        {
            mDirtyParticlesEffect = false;
            mCurrentParticleEffect = weather.mParticleEffect;

            // cleanup old particles
            if (mParticleEffect)
            {
                mParticleNode->removeChild(mParticleEffect);
                mParticleEffect = nullptr;
            }

            if (mCurrentParticleEffect.empty())
            {
                if (mParticleNode)
                {
                    mSkyNode->removeChild(mParticleNode);
                    mParticleNode = nullptr;
                }
                if (mRainEffect.empty())
                {
                    mPrecipitationOccluder->disable();
                }
            }
            else
            {
                if (!mParticleNode)
                {
                    mParticleNode = new osg::PositionAttitudeTransform;
                    mParticleNode->addCullCallback(mUnderwaterSwitch);
                    mParticleNode->setNodeMask(Mask_WeatherParticles);
                    mSkyNode->addChild(mParticleNode);
                }

                mParticleEffect = mSceneManager->getInstance(mCurrentParticleEffect, mParticleNode);

                SceneUtil::AssignControllerSourcesVisitor assignVisitor(std::make_shared<SceneUtil::FrameTimeSource>());
                mParticleEffect->accept(assignVisitor);

                SetupVisitor alphaFaderSetupVisitor(mPrecipitationAlpha);
                mParticleEffect->accept(alphaFaderSetupVisitor);

                SceneUtil::FindByClassVisitor findPSVisitor("ParticleSystem");
                mParticleEffect->accept(findPSVisitor);

                const osg::Vec3 defaultWrapRange = osg::Vec3(1024, 1024, 800);
                const bool occlusionEnabledForEffect
                    = !mRainEffect.empty() || mCurrentParticleEffect == Settings::models().mWeathersnow.get();

                for (unsigned int i = 0; i < findPSVisitor.mFoundNodes.size(); ++i)
                {
                    osgParticle::ParticleSystem* ps
                        = static_cast<osgParticle::ParticleSystem*>(findPSVisitor.mFoundNodes[i]);

                    osg::ref_ptr<osgParticle::ModularProgram> program = new osgParticle::ModularProgram;
                    if (occlusionEnabledForEffect)
                        program->addOperator(new WrapAroundOperator(mCamera, defaultWrapRange));
                    program->addOperator(new WeatherAlphaOperator(mPrecipitationAlpha, false));
                    program->setParticleSystem(ps);
                    mParticleNode->addChild(program);

                    for (int particleIndex = 0; particleIndex < ps->numParticles(); ++particleIndex)
                    {
                        ps->getParticle(particleIndex)
                            ->setAlphaRange(osgParticle::rangef(mPrecipitationAlpha, mPrecipitationAlpha));
                        ps->getParticle(particleIndex)->update(0, true);
                    }

                    ps->setUserValue("simpleLighting", true);

                    if (occlusionEnabledForEffect)
                        ps->setUserValue("particleOcclusion", true);
                }

                mSceneManager->recreateShaders(mParticleNode);

                if (mPrecipitationOcclusion && occlusionEnabledForEffect)
                {
                    mPrecipitationOccluder->enable();
                    mPrecipitationOccluder->updateRange(defaultWrapRange);
                }
            }
        }

        if (mClouds != weather.mCloudTexture)
        {
            mClouds = weather.mCloudTexture;

            const VFS::Path::Normalized texture
                = Misc::ResourceHelpers::correctTexturePath(VFS::Path::toNormalized(mClouds), *mSceneManager->getVFS());

            osg::ref_ptr<osg::Texture2D> cloudTex
                = new osg::Texture2D(mSceneManager->getImageManager()->getImage(texture));
            cloudTex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
            cloudTex->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);

            mCloudUpdater->setTexture(std::move(cloudTex));
        }

        if (mStormDirection != weather.mStormDirection)
            mStormDirection = weather.mStormDirection;

        if (mNextStormDirection != weather.mNextStormDirection)
            mNextStormDirection = weather.mNextStormDirection;

        if (mNextClouds != weather.mNextCloudTexture)
        {
            mNextClouds = weather.mNextCloudTexture;

            if (!mNextClouds.empty())
            {
                const VFS::Path::Normalized texture = Misc::ResourceHelpers::correctTexturePath(
                    VFS::Path::toNormalized(mNextClouds), *mSceneManager->getVFS());

                osg::ref_ptr<osg::Texture2D> cloudTex
                    = new osg::Texture2D(mSceneManager->getImageManager()->getImage(texture));
                cloudTex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
                cloudTex->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);

                mNextCloudUpdater->setTexture(std::move(cloudTex));
                mNextStormDirection = weather.mStormDirection;
            }
        }

        if (mCloudBlendFactor != weather.mCloudBlendFactor)
        {
            mCloudBlendFactor = std::clamp(weather.mCloudBlendFactor, 0.f, 1.f);

            mCloudUpdater->setOpacity(1.f - mCloudBlendFactor);
            mNextCloudUpdater->setOpacity(mCloudBlendFactor);
            mNextCloudMesh->setNodeMask(mCloudBlendFactor > 0.f ? ~0u : 0);
        }

        if (mCloudColour != weather.mFogColor)
        {
            osg::Vec4f clr(weather.mFogColor);
            clr += osg::Vec4f(0.13f, 0.13f, 0.13f, 0.f);

            mCloudUpdater->setEmissionColor(clr);
            mNextCloudUpdater->setEmissionColor(clr);

            mCloudColour = weather.mFogColor;
        }

        if (mSkyColour != weather.mSkyColor)
        {
            mSkyColour = weather.mSkyColor;

            mAtmosphereUpdater->setEmissionColor(mSkyColour);
            mMasser->setAtmosphereColor(mSkyColour);
            mSecunda->setAtmosphereColor(mSkyColour);
        }

        if (mFogColour != weather.mFogColor)
        {
            mFogColour = weather.mFogColor;
        }

        mCloudSpeed = weather.mCloudSpeed;

        mMasser->adjustTransparency(weather.mGlareView);
        mSecunda->adjustTransparency(weather.mGlareView);

        mSun->setColor(weather.mSunDiscColor);
        mSun->adjustTransparency(weather.mGlareView * weather.mSunDiscColor.a());

        float nextStarsOpacity = weather.mNightFade * weather.mGlareView;

        if (weather.mNight && mStarsOpacity != nextStarsOpacity)
        {
            mStarsOpacity = nextStarsOpacity;

            mAtmosphereNightUpdater->setFade(mStarsOpacity);
        }

        mAtmosphereNightNode->setNodeMask(weather.mNight ? ~0u : 0);
        mPrecipitationAlpha = weather.mPrecipitationAlpha;
    }

    float SkyManager::getBaseWindSpeed() const
    {
        if (!mCreated)
            return 0.f;

        return mBaseWindSpeed;
    }

    void SkyManager::setSunglare(bool enabled)
    {
        mSunglareEnabled = enabled;

        if (mSun)
            mSun->setSunglare(mSunglareEnabled);
    }

    void SkyManager::sunEnable()
    {
        if (!mCreated)
            return;

        mSun->setVisible(true);
    }

    void SkyManager::sunDisable()
    {
        if (!mCreated)
            return;

        mSun->setVisible(false);
    }

    void SkyManager::setStormParticleDirection(const osg::Vec3f& direction)
    {
        mStormParticleDirection = direction;
    }

    void SkyManager::setSunDirection(const osg::Vec3f& direction)
    {
        if (!mCreated)
            return;

        mSun->setDirection(direction);
    }

    void SkyManager::setMasserState(const MoonState& state)
    {
        if (!mCreated)
            return;

        mMasser->setState(state);
    }

    void SkyManager::setSecundaState(const MoonState& state)
    {
        if (!mCreated)
            return;

        mSecunda->setState(state);
    }

    void SkyManager::setGlareTimeOfDayFade(float val)
    {
        mSun->setGlareTimeOfDayFade(val);
    }

    void SkyManager::setWaterHeight(float height)
    {
        mUnderwaterSwitch->setWaterLevel(height);
    }

    void SkyManager::listAssetsToPreload(
        std::vector<VFS::Path::Normalized>& models, std::vector<VFS::Path::Normalized>& textures)
    {
        models.push_back(Settings::models().mSkyatmosphere);
        if (mSceneManager->getVFS()->exists(Settings::models().mSkynight02.get()))
            models.push_back(Settings::models().mSkynight02);
        models.push_back(Settings::models().mSkynight01);
        models.push_back(Settings::models().mSkyclouds);

        models.push_back(Settings::models().mWeatherashcloud);
        models.push_back(Settings::models().mWeatherblightcloud);
        models.push_back(Settings::models().mWeathersnow);
        models.push_back(Settings::models().mWeatherblizzard);

        textures.emplace_back("textures/tx_mooncircle_full_s.dds");
        textures.emplace_back("textures/tx_mooncircle_full_m.dds");

        textures.emplace_back("textures/tx_masser_new.dds");
        textures.emplace_back("textures/tx_masser_one_wax.dds");
        textures.emplace_back("textures/tx_masser_half_wax.dds");
        textures.emplace_back("textures/tx_masser_three_wax.dds");
        textures.emplace_back("textures/tx_masser_one_wan.dds");
        textures.emplace_back("textures/tx_masser_half_wan.dds");
        textures.emplace_back("textures/tx_masser_three_wan.dds");
        textures.emplace_back("textures/tx_masser_full.dds");

        textures.emplace_back("textures/tx_secunda_new.dds");
        textures.emplace_back("textures/tx_secunda_one_wax.dds");
        textures.emplace_back("textures/tx_secunda_half_wax.dds");
        textures.emplace_back("textures/tx_secunda_three_wax.dds");
        textures.emplace_back("textures/tx_secunda_one_wan.dds");
        textures.emplace_back("textures/tx_secunda_half_wan.dds");
        textures.emplace_back("textures/tx_secunda_three_wan.dds");
        textures.emplace_back("textures/tx_secunda_full.dds");

        textures.emplace_back("textures/tx_sun_05.dds");
        textures.emplace_back("textures/tx_sun_flash_grey_05.dds");

        textures.emplace_back("textures/tx_raindrop_01.dds");
    }

    void SkyManager::setWaterEnabled(bool enabled)
    {
        mUnderwaterSwitch->setEnabled(enabled);
    }
}
