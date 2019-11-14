#include "water.hpp"

#include <iomanip>

#include <osg/Fog>
#include <osg/Depth>
#include <osg/Group>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/PositionAttitudeTransform>
#include <osg/ClipNode>
#include <osg/FrontFace>

#include <osgDB/ReadFile>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

#include <osgUtil/IncrementalCompileOperation>
#include <osgUtil/CullVisitor>

#include <components/debug/debuglog.hpp>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/imagemanager.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/sceneutil/shadow.hpp>
#include <components/sceneutil/waterutil.hpp>

#include <components/misc/constants.hpp>

#include <components/nifosg/controller.hpp>

#include <components/shader/shadermanager.hpp>

#include <components/esm/loadcell.hpp>

#include <components/fallback/fallback.hpp>

#include "../mwworld/cellstore.hpp"

#include "vismask.hpp"
#include "ripplesimulation.hpp"
#include "renderbin.hpp"
#include "util.hpp"

namespace MWRender
{

// --------------------------------------------------------------------------------------------------------------------------------

/// @brief Allows to cull and clip meshes that are below a plane. Useful for reflection & refraction camera effects.
/// Also handles flipping of the plane when the eye point goes below it.
/// To use, simply create the scene as subgraph of this node, then do setPlane(const osg::Plane& plane);
class ClipCullNode : public osg::Group
{
    class PlaneCullCallback : public osg::NodeCallback
    {
    public:
        /// @param cullPlane The culling plane (in world space).
        PlaneCullCallback(const osg::Plane* cullPlane)
            : osg::NodeCallback()
            , mCullPlane(cullPlane)
        {
        }

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);

            osg::Polytope::PlaneList origPlaneList = cv->getProjectionCullingStack().back().getFrustum().getPlaneList();

            osg::Plane plane = *mCullPlane;
            plane.transform(*cv->getCurrentRenderStage()->getInitialViewMatrix());

            osg::Vec3d eyePoint = cv->getEyePoint();
            if (mCullPlane->intersect(osg::BoundingSphere(osg::Vec3d(0,0,eyePoint.z()), 0)) > 0)
                plane.flip();

            cv->getProjectionCullingStack().back().getFrustum().add(plane);

            traverse(node, nv);

            // undo
            cv->getProjectionCullingStack().back().getFrustum().set(origPlaneList);
        }

    private:
        const osg::Plane* mCullPlane;
    };

    class FlipCallback : public osg::NodeCallback
    {
    public:
        FlipCallback(const osg::Plane* cullPlane)
            : mCullPlane(cullPlane)
        {
        }

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);
            osg::Vec3d eyePoint = cv->getEyePoint();

            osg::RefMatrix* modelViewMatrix = new osg::RefMatrix(*cv->getModelViewMatrix());

            // apply the height of the plane
            // we can't apply this height in the addClipPlane() since the "flip the below graph" function would otherwise flip the height as well
            modelViewMatrix->preMultTranslate(mCullPlane->getNormal() * ((*mCullPlane)[3] * -1));

            // flip the below graph if the eye point is above the plane
            if (mCullPlane->intersect(osg::BoundingSphere(osg::Vec3d(0,0,eyePoint.z()), 0)) > 0)
            {
                modelViewMatrix->preMultScale(osg::Vec3(1,1,-1));
            }

            // move the plane back along its normal a little bit to prevent bleeding at the water shore
            const float clipFudge = -5;
            modelViewMatrix->preMultTranslate(mCullPlane->getNormal() * clipFudge);

            cv->pushModelViewMatrix(modelViewMatrix, osg::Transform::RELATIVE_RF);
            traverse(node, nv);
            cv->popModelViewMatrix();
        }

    private:
        const osg::Plane* mCullPlane;
    };

public:
    ClipCullNode()
    {
        addCullCallback (new PlaneCullCallback(&mPlane));

        mClipNodeTransform = new osg::Group;
        mClipNodeTransform->addCullCallback(new FlipCallback(&mPlane));
        addChild(mClipNodeTransform);

        mClipNode = new osg::ClipNode;

        mClipNodeTransform->addChild(mClipNode);
    }

    void setPlane (const osg::Plane& plane)
    {
        if (plane == mPlane)
            return;
        mPlane = plane;

        mClipNode->getClipPlaneList().clear();
        mClipNode->addClipPlane(new osg::ClipPlane(0, osg::Plane(mPlane.getNormal(), 0))); // mPlane.d() applied in FlipCallback
        mClipNode->setStateSetModes(*getOrCreateStateSet(), osg::StateAttribute::ON);
        mClipNode->setCullingActive(false);
    }

private:
    osg::ref_ptr<osg::Group> mClipNodeTransform;
    osg::ref_ptr<osg::ClipNode> mClipNode;

    osg::Plane mPlane;
};

/// This callback on the Camera has the effect of a RELATIVE_RF_INHERIT_VIEWPOINT transform mode (which does not exist in OSG).
/// We want to keep the View Point of the parent camera so we will not have to recreate LODs.
class InheritViewPointCallback : public osg::NodeCallback
{
public:
        InheritViewPointCallback() {}

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);
        osg::ref_ptr<osg::RefMatrix> modelViewMatrix = new osg::RefMatrix(*cv->getModelViewMatrix());
        cv->popModelViewMatrix();
        cv->pushModelViewMatrix(modelViewMatrix, osg::Transform::ABSOLUTE_RF_INHERIT_VIEWPOINT);
        traverse(node, nv);
    }
};

/// Moves water mesh away from the camera slightly if the camera gets too close on the Z axis.
/// The offset works around graphics artifacts that occurred with the GL_DEPTH_CLAMP when the camera gets extremely close to the mesh (seen on NVIDIA at least).
/// Must be added as a Cull callback.
class FudgeCallback : public osg::NodeCallback
{
public:
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);

        const float fudge = 0.2;
        if (std::abs(cv->getEyeLocal().z()) < fudge)
        {
            float diff = fudge - cv->getEyeLocal().z();
            osg::RefMatrix* modelViewMatrix = new osg::RefMatrix(*cv->getModelViewMatrix());

            if (cv->getEyeLocal().z() > 0)
                modelViewMatrix->preMultTranslate(osg::Vec3f(0,0,-diff));
            else
                modelViewMatrix->preMultTranslate(osg::Vec3f(0,0,diff));

            cv->pushModelViewMatrix(modelViewMatrix, osg::Transform::RELATIVE_RF);
            traverse(node, nv);
            cv->popModelViewMatrix();
        }
        else
            traverse(node, nv);
    }
};

osg::ref_ptr<osg::Image> readPngImage (const std::string& file)
{
    // use boost in favor of osgDB::readImage, to handle utf-8 path issues on Windows
    boost::filesystem::ifstream inStream;
    inStream.open(file, std::ios_base::in | std::ios_base::binary);
    if (inStream.fail())
        Log(Debug::Error) << "Error: Failed to open " << file;
    osgDB::ReaderWriter* reader = osgDB::Registry::instance()->getReaderWriterForExtension("png");
    if (!reader)
    {
        Log(Debug::Error) << "Error: Failed to read " << file << ", no png readerwriter found";
        return osg::ref_ptr<osg::Image>();
    }
    osgDB::ReaderWriter::ReadResult result = reader->readImage(inStream);
    if (!result.success())
        Log(Debug::Error) << "Error: Failed to read " << file << ": " << result.message() << " code " << result.status();

    return result.getImage();
}


class Refraction : public osg::Camera
{
public:
    Refraction()
    {
        unsigned int rttSize = Settings::Manager::getInt("rtt size", "Water");
        setRenderOrder(osg::Camera::PRE_RENDER);
        setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        setReferenceFrame(osg::Camera::RELATIVE_RF);
        setSmallFeatureCullingPixelSize(Settings::Manager::getInt("small feature culling pixel size", "Water"));
        setName("RefractionCamera");
        setCullCallback(new InheritViewPointCallback);

        setCullMask(Mask_Effect|Mask_Scene|Mask_Object|Mask_Static|Mask_Terrain|Mask_Actor|Mask_ParticleSystem|Mask_Sky|Mask_Sun|Mask_Player|Mask_Lighting);
        setNodeMask(Mask_RenderToTexture);
        setViewport(0, 0, rttSize, rttSize);

        // No need for Update traversal since the scene is already updated as part of the main scene graph
        // A double update would mess with the light collection (in addition to being plain redundant)
        setUpdateCallback(new NoTraverseCallback);

        // No need for fog here, we are already applying fog on the water surface itself as well as underwater fog
        // assign large value to effectively turn off fog
        // shaders don't respect glDisable(GL_FOG)
        osg::ref_ptr<osg::Fog> fog (new osg::Fog);
        fog->setStart(10000000);
        fog->setEnd(10000000);
        getOrCreateStateSet()->setAttributeAndModes(fog, osg::StateAttribute::OFF|osg::StateAttribute::OVERRIDE);

        mClipCullNode = new ClipCullNode;
        addChild(mClipCullNode);

        mRefractionTexture = new osg::Texture2D;
        mRefractionTexture->setTextureSize(rttSize, rttSize);
        mRefractionTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        mRefractionTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        mRefractionTexture->setInternalFormat(GL_RGB);
        mRefractionTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        mRefractionTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

        attach(osg::Camera::COLOR_BUFFER, mRefractionTexture);

        mRefractionDepthTexture = new osg::Texture2D;
        mRefractionDepthTexture->setTextureSize(rttSize, rttSize);
        mRefractionDepthTexture->setSourceFormat(GL_DEPTH_COMPONENT);
        mRefractionDepthTexture->setInternalFormat(GL_DEPTH_COMPONENT24);
        mRefractionDepthTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        mRefractionDepthTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        mRefractionDepthTexture->setSourceType(GL_UNSIGNED_INT);
        mRefractionDepthTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        mRefractionDepthTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

        attach(osg::Camera::DEPTH_BUFFER, mRefractionDepthTexture);

        SceneUtil::ShadowManager::disableShadowsForStateSet(getOrCreateStateSet());
    }

    void setScene(osg::Node* scene)
    {
        if (mScene)
            mClipCullNode->removeChild(mScene);
        mScene = scene;
        mClipCullNode->addChild(scene);
    }

    void setWaterLevel(float waterLevel)
    {
        const float refractionScale = std::min(1.0f,std::max(0.0f,
            Settings::Manager::getFloat("refraction scale", "Water")));

        setViewMatrix(osg::Matrix::scale(1,1,refractionScale) *
            osg::Matrix::translate(0,0,(1.0 - refractionScale) * waterLevel));

        mClipCullNode->setPlane(osg::Plane(osg::Vec3d(0,0,-1), osg::Vec3d(0,0, waterLevel)));
    }

    osg::Texture2D* getRefractionTexture() const
    {
        return mRefractionTexture.get();
    }

    osg::Texture2D* getRefractionDepthTexture() const
    {
        return mRefractionDepthTexture.get();
    }

private:
    osg::ref_ptr<ClipCullNode> mClipCullNode;
    osg::ref_ptr<osg::Texture2D> mRefractionTexture;
    osg::ref_ptr<osg::Texture2D> mRefractionDepthTexture;
    osg::ref_ptr<osg::Node> mScene;
};

class Reflection : public osg::Camera
{
public:
    Reflection(bool isInterior)
    {
        setRenderOrder(osg::Camera::PRE_RENDER);
        setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        setReferenceFrame(osg::Camera::RELATIVE_RF);
        setSmallFeatureCullingPixelSize(Settings::Manager::getInt("small feature culling pixel size", "Water"));
        setName("ReflectionCamera");
        setCullCallback(new InheritViewPointCallback);

        setInterior(isInterior);
        setNodeMask(Mask_RenderToTexture);

        unsigned int rttSize = Settings::Manager::getInt("rtt size", "Water");
        setViewport(0, 0, rttSize, rttSize);

        // No need for Update traversal since the mSceneRoot is already updated as part of the main scene graph
        // A double update would mess with the light collection (in addition to being plain redundant)
        setUpdateCallback(new NoTraverseCallback);

        mReflectionTexture = new osg::Texture2D;
        mReflectionTexture->setTextureSize(rttSize, rttSize);
        mReflectionTexture->setInternalFormat(GL_RGB);
        mReflectionTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        mReflectionTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        mReflectionTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        mReflectionTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

        attach(osg::Camera::COLOR_BUFFER, mReflectionTexture);

        // XXX: should really flip the FrontFace on each renderable instead of forcing clockwise.
        osg::ref_ptr<osg::FrontFace> frontFace (new osg::FrontFace);
        frontFace->setMode(osg::FrontFace::CLOCKWISE);
        getOrCreateStateSet()->setAttributeAndModes(frontFace, osg::StateAttribute::ON);

        mClipCullNode = new ClipCullNode;
        addChild(mClipCullNode);

        SceneUtil::ShadowManager::disableShadowsForStateSet(getOrCreateStateSet());
    }

    void setInterior(bool isInterior)
    {
        int reflectionDetail = Settings::Manager::getInt("reflection detail", "Water");
        reflectionDetail = std::min(4, std::max(isInterior ? 2 : 0, reflectionDetail));
        unsigned int extraMask = 0;
        if(reflectionDetail >= 1) extraMask |= Mask_Terrain;
        if(reflectionDetail >= 2) extraMask |= Mask_Static;
        if(reflectionDetail >= 3) extraMask |= Mask_Effect|Mask_ParticleSystem|Mask_Object;
        if(reflectionDetail >= 4) extraMask |= Mask_Player|Mask_Actor;
        setCullMask(Mask_Scene|Mask_Sky|Mask_Lighting|extraMask);
    }

    void setWaterLevel(float waterLevel)
    {
        setViewMatrix(osg::Matrix::scale(1,1,-1) * osg::Matrix::translate(0,0,2 * waterLevel));
        mClipCullNode->setPlane(osg::Plane(osg::Vec3d(0,0,1), osg::Vec3d(0,0,waterLevel)));
    }

    void setScene(osg::Node* scene)
    {
        if (mScene)
            mClipCullNode->removeChild(mScene);
        mScene = scene;
        mClipCullNode->addChild(scene);
    }

    osg::Texture2D* getReflectionTexture() const
    {
        return mReflectionTexture.get();
    }

private:
    osg::ref_ptr<osg::Texture2D> mReflectionTexture;
    osg::ref_ptr<ClipCullNode> mClipCullNode;
    osg::ref_ptr<osg::Node> mScene;
};

/// DepthClampCallback enables GL_DEPTH_CLAMP for the current draw, if supported.
class DepthClampCallback : public osg::Drawable::DrawCallback
{
public:
    virtual void drawImplementation(osg::RenderInfo& renderInfo,const osg::Drawable* drawable) const
    {
        static bool supported = osg::isGLExtensionOrVersionSupported(renderInfo.getState()->getContextID(), "GL_ARB_depth_clamp", 3.3);
        if (!supported)
        {
            drawable->drawImplementation(renderInfo);
            return;
        }

        glEnable(GL_DEPTH_CLAMP);

        drawable->drawImplementation(renderInfo);

        // restore default
        glDisable(GL_DEPTH_CLAMP);
    }
};

Water::Water(osg::Group *parent, osg::Group* sceneRoot, Resource::ResourceSystem *resourceSystem,
             osgUtil::IncrementalCompileOperation *ico, const std::string& resourcePath)
    : mParent(parent)
    , mSceneRoot(sceneRoot)
    , mResourceSystem(resourceSystem)
    , mResourcePath(resourcePath)
    , mEnabled(true)
    , mToggled(true)
    , mTop(0)
    , mInterior(false)
{
    mSimulation.reset(new RippleSimulation(mSceneRoot, resourceSystem));

    mWaterGeom = SceneUtil::createWaterGeometry(Constants::CellSizeInUnits*150, 40, 900);
    mWaterGeom->setDrawCallback(new DepthClampCallback);
    mWaterGeom->setNodeMask(Mask_Water);

    mWaterNode = new osg::PositionAttitudeTransform;
    mWaterNode->setName("Water Root");
    mWaterNode->addChild(mWaterGeom);
    mWaterNode->addCullCallback(new FudgeCallback);

    // simple water fallback for the local map
    osg::ref_ptr<osg::Geometry> geom2 (osg::clone(mWaterGeom.get(), osg::CopyOp::DEEP_COPY_NODES));
    createSimpleWaterStateSet(geom2, Fallback::Map::getFloat("Water_Map_Alpha"));
    geom2->setNodeMask(Mask_SimpleWater);
    mWaterNode->addChild(geom2);
 
    mSceneRoot->addChild(mWaterNode);

    setHeight(mTop);

    mRainIntensityUniform = new osg::Uniform("rainIntensity",(float) 0.0);

    updateWaterMaterial();

    if (ico)
        ico->add(mWaterNode);
}

osg::Uniform *Water::getRainIntensityUniform()
{
    return mRainIntensityUniform.get();
}

void Water::updateWaterMaterial()
{
    if (mReflection)
    {
        mReflection->removeChildren(0, mReflection->getNumChildren());
        mParent->removeChild(mReflection);
        mReflection = nullptr;
    }
    if (mRefraction)
    {
        mRefraction->removeChildren(0, mRefraction->getNumChildren());
        mParent->removeChild(mRefraction);
        mRefraction = nullptr;
    }

    if (Settings::Manager::getBool("shader", "Water"))
    {
        mReflection = new Reflection(mInterior);
        mReflection->setWaterLevel(mTop);
        mReflection->setScene(mSceneRoot);
        mParent->addChild(mReflection);

        if (Settings::Manager::getBool("refraction", "Water"))
        {
            mRefraction = new Refraction;
            mRefraction->setWaterLevel(mTop);
            mRefraction->setScene(mSceneRoot);
            mParent->addChild(mRefraction);
        }

        createShaderWaterStateSet(mWaterGeom, mReflection, mRefraction);
    }
    else
        createSimpleWaterStateSet(mWaterGeom, Fallback::Map::getFloat("Water_World_Alpha"));

    updateVisible();
}

osg::Camera *Water::getReflectionCamera()
{
    return mReflection;
}

osg::Camera *Water::getRefractionCamera()
{
    return mRefraction;
}

void Water::createSimpleWaterStateSet(osg::Node* node, float alpha)
{
    osg::ref_ptr<osg::StateSet> stateset = SceneUtil::createSimpleWaterStateSet(alpha, MWRender::RenderBin_Water);

    node->setStateSet(stateset);

    // Add animated textures
    std::vector<osg::ref_ptr<osg::Texture2D> > textures;
    int frameCount = std::max(0, std::min(Fallback::Map::getInt("Water_SurfaceFrameCount"), 320));
    const std::string& texture = Fallback::Map::getString("Water_SurfaceTexture");
    for (int i=0; i<frameCount; ++i)
    {
        std::ostringstream texname;
        texname << "textures/water/" << texture << std::setw(2) << std::setfill('0') << i << ".dds";
        osg::ref_ptr<osg::Texture2D> tex (new osg::Texture2D(mResourceSystem->getImageManager()->getImage(texname.str())));
        tex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        tex->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
        textures.push_back(tex);
    }

    if (textures.empty())
        return;

    float fps = Fallback::Map::getFloat("Water_SurfaceFPS");

    osg::ref_ptr<NifOsg::FlipController> controller (new NifOsg::FlipController(0, 1.f/fps, textures));
    controller->setSource(std::shared_ptr<SceneUtil::ControllerSource>(new SceneUtil::FrameTimeSource));
    node->setUpdateCallback(controller);

    stateset->setTextureAttributeAndModes(0, textures[0], osg::StateAttribute::ON);

    // use a shader to render the simple water, ensuring that fog is applied per pixel as required.
    // this could be removed if a more detailed water mesh, using some sort of paging solution, is implemented.
    Resource::SceneManager* sceneManager = mResourceSystem->getSceneManager();
    bool oldValue = sceneManager->getForceShaders();
    sceneManager->setForceShaders(true);
    sceneManager->recreateShaders(node);
    sceneManager->setForceShaders(oldValue);
}

void Water::createShaderWaterStateSet(osg::Node* node, Reflection* reflection, Refraction* refraction)
{
    // use a define map to conditionally compile the shader
    std::map<std::string, std::string> defineMap;
    defineMap.insert(std::make_pair(std::string("refraction_enabled"), std::string(refraction ? "1" : "0")));

    Shader::ShaderManager& shaderMgr = mResourceSystem->getSceneManager()->getShaderManager();
    osg::ref_ptr<osg::Shader> vertexShader (shaderMgr.getShader("water_vertex.glsl", defineMap, osg::Shader::VERTEX));
    osg::ref_ptr<osg::Shader> fragmentShader (shaderMgr.getShader("water_fragment.glsl", defineMap, osg::Shader::FRAGMENT));

    osg::ref_ptr<osg::Texture2D> normalMap (new osg::Texture2D(readPngImage(mResourcePath + "/shaders/water_nm.png")));

    if (normalMap->getImage())
        normalMap->getImage()->flipVertical();
    normalMap->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    normalMap->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    normalMap->setMaxAnisotropy(16);
    normalMap->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
    normalMap->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

    osg::ref_ptr<osg::StateSet> shaderStateset = new osg::StateSet;
    shaderStateset->addUniform(new osg::Uniform("normalMap", 0));
    shaderStateset->addUniform(new osg::Uniform("reflectionMap", 1));

    shaderStateset->setTextureAttributeAndModes(0, normalMap, osg::StateAttribute::ON);
    shaderStateset->setTextureAttributeAndModes(1, reflection->getReflectionTexture(), osg::StateAttribute::ON);

    if (refraction)
    {
        shaderStateset->setTextureAttributeAndModes(2, refraction->getRefractionTexture(), osg::StateAttribute::ON);
        shaderStateset->setTextureAttributeAndModes(3, refraction->getRefractionDepthTexture(), osg::StateAttribute::ON);
        shaderStateset->addUniform(new osg::Uniform("refractionMap", 2));
        shaderStateset->addUniform(new osg::Uniform("refractionDepthMap", 3));
        shaderStateset->setRenderBinDetails(MWRender::RenderBin_Default, "RenderBin");
    }
    else
    {
        shaderStateset->setMode(GL_BLEND, osg::StateAttribute::ON);

        shaderStateset->setRenderBinDetails(MWRender::RenderBin_Water, "RenderBin");

        osg::ref_ptr<osg::Depth> depth (new osg::Depth);
        depth->setWriteMask(false);
        shaderStateset->setAttributeAndModes(depth, osg::StateAttribute::ON);
    }

    shaderStateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

    shaderStateset->addUniform(mRainIntensityUniform.get());

    osg::ref_ptr<osg::Program> program (new osg::Program);
    program->addShader(vertexShader);
    program->addShader(fragmentShader);
    shaderStateset->setAttributeAndModes(program, osg::StateAttribute::ON);

    node->setStateSet(shaderStateset);
    node->setUpdateCallback(nullptr);
}

void Water::processChangedSettings(const Settings::CategorySettingVector& settings)
{
    updateWaterMaterial();
}

Water::~Water()
{
    mParent->removeChild(mWaterNode);

    if (mReflection)
    {
        mReflection->removeChildren(0, mReflection->getNumChildren());
        mParent->removeChild(mReflection);
        mReflection = nullptr;
    }
    if (mRefraction)
    {
        mRefraction->removeChildren(0, mRefraction->getNumChildren());
        mParent->removeChild(mRefraction);
        mRefraction = nullptr;
    }
}

void Water::listAssetsToPreload(std::vector<std::string> &textures)
{
    int frameCount = std::max(0, std::min(Fallback::Map::getInt("Water_SurfaceFrameCount"), 320));
    const std::string& texture = Fallback::Map::getString("Water_SurfaceTexture");
    for (int i=0; i<frameCount; ++i)
    {
        std::ostringstream texname;
        texname << "textures/water/" << texture << std::setw(2) << std::setfill('0') << i << ".dds";
        textures.push_back(texname.str());
    }
}

void Water::setEnabled(bool enabled)
{
    mEnabled = enabled;
    updateVisible();
}

void Water::changeCell(const MWWorld::CellStore* store)
{
    bool isInterior = !store->getCell()->isExterior();
    bool wasInterior = mInterior;
    if (!isInterior)
    {
        mWaterNode->setPosition(getSceneNodeCoordinates(store->getCell()->mData.mX, store->getCell()->mData.mY));
        mInterior = false;
    }
    else
    {
        mWaterNode->setPosition(osg::Vec3f(0,0,mTop));
        mInterior = true;
    }
    if(mInterior != wasInterior && mReflection)
        mReflection->setInterior(mInterior);

    // create a new StateSet to prevent threading issues
    osg::ref_ptr<osg::StateSet> nodeStateSet (new osg::StateSet);
    nodeStateSet->addUniform(new osg::Uniform("nodePosition", osg::Vec3f(mWaterNode->getPosition())));
    mWaterNode->setStateSet(nodeStateSet);
}

void Water::setHeight(const float height)
{
    mTop = height;

    mSimulation->setWaterHeight(height);

    osg::Vec3f pos = mWaterNode->getPosition();
    pos.z() = height;
    mWaterNode->setPosition(pos);

    if (mReflection)
        mReflection->setWaterLevel(mTop);
    if (mRefraction)
        mRefraction->setWaterLevel(mTop);
}

void Water::update(float dt)
{
    mSimulation->update(dt);
}

void Water::updateVisible()
{
    bool visible = mEnabled && mToggled;
    mWaterNode->setNodeMask(visible ? ~0 : 0);
    if (mRefraction)
        mRefraction->setNodeMask(visible ? Mask_RenderToTexture : 0);
    if (mReflection)
        mReflection->setNodeMask(visible ? Mask_RenderToTexture : 0);
}

bool Water::toggle()
{
    mToggled = !mToggled;
    updateVisible();
    return mToggled;
}

bool Water::isUnderwater(const osg::Vec3f &pos) const
{
    return pos.z() < mTop && mToggled && mEnabled;
}

osg::Vec3f Water::getSceneNodeCoordinates(int gridX, int gridY)
{
    return osg::Vec3f(static_cast<float>(gridX * Constants::CellSizeInUnits + (Constants::CellSizeInUnits / 2)),
                      static_cast<float>(gridY * Constants::CellSizeInUnits + (Constants::CellSizeInUnits / 2)), mTop);
}

void Water::addEmitter (const MWWorld::Ptr& ptr, float scale, float force)
{
    mSimulation->addEmitter (ptr, scale, force);
}

void Water::removeEmitter (const MWWorld::Ptr& ptr)
{
    mSimulation->removeEmitter (ptr);
}

void Water::updateEmitterPtr (const MWWorld::Ptr& old, const MWWorld::Ptr& ptr)
{
    mSimulation->updateEmitterPtr(old, ptr);
}

void Water::emitRipple(const osg::Vec3f &pos)
{
    mSimulation->emitRipple(pos);
}

void Water::removeCell(const MWWorld::CellStore *store)
{
    mSimulation->removeCell(store);
}

void Water::clearRipples()
{
    mSimulation->clear();
}

}
