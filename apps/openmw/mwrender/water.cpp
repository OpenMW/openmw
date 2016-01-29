#include "water.hpp"

#include <iomanip>

#include <osg/Fog>
#include <osg/Depth>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/PositionAttitudeTransform>
#include <osg/Depth>
#include <osg/ClipNode>
#include <osg/MatrixTransform>
#include <osg/FrontFace>
#include <osg/Shader>
#include <osg/GLExtensions>

#include <osgDB/ReadFile>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

#include <osgUtil/IncrementalCompileOperation>
#include <osgUtil/CullVisitor>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/texturemanager.hpp>

#include <components/nifosg/controller.hpp>
#include <components/sceneutil/controller.hpp>

#include <components/settings/settings.hpp>

#include <components/esm/loadcell.hpp>

#include <components/fallback/fallback.hpp>

#include "../mwworld/cellstore.hpp"

#include "vismask.hpp"
#include "ripplesimulation.hpp"
#include "renderbin.hpp"
#include "util.hpp"

namespace
{

    osg::ref_ptr<osg::Geometry> createWaterGeometry(float size, int segments, float textureRepeats)
    {
        osg::ref_ptr<osg::Vec3Array> verts (new osg::Vec3Array);
        osg::ref_ptr<osg::Vec2Array> texcoords (new osg::Vec2Array);

        // some drivers don't like huge triangles, so we do some subdivisons
        // a paged solution would be even better
        const float step = size/segments;
        const float texCoordStep = textureRepeats / segments;
        for (int x=0; x<segments; ++x)
        {
            for (int y=0; y<segments; ++y)
            {
                float x1 = -size/2.f + x*step;
                float y1 = -size/2.f + y*step;
                float x2 = x1 + step;
                float y2 = y1 + step;

                verts->push_back(osg::Vec3f(x1, y2, 0.f));
                verts->push_back(osg::Vec3f(x1, y1, 0.f));
                verts->push_back(osg::Vec3f(x2, y1, 0.f));
                verts->push_back(osg::Vec3f(x2, y2, 0.f));

                float u1 = x*texCoordStep;
                float v1 = y*texCoordStep;
                float u2 = u1 + texCoordStep;
                float v2 = v1 + texCoordStep;

                texcoords->push_back(osg::Vec2f(u1, v2));
                texcoords->push_back(osg::Vec2f(u1, v1));
                texcoords->push_back(osg::Vec2f(u2, v1));
                texcoords->push_back(osg::Vec2f(u2, v2));
            }
        }

        osg::ref_ptr<osg::Geometry> waterGeom (new osg::Geometry);
        waterGeom->setVertexArray(verts);
        waterGeom->setTexCoordArray(0, texcoords);

        osg::ref_ptr<osg::Vec3Array> normal (new osg::Vec3Array);
        normal->push_back(osg::Vec3f(0,0,1));
        waterGeom->setNormalArray(normal, osg::Array::BIND_OVERALL);

        waterGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,verts->size()));
        return waterGeom;
    }

}

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

/// Moves water mesh away from the camera slightly if the camera gets too close on the Z axis.
/// The offset works around graphics artifacts that occured with the GL_DEPTH_CLAMP when the camera gets extremely close to the mesh (seen on NVIDIA at least).
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

osg::ref_ptr<osg::Shader> readShader (osg::Shader::Type type, const std::string& file, const std::map<std::string, std::string>& defineMap = std::map<std::string, std::string>())
{
    osg::ref_ptr<osg::Shader> shader (new osg::Shader(type));

    // use boost in favor of osg::Shader::readShaderFile, to handle utf-8 path issues on Windows
    boost::filesystem::ifstream inStream;
    inStream.open(boost::filesystem::path(file));
    std::stringstream strstream;
    strstream << inStream.rdbuf();

    std::string shaderSource = strstream.str();

    for (std::map<std::string, std::string>::const_iterator it = defineMap.begin(); it != defineMap.end(); ++it)
    {
        size_t pos = shaderSource.find(it->first);
        if (pos != std::string::npos)
            shaderSource.replace(pos, it->first.length(), it->second);
    }

    shader->setShaderSource(shaderSource);
    return shader;
}

osg::ref_ptr<osg::Image> readPngImage (const std::string& file)
{
    // use boost in favor of osgDB::readImage, to handle utf-8 path issues on Windows
    boost::filesystem::ifstream inStream;
    inStream.open(file, std::ios_base::in | std::ios_base::binary);
    if (inStream.fail())
        std::cerr << "Failed to open " << file << std::endl;
    osgDB::ReaderWriter* reader = osgDB::Registry::instance()->getReaderWriterForExtension("png");
    if (!reader)
    {
        std::cerr << "Failed to read " << file << ", no png readerwriter found" << std::endl;
        return osg::ref_ptr<osg::Image>();
    }
    osgDB::ReaderWriter::ReadResult result = reader->readImage(inStream);
    if (!result.success())
        std::cerr << "Failed to read " << file << ": " << result.message() << " code " << result.status() << std::endl;

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

        setCullMask(Mask_Effect|Mask_Scene|Mask_Terrain|Mask_Actor|Mask_ParticleSystem|Mask_Sky|Mask_Sun|Mask_Player|Mask_Lighting);
        setNodeMask(Mask_RenderToTexture);
        setViewport(0, 0, rttSize, rttSize);

        // No need for Update traversal since the scene is already updated as part of the main scene graph
        // A double update would mess with the light collection (in addition to being plain redundant)
        setUpdateCallback(new NoTraverseCallback);

        // No need for fog here, we are already applying fog on the water surface itself as well as underwater fog
        getOrCreateStateSet()->setMode(GL_FOG, osg::StateAttribute::OFF|osg::StateAttribute::OVERRIDE);

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
        mRefractionDepthTexture->setSourceFormat(GL_DEPTH_COMPONENT);
        mRefractionDepthTexture->setInternalFormat(GL_DEPTH_COMPONENT24);
        mRefractionDepthTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        mRefractionDepthTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        mRefractionDepthTexture->setSourceType(GL_UNSIGNED_INT);
        mRefractionDepthTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        mRefractionDepthTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

        attach(osg::Camera::DEPTH_BUFFER, mRefractionDepthTexture);
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
    Reflection()
    {
        setRenderOrder(osg::Camera::PRE_RENDER);
        setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        setReferenceFrame(osg::Camera::RELATIVE_RF);

        setCullMask(Mask_Effect|Mask_Scene|Mask_Terrain|Mask_Actor|Mask_ParticleSystem|Mask_Sky|Mask_Player|Mask_Lighting);
        setNodeMask(Mask_RenderToTexture);

        unsigned int rttSize = Settings::Manager::getInt("rtt size", "Water");
        setViewport(0, 0, rttSize, rttSize);

        // No need for Update traversal since the mSceneRoot is already updated as part of the main scene graph
        // A double update would mess with the light collection (in addition to being plain redundant)
        setUpdateCallback(new NoTraverseCallback);

        mReflectionTexture = new osg::Texture2D;
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
    }

    void setWaterLevel(float waterLevel)
    {
        setViewMatrix(osg::Matrix::translate(0,0,-waterLevel) * osg::Matrix::scale(1,1,-1) * osg::Matrix::translate(0,0,waterLevel));

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

Water::Water(osg::Group *parent, osg::Group* sceneRoot, Resource::ResourceSystem *resourceSystem, osgUtil::IncrementalCompileOperation *ico,
             const Fallback::Map* fallback, const std::string& resourcePath)
    : mParent(parent)
    , mSceneRoot(sceneRoot)
    , mResourceSystem(resourceSystem)
    , mFallback(fallback)
    , mResourcePath(resourcePath)
    , mEnabled(true)
    , mToggled(true)
    , mTop(0)
{
    mSimulation.reset(new RippleSimulation(parent, resourceSystem, fallback));

    osg::ref_ptr<osg::Geometry> waterGeom = createWaterGeometry(CELL_SIZE*150, 40, 900);
    waterGeom->setDrawCallback(new DepthClampCallback);

    mWaterGeode = new osg::Geode;
    mWaterGeode->addDrawable(waterGeom);
    mWaterGeode->setNodeMask(Mask_Water);

    if (ico)
        ico->add(mWaterGeode);

    mWaterNode = new osg::PositionAttitudeTransform;
    mWaterNode->addChild(mWaterGeode);
    mWaterNode->addCullCallback(new FudgeCallback);

    // simple water fallback for the local map
    osg::ref_ptr<osg::Geode> geode2 (osg::clone(mWaterGeode.get(), osg::CopyOp::DEEP_COPY_NODES));
    createSimpleWaterStateSet(geode2, mFallback->getFallbackFloat("Water_Map_Alpha"));
    geode2->setNodeMask(Mask_SimpleWater);
    mWaterNode->addChild(geode2);

    mSceneRoot->addChild(mWaterNode);

    setHeight(mTop);

    updateWaterMaterial();
}

void Water::updateWaterMaterial()
{
    if (mReflection)
    {
        mParent->removeChild(mReflection);
        mReflection = NULL;
    }
    if (mRefraction)
    {
        mParent->removeChild(mRefraction);
        mRefraction = NULL;
    }

    if (Settings::Manager::getBool("shader", "Water"))
    {
        mReflection = new Reflection;
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

        createShaderWaterStateSet(mWaterGeode, mReflection, mRefraction);
    }
    else
        createSimpleWaterStateSet(mWaterGeode, mFallback->getFallbackFloat("Water_World_Alpha"));

    updateVisible();
}

void Water::createSimpleWaterStateSet(osg::Node* node, float alpha)
{
    osg::ref_ptr<osg::StateSet> stateset (new osg::StateSet);

    osg::ref_ptr<osg::Material> material (new osg::Material);
    material->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 1.f));
    material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(1.f, 1.f, 1.f, alpha));
    material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1.f, 1.f, 1.f, 1.f));
    material->setColorMode(osg::Material::OFF);
    stateset->setAttributeAndModes(material, osg::StateAttribute::ON);

    stateset->setMode(GL_BLEND, osg::StateAttribute::ON);
    stateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

    osg::ref_ptr<osg::Depth> depth (new osg::Depth);
    depth->setWriteMask(false);
    stateset->setAttributeAndModes(depth, osg::StateAttribute::ON);

    stateset->setRenderBinDetails(MWRender::RenderBin_Water, "RenderBin");

    node->setStateSet(stateset);

    std::vector<osg::ref_ptr<osg::Texture2D> > textures;
    int frameCount = mFallback->getFallbackInt("Water_SurfaceFrameCount");
    std::string texture = mFallback->getFallbackString("Water_SurfaceTexture");
    for (int i=0; i<frameCount; ++i)
    {
        std::ostringstream texname;
        texname << "textures/water/" << texture << std::setw(2) << std::setfill('0') << i << ".dds";
        textures.push_back(mResourceSystem->getTextureManager()->getTexture2D(texname.str(), osg::Texture::REPEAT, osg::Texture::REPEAT));
    }

    if (!textures.size())
        return;

    float fps = mFallback->getFallbackFloat("Water_SurfaceFPS");

    osg::ref_ptr<NifOsg::FlipController> controller (new NifOsg::FlipController(0, 1.f/fps, textures));
    controller->setSource(boost::shared_ptr<SceneUtil::ControllerSource>(new SceneUtil::FrameTimeSource));
    node->setUpdateCallback(controller);

    stateset->setTextureAttributeAndModes(0, textures[0], osg::StateAttribute::ON);
}

void Water::createShaderWaterStateSet(osg::Node* node, Reflection* reflection, Refraction* refraction)
{
    // use a define map to conditionally compile the shader
    std::map<std::string, std::string> defineMap;
    defineMap.insert(std::make_pair(std::string("@refraction_enabled"), std::string(refraction ? "1" : "0")));

    osg::ref_ptr<osg::Shader> vertexShader (readShader(osg::Shader::VERTEX, mResourcePath + "/shaders/water_vertex.glsl", defineMap));
    osg::ref_ptr<osg::Shader> fragmentShader (readShader(osg::Shader::FRAGMENT, mResourcePath + "/shaders/water_fragment.glsl", defineMap));

    osg::ref_ptr<osg::Texture2D> normalMap (new osg::Texture2D(readPngImage(mResourcePath + "/shaders/water_nm.png")));
    normalMap->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    normalMap->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    normalMap->setMaxAnisotropy(16);
    normalMap->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
    normalMap->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    normalMap->getImage()->flipVertical();

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

    osg::ref_ptr<osg::Program> program (new osg::Program);
    program->addShader(vertexShader);
    program->addShader(fragmentShader);
    shaderStateset->setAttributeAndModes(program, osg::StateAttribute::ON);

    node->setStateSet(shaderStateset);
    node->setUpdateCallback(NULL);
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
        mParent->removeChild(mReflection);
        mReflection = NULL;
    }
    if (mRefraction)
    {
        mParent->removeChild(mRefraction);
        mRefraction = NULL;
    }
}

void Water::setEnabled(bool enabled)
{
    mEnabled = enabled;
    updateVisible();
}

void Water::changeCell(const MWWorld::CellStore* store)
{
    if (store->getCell()->isExterior())
        mWaterNode->setPosition(getSceneNodeCoordinates(store->getCell()->mData.mX, store->getCell()->mData.mY));
    else
        mWaterNode->setPosition(osg::Vec3f(0,0,mTop));

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
    return osg::Vec3f(static_cast<float>(gridX * CELL_SIZE + (CELL_SIZE / 2)), static_cast<float>(gridY * CELL_SIZE + (CELL_SIZE / 2)), mTop);
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
