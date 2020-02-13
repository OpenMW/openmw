#include "sky.hpp"

#include <cmath>

#include <osg/ClipPlane>
#include <osg/Fog>
#include <osg/Transform>
#include <osg/Depth>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/TexEnvCombine>
#include <osg/TexMat>
#include <osg/OcclusionQueryNode>
#include <osg/ColorMask>
#include <osg/PositionAttitudeTransform>
#include <osg/BlendFunc>
#include <osg/AlphaFunc>
#include <osg/PolygonOffset>
#include <osg/observer_ptr>

#include <osgParticle/BoxPlacer>
#include <osgParticle/ModularEmitter>
#include <osgParticle/ParticleSystem>
#include <osgParticle/ParticleSystemUpdater>
#include <osgParticle/ConstantRateCounter>
#include <osgParticle/RadialShooter>

#include <osgParticle/Operator>
#include <osgParticle/ModularProgram>

#include <components/misc/rng.hpp>

#include <components/misc/resourcehelpers.hpp>

#include <components/resource/scenemanager.hpp>
#include <components/resource/imagemanager.hpp>

#include <components/vfs/manager.hpp>
#include <components/fallback/fallback.hpp>

#include <components/sceneutil/util.hpp>
#include <components/sceneutil/statesetupdater.hpp>
#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/visitor.hpp>
#include <components/sceneutil/shadow.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "vismask.hpp"
#include "renderbin.hpp"

namespace
{
    osg::ref_ptr<osg::Material> createAlphaTrackingUnlitMaterial()
    {
        osg::ref_ptr<osg::Material> mat = new osg::Material;
        mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 1.f));
        mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 1.f));
        mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(1.f, 1.f, 1.f, 1.f));
        mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 0.f));
        mat->setColorMode(osg::Material::DIFFUSE);
        return mat;
    }

    osg::ref_ptr<osg::Material> createUnlitMaterial()
    {
        osg::ref_ptr<osg::Material> mat = new osg::Material;
        mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 1.f));
        mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 1.f));
        mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(1.f, 1.f, 1.f, 1.f));
        mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 0.f));
        mat->setColorMode(osg::Material::OFF);
        return mat;
    }

    osg::ref_ptr<osg::Geometry> createTexturedQuad(int numUvSets=1)
    {
        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;

        osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array;
        verts->push_back(osg::Vec3f(-0.5, -0.5, 0));
        verts->push_back(osg::Vec3f(-0.5, 0.5, 0));
        verts->push_back(osg::Vec3f(0.5, 0.5, 0));
        verts->push_back(osg::Vec3f(0.5, -0.5, 0));

        geom->setVertexArray(verts);

        osg::ref_ptr<osg::Vec2Array> texcoords = new osg::Vec2Array;
        texcoords->push_back(osg::Vec2f(0, 0));
        texcoords->push_back(osg::Vec2f(0, 1));
        texcoords->push_back(osg::Vec2f(1, 1));
        texcoords->push_back(osg::Vec2f(1, 0));

        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
        colors->push_back(osg::Vec4(1.f, 1.f, 1.f, 1.f));
        geom->setColorArray(colors, osg::Array::BIND_OVERALL);

        for (int i=0; i<numUvSets; ++i)
            geom->setTexCoordArray(i, texcoords, osg::Array::BIND_PER_VERTEX);

        geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,4));

        return geom;
    }

}

namespace MWRender
{

class AtmosphereUpdater : public SceneUtil::StateSetUpdater
{
public:
    void setEmissionColor(const osg::Vec4f& emissionColor)
    {
        mEmissionColor = emissionColor;
    }

protected:
    virtual void setDefaults(osg::StateSet* stateset)
    {
        stateset->setAttributeAndModes(createAlphaTrackingUnlitMaterial(), osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
    }

    virtual void apply(osg::StateSet* stateset, osg::NodeVisitor* /*nv*/)
    {
        osg::Material* mat = static_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
        mat->setEmission(osg::Material::FRONT_AND_BACK, mEmissionColor);
    }

private:
    osg::Vec4f mEmissionColor;
};

class AtmosphereNightUpdater : public SceneUtil::StateSetUpdater
{
public:
    AtmosphereNightUpdater(Resource::ImageManager* imageManager)
    {
        // we just need a texture, its contents don't really matter
        mTexture = new osg::Texture2D(imageManager->getWarningImage());
    }

    void setFade(const float fade)
    {
        mColor.a() = fade;
    }

protected:
    virtual void setDefaults(osg::StateSet* stateset)
    {
        osg::ref_ptr<osg::TexEnvCombine> texEnv (new osg::TexEnvCombine);
        texEnv->setCombine_Alpha(osg::TexEnvCombine::MODULATE);
        texEnv->setSource0_Alpha(osg::TexEnvCombine::PREVIOUS);
        texEnv->setSource1_Alpha(osg::TexEnvCombine::CONSTANT);
        texEnv->setCombine_RGB(osg::TexEnvCombine::REPLACE);
        texEnv->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);

        stateset->setTextureAttributeAndModes(1, mTexture, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
        stateset->setTextureAttributeAndModes(1, texEnv, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
    }

    virtual void apply(osg::StateSet* stateset, osg::NodeVisitor* /*nv*/)
    {
        osg::TexEnvCombine* texEnv = static_cast<osg::TexEnvCombine*>(stateset->getTextureAttribute(1, osg::StateAttribute::TEXENV));
        texEnv->setConstantColor(mColor);
    }

    osg::ref_ptr<osg::Texture2D> mTexture;

    osg::Vec4f mColor;
};

class CloudUpdater : public SceneUtil::StateSetUpdater
{
public:
    CloudUpdater()
        : mAnimationTimer(0.f)
        , mOpacity(0.f)
    {
    }

    void setAnimationTimer(float timer)
    {
        mAnimationTimer = timer;
    }

    void setTexture(osg::ref_ptr<osg::Texture2D> texture)
    {
        mTexture = texture;
    }
    void setEmissionColor(const osg::Vec4f& emissionColor)
    {
        mEmissionColor = emissionColor;
    }
    void setOpacity(float opacity)
    {
        mOpacity = opacity;
    }

protected:
    virtual void setDefaults(osg::StateSet *stateset)
    {
        osg::ref_ptr<osg::TexMat> texmat (new osg::TexMat);
        stateset->setTextureAttributeAndModes(0, texmat, osg::StateAttribute::ON);
        stateset->setTextureAttributeAndModes(1, texmat, osg::StateAttribute::ON);
        stateset->setAttribute(createAlphaTrackingUnlitMaterial(), osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);

        // need to set opacity on a separate texture unit, diffuse alpha is used by the vertex colors already
        osg::ref_ptr<osg::TexEnvCombine> texEnvCombine (new osg::TexEnvCombine);
        texEnvCombine->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
        texEnvCombine->setSource0_Alpha(osg::TexEnvCombine::PREVIOUS);
        texEnvCombine->setSource1_Alpha(osg::TexEnvCombine::CONSTANT);
        texEnvCombine->setConstantColor(osg::Vec4f(1,1,1,1));
        texEnvCombine->setCombine_Alpha(osg::TexEnvCombine::MODULATE);
        texEnvCombine->setCombine_RGB(osg::TexEnvCombine::REPLACE);

        stateset->setTextureAttributeAndModes(1, texEnvCombine, osg::StateAttribute::ON);

        stateset->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
        stateset->setTextureMode(1, GL_TEXTURE_2D, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
    }

    virtual void apply(osg::StateSet *stateset, osg::NodeVisitor *nv)
    {
        osg::TexMat* texMat = static_cast<osg::TexMat*>(stateset->getTextureAttribute(0, osg::StateAttribute::TEXMAT));
        texMat->setMatrix(osg::Matrix::translate(osg::Vec3f(0, -mAnimationTimer, 0.f)));

        stateset->setTextureAttribute(0, mTexture, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
        stateset->setTextureAttribute(1, mTexture, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);

        osg::Material* mat = static_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
        mat->setEmission(osg::Material::FRONT_AND_BACK, mEmissionColor);

        osg::TexEnvCombine* texEnvCombine = static_cast<osg::TexEnvCombine*>(stateset->getTextureAttribute(1, osg::StateAttribute::TEXENV));
        texEnvCombine->setConstantColor(osg::Vec4f(1,1,1,mOpacity));
    }

private:
    float mAnimationTimer;
    osg::ref_ptr<osg::Texture2D> mTexture;
    osg::Vec4f mEmissionColor;
    float mOpacity;
};

/// Transform that removes the eyepoint of the modelview matrix,
/// i.e. its children are positioned relative to the camera.
class CameraRelativeTransform : public osg::Transform
{
public:
    CameraRelativeTransform()
    {
        // Culling works in node-local space, not in camera space, so we can't cull this node correctly
        // That's not a problem though, children of this node can be culled just fine
        // Just make sure you do not place a CameraRelativeTransform deep in the scene graph
        setCullingActive(false);

        addCullCallback(new CullCallback);
    }

    CameraRelativeTransform(const CameraRelativeTransform& copy, const osg::CopyOp& copyop)
        : osg::Transform(copy, copyop)
    {
    }

    META_Node(MWRender, CameraRelativeTransform)

    const osg::Vec3f& getLastViewPoint() const
    {
        return mViewPoint;
    }

    virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
    {
        if (nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
        {
            mViewPoint = static_cast<osgUtil::CullVisitor*>(nv)->getViewPoint();
        }

        if (_referenceFrame==RELATIVE_RF)
        {
            matrix.setTrans(osg::Vec3f(0.f,0.f,0.f));
            return false;
        }
        else // absolute
        {
            matrix.makeIdentity();
            return true;
        }
    }

    osg::BoundingSphere computeBound() const
    {
        return osg::BoundingSphere(osg::Vec3f(0,0,0), 0);
    }

    class CullCallback : public osg::NodeCallback
    {
    public:
        virtual void operator() (osg::Node* node, osg::NodeVisitor* nv)
        {
            osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);

            // XXX have to remove unwanted culling plane of the water reflection camera

            // Remove all planes that aren't from the standard frustum
            unsigned int numPlanes = 4;
            if (cv->getCullingMode() & osg::CullSettings::NEAR_PLANE_CULLING)
                ++numPlanes;
            if (cv->getCullingMode() & osg::CullSettings::FAR_PLANE_CULLING)
                ++numPlanes;

            int mask = 0x1;
            int resultMask = cv->getProjectionCullingStack().back().getFrustum().getResultMask();
            for (unsigned int i=0; i<cv->getProjectionCullingStack().back().getFrustum().getPlaneList().size(); ++i)
            {
                if (i >= numPlanes)
                {
                    // turn off this culling plane
                    resultMask &= (~mask);
                }

                mask <<= 1;
            }

            cv->getProjectionCullingStack().back().getFrustum().setResultMask(resultMask);
            cv->getCurrentCullingSet().getFrustum().setResultMask(resultMask);

            cv->getProjectionCullingStack().back().pushCurrentMask();
            cv->getCurrentCullingSet().pushCurrentMask();

            traverse(node, nv);

            cv->getProjectionCullingStack().back().popCurrentMask();
            cv->getCurrentCullingSet().popCurrentMask();
        }
    };
private:
    // viewPoint for the current frame
    mutable osg::Vec3f mViewPoint;
};

class ModVertexAlphaVisitor : public osg::NodeVisitor
{
public:
    ModVertexAlphaVisitor(int meshType)
        : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        , mMeshType(meshType)
    {
    }

    void apply(osg::Drawable& drw)
    {
        osg::Geometry* geom = drw.asGeometry();
        if (!geom)
            return;

        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array(geom->getVertexArray()->getNumElements());
        for (unsigned int i=0; i<colors->size(); ++i)
        {
            float alpha = 1.f;
            if (mMeshType == 0) alpha = (i%2) ? 0.f : 1.f; // this is a cylinder, so every second vertex belongs to the bottom-most row
            else if (mMeshType == 1)
            {
                if (i>= 49 && i <= 64) alpha = 0.f; // bottom-most row
                else if (i>= 33 && i <= 48) alpha = 0.25098; // second row
                else alpha = 1.f;
            }
            else if (mMeshType == 2)
            {
                if (geom->getColorArray())
                {
                    osg::Vec4Array* origColors = static_cast<osg::Vec4Array*>(geom->getColorArray());
                    alpha = ((*origColors)[i].x() == 1.f) ? 1.f : 0.f;
                }
                else
                    alpha = 1.f;
            }

            (*colors)[i] = osg::Vec4f(0.f, 0.f, 0.f, alpha);
        }

        geom->setColorArray(colors, osg::Array::BIND_PER_VERTEX);
    }

private:
    int mMeshType;
};

/// @brief Hides the node subgraph if the eye point is below water.
/// @note Must be added as cull callback.
/// @note Meant to be used on a node that is child of a CameraRelativeTransform.
/// The current view point must be retrieved by the CameraRelativeTransform since we can't get it anymore once we are in camera-relative space.
class UnderwaterSwitchCallback : public osg::NodeCallback
{
public:
    UnderwaterSwitchCallback(CameraRelativeTransform* cameraRelativeTransform)
        : mCameraRelativeTransform(cameraRelativeTransform)
        , mEnabled(true)
        , mWaterLevel(0.f)
    {
    }

    bool isUnderwater()
    {
        osg::Vec3f viewPoint = mCameraRelativeTransform->getLastViewPoint();
        return mEnabled && viewPoint.z() < mWaterLevel;
    }

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        if (isUnderwater())
            return;

        traverse(node, nv);
    }

    void setEnabled(bool enabled)
    {
        mEnabled = enabled;
    }
    void setWaterLevel(float waterLevel)
    {
        mWaterLevel = waterLevel;
    }

private:
    osg::ref_ptr<CameraRelativeTransform> mCameraRelativeTransform;
    bool mEnabled;
    float mWaterLevel;
};

/// A base class for the sun and moons.
class CelestialBody
{
public:
    CelestialBody(osg::Group* parentNode, float scaleFactor, int numUvSets, unsigned int visibleMask=~0)
        : mVisibleMask(visibleMask)
    {
        mGeom = createTexturedQuad(numUvSets);
        mTransform = new osg::PositionAttitudeTransform;
        mTransform->setNodeMask(mVisibleMask);
        mTransform->setScale(osg::Vec3f(450,450,450) * scaleFactor);
        mTransform->addChild(mGeom);

        parentNode->addChild(mTransform);
    }

    virtual ~CelestialBody() {}

    virtual void adjustTransparency(const float ratio) = 0;

    void setVisible(bool visible)
    {
        mTransform->setNodeMask(visible ? mVisibleMask : 0);
    }

protected:
    unsigned int mVisibleMask;
    static const float mDistance;
    osg::ref_ptr<osg::PositionAttitudeTransform> mTransform;
    osg::ref_ptr<osg::Geometry> mGeom;
};

const float CelestialBody::mDistance = 1000.0f;

class Sun : public CelestialBody
{
public:
    Sun(osg::Group* parentNode, Resource::ImageManager& imageManager)
        : CelestialBody(parentNode, 1.0f, 1, Mask_Sun)
        , mUpdater(new Updater)
    {
        mTransform->addUpdateCallback(mUpdater);

        osg::ref_ptr<osg::Texture2D> sunTex (new osg::Texture2D(imageManager.getImage("textures/tx_sun_05.dds")));
        sunTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        sunTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

        mGeom->getOrCreateStateSet()->setTextureAttributeAndModes(0, sunTex, osg::StateAttribute::ON);

        osg::ref_ptr<osg::Group> queryNode (new osg::Group);
        // Need to render after the world geometry so we can correctly test for occlusions
        osg::StateSet* stateset = queryNode->getOrCreateStateSet();
        stateset->setRenderBinDetails(RenderBin_OcclusionQuery, "RenderBin");
        stateset->setNestRenderBins(false);
        // Set up alpha testing on the occlusion testing subgraph, that way we can get the occlusion tested fragments to match the circular shape of the sun
        osg::ref_ptr<osg::AlphaFunc> alphaFunc (new osg::AlphaFunc);
        alphaFunc->setFunction(osg::AlphaFunc::GREATER, 0.8);
        stateset->setAttributeAndModes(alphaFunc, osg::StateAttribute::ON);
        stateset->setTextureAttributeAndModes(0, sunTex, osg::StateAttribute::ON);
        stateset->setAttributeAndModes(createUnlitMaterial(), osg::StateAttribute::ON);
        // Disable writing to the color buffer. We are using this geometry for visibility tests only.
        osg::ref_ptr<osg::ColorMask> colormask (new osg::ColorMask(0, 0, 0, 0));
        stateset->setAttributeAndModes(colormask, osg::StateAttribute::ON);
        osg::ref_ptr<osg::PolygonOffset> po (new osg::PolygonOffset( -1., -1. ));
        stateset->setAttributeAndModes(po, osg::StateAttribute::ON);

        mTransform->addChild(queryNode);

        mOcclusionQueryVisiblePixels = createOcclusionQueryNode(queryNode, true);
        mOcclusionQueryTotalPixels = createOcclusionQueryNode(queryNode, false);

        createSunFlash(imageManager);
        createSunGlare();
    }

    ~Sun()
    {
        mTransform->removeUpdateCallback(mUpdater);
        destroySunFlash();
        destroySunGlare();
    }

    void setColor(const osg::Vec4f& color)
    {
        mUpdater->mColor.r() = color.r();
        mUpdater->mColor.g() = color.g();
        mUpdater->mColor.b() = color.b();
    }

    virtual void adjustTransparency(const float ratio)
    {
        mUpdater->mColor.a() = ratio;
        if (mSunGlareCallback)
            mSunGlareCallback->setGlareView(ratio);
        if (mSunFlashCallback)
            mSunFlashCallback->setGlareView(ratio);
    }

    void setDirection(const osg::Vec3f& direction)
    {
        osg::Vec3f normalizedDirection = direction / direction.length();
        mTransform->setPosition(normalizedDirection * mDistance);

        osg::Quat quat;
        quat.makeRotate(osg::Vec3f(0.0f, 0.0f, 1.0f), normalizedDirection);
        mTransform->setAttitude(quat);
    }

    void setGlareTimeOfDayFade(float val)
    {
        if (mSunGlareCallback)
            mSunGlareCallback->setTimeOfDayFade(val);
    }

private:
    class DummyComputeBoundCallback : public osg::Node::ComputeBoundingSphereCallback
    {
    public:
        virtual osg::BoundingSphere computeBound(const osg::Node& node) const { return osg::BoundingSphere(); }
    };

    /// @param queryVisible If true, queries the amount of visible pixels. If false, queries the total amount of pixels.
    osg::ref_ptr<osg::OcclusionQueryNode> createOcclusionQueryNode(osg::Group* parent, bool queryVisible)
    {
        osg::ref_ptr<osg::OcclusionQueryNode> oqn = new osg::OcclusionQueryNode;
        oqn->setQueriesEnabled(true);

#if OSG_VERSION_GREATER_OR_EQUAL(3, 6, 5)
        // With OSG 3.6.5, the method of providing user defined query geometry has been completely replaced
        osg::ref_ptr<osg::QueryGeometry> queryGeom = new osg::QueryGeometry(oqn->getName());
#else
        auto* queryGeom = oqn->getQueryGeometry();
#endif

        // Make it fast! A DYNAMIC query geometry means we can't break frame until the flare is rendered (which is rendered after all the other geometry,
        // so that would be pretty bad). STATIC should be safe, since our node's local bounds are static, thus computeBounds() which modifies the queryGeometry
        // is only called once.
        // Note the debug geometry setDebugDisplay(true) is always DYNAMIC and that can't be changed, not a big deal.
        queryGeom->setDataVariance(osg::Object::STATIC);

        // Set up the query geometry to match the actual sun's rendering shape. osg::OcclusionQueryNode wasn't originally intended to allow this,
        // normally it would automatically adjust the query geometry to match the sub graph's bounding box. The below hack is needed to
        // circumvent this.
        queryGeom->setVertexArray(mGeom->getVertexArray());
        queryGeom->setTexCoordArray(0, mGeom->getTexCoordArray(0), osg::Array::BIND_PER_VERTEX);
        queryGeom->removePrimitiveSet(0, oqn->getQueryGeometry()->getNumPrimitiveSets());
        queryGeom->addPrimitiveSet(mGeom->getPrimitiveSet(0));

        // Hack to disable unwanted awful code inside OcclusionQueryNode::computeBound.
        oqn->setComputeBoundingSphereCallback(new DummyComputeBoundCallback);
        // Still need a proper bounding sphere.
        oqn->setInitialBound(queryGeom->getBound());

#if OSG_VERSION_GREATER_OR_EQUAL(3, 6, 5)
        oqn->setQueryGeometry(queryGeom.release());
#endif

        osg::StateSet* queryStateSet = new osg::StateSet;
        if (queryVisible)
        {
            osg::ref_ptr<osg::Depth> depth (new osg::Depth);
            depth->setFunction(osg::Depth::LESS);
            // This is a trick to make fragments written by the query always use the maximum depth value,
            // without having to retrieve the current far clipping distance.
            // We want the sun glare to be "infinitely" far away.
            depth->setZNear(1.0);
            depth->setZFar(1.0);
            depth->setWriteMask(false);
            queryStateSet->setAttributeAndModes(depth, osg::StateAttribute::ON);
        }
        else
        {
            queryStateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
        }
        oqn->setQueryStateSet(queryStateSet);

        parent->addChild(oqn);

        return oqn;
    }

    void createSunFlash(Resource::ImageManager& imageManager)
    {
        osg::ref_ptr<osg::Texture2D> tex (new osg::Texture2D(imageManager.getImage("textures/tx_sun_flash_grey_05.dds")));
        tex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        tex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

        osg::ref_ptr<osg::PositionAttitudeTransform> transform (new osg::PositionAttitudeTransform);
        const float scale = 2.6f;
        transform->setScale(osg::Vec3f(scale,scale,scale));

        mTransform->addChild(transform);

        osg::ref_ptr<osg::Geometry> geom = createTexturedQuad();
        transform->addChild(geom);

        osg::StateSet* stateset = geom->getOrCreateStateSet();

        stateset->setTextureAttributeAndModes(0, tex, osg::StateAttribute::ON);
        stateset->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
        stateset->setRenderBinDetails(RenderBin_SunGlare, "RenderBin");
        stateset->setNestRenderBins(false);

        mSunFlashNode = transform;

        mSunFlashCallback = new SunFlashCallback(mOcclusionQueryVisiblePixels, mOcclusionQueryTotalPixels);
        mSunFlashNode->addCullCallback(mSunFlashCallback);
    }
    void destroySunFlash()
    {
        if (mSunFlashNode)
        {
            mSunFlashNode->removeCullCallback(mSunFlashCallback);
            mSunFlashCallback = nullptr;
        }
    }

    void createSunGlare()
    {
        osg::ref_ptr<osg::Camera> camera (new osg::Camera);
        camera->setProjectionMatrix(osg::Matrix::identity());
        camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF); // add to skyRoot instead?
        camera->setViewMatrix(osg::Matrix::identity());
        camera->setClearMask(0);
        camera->setRenderOrder(osg::Camera::NESTED_RENDER);
        camera->setAllowEventFocus(false);

        osg::ref_ptr<osg::Geometry> geom = osg::createTexturedQuadGeometry(osg::Vec3f(-1,-1,0), osg::Vec3f(2,0,0), osg::Vec3f(0,2,0));

        camera->addChild(geom);

        osg::StateSet* stateset = geom->getOrCreateStateSet();

        stateset->setRenderBinDetails(RenderBin_SunGlare, "RenderBin");
        stateset->setNestRenderBins(false);
        stateset->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);

        // set up additive blending
        osg::ref_ptr<osg::BlendFunc> blendFunc (new osg::BlendFunc);
        blendFunc->setSource(osg::BlendFunc::SRC_ALPHA);
        blendFunc->setDestination(osg::BlendFunc::ONE);
        stateset->setAttributeAndModes(blendFunc, osg::StateAttribute::ON);

        mSunGlareCallback = new SunGlareCallback(mOcclusionQueryVisiblePixels, mOcclusionQueryTotalPixels, mTransform);
        mSunGlareNode = camera;

        mSunGlareNode->addCullCallback(mSunGlareCallback);

        mTransform->addChild(camera);
    }
    void destroySunGlare()
    {
        if (mSunGlareNode)
        {
            mSunGlareNode->removeCullCallback(mSunGlareCallback);
            mSunGlareCallback = nullptr;
        }
    }

    class Updater : public SceneUtil::StateSetUpdater
    {
    public:
        osg::Vec4f mColor;

        Updater()
            : mColor(1.f, 1.f, 1.f, 1.f)
        {
        }

        virtual void setDefaults(osg::StateSet* stateset)
        {
            stateset->setAttributeAndModes(createUnlitMaterial(), osg::StateAttribute::ON);
        }

        virtual void apply(osg::StateSet* stateset, osg::NodeVisitor*)
        {
            osg::Material* mat = static_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
            mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0,0,0,mColor.a()));
            mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(mColor.r(), mColor.g(), mColor.b(), 1));
        }
    };

    class OcclusionCallback : public osg::NodeCallback
    {
    public:
        OcclusionCallback(osg::ref_ptr<osg::OcclusionQueryNode> oqnVisible, osg::ref_ptr<osg::OcclusionQueryNode> oqnTotal)
            : mOcclusionQueryVisiblePixels(oqnVisible)
            , mOcclusionQueryTotalPixels(oqnTotal)
        {
        }

    protected:
        float getVisibleRatio (osg::Camera* camera)
        {
            int visible = mOcclusionQueryVisiblePixels->getQueryGeometry()->getNumPixels(camera);
            int total = mOcclusionQueryTotalPixels->getQueryGeometry()->getNumPixels(camera);

            float visibleRatio = 0.f;
            if (total > 0)
                visibleRatio = static_cast<float>(visible) / static_cast<float>(total);

            float dt = MWBase::Environment::get().getFrameDuration();

            float lastRatio = mLastRatio[osg::observer_ptr<osg::Camera>(camera)];

            float change = dt*10;

            if (visibleRatio > lastRatio)
                visibleRatio = std::min(visibleRatio, lastRatio + change);
            else
                visibleRatio = std::max(visibleRatio, lastRatio - change);

            mLastRatio[osg::observer_ptr<osg::Camera>(camera)] = visibleRatio;

            return visibleRatio;
        }

    private:
        osg::ref_ptr<osg::OcclusionQueryNode> mOcclusionQueryVisiblePixels;
        osg::ref_ptr<osg::OcclusionQueryNode> mOcclusionQueryTotalPixels;

        std::map<osg::observer_ptr<osg::Camera>, float> mLastRatio;
    };

    /// SunFlashCallback handles fading/scaling of a node depending on occlusion query result. Must be attached as a cull callback.
    class SunFlashCallback : public OcclusionCallback
    {
    public:
        SunFlashCallback(osg::ref_ptr<osg::OcclusionQueryNode> oqnVisible, osg::ref_ptr<osg::OcclusionQueryNode> oqnTotal)
            : OcclusionCallback(oqnVisible, oqnTotal)
            , mGlareView(1.f)
        {
        }

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);

            float visibleRatio = getVisibleRatio(cv->getCurrentCamera());

            osg::ref_ptr<osg::StateSet> stateset;

            if (visibleRatio > 0.f)
            {
                const float fadeThreshold = 0.1;
                if (visibleRatio < fadeThreshold)
                {
                    float fade = 1.f - (fadeThreshold - visibleRatio) / fadeThreshold;
                    osg::ref_ptr<osg::Material> mat (createUnlitMaterial());
                    mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0,0,0,fade*mGlareView));
                    stateset = new osg::StateSet;
                    stateset->setAttributeAndModes(mat, osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
                }

                const float threshold = 0.6;
                visibleRatio = visibleRatio * (1.f - threshold) + threshold;
            }

            float scale = visibleRatio;

            if (scale == 0.f)
            {
                // no traverse
                return;
            }
            else
            {
                osg::Matrix modelView = *cv->getModelViewMatrix();

                modelView.preMultScale(osg::Vec3f(visibleRatio, visibleRatio, visibleRatio));

                if (stateset)
                    cv->pushStateSet(stateset);

                cv->pushModelViewMatrix(new osg::RefMatrix(modelView), osg::Transform::RELATIVE_RF);

                traverse(node, nv);

                cv->popModelViewMatrix();

                if (stateset)
                    cv->popStateSet();
            }
        }

        void setGlareView(float value)
        {
            mGlareView = value;
        }

    private:
        float mGlareView;
    };


    /// SunGlareCallback controls a full-screen glare effect depending on occlusion query result and the angle between sun and camera.
    /// Must be attached as a cull callback to the node above the glare node.
    class SunGlareCallback : public OcclusionCallback
    {
    public:
        SunGlareCallback(osg::ref_ptr<osg::OcclusionQueryNode> oqnVisible, osg::ref_ptr<osg::OcclusionQueryNode> oqnTotal,
                         osg::ref_ptr<osg::PositionAttitudeTransform> sunTransform)
            : OcclusionCallback(oqnVisible, oqnTotal)
            , mSunTransform(sunTransform)
            , mTimeOfDayFade(1.f)
            , mGlareView(1.f)
        {
            mColor = Fallback::Map::getColour("Weather_Sun_Glare_Fader_Color");
            mSunGlareFaderMax = Fallback::Map::getFloat("Weather_Sun_Glare_Fader_Max");
            mSunGlareFaderAngleMax = Fallback::Map::getFloat("Weather_Sun_Glare_Fader_Angle_Max");

            // Replicating a design flaw in MW. The color was being set on both ambient and emissive properties, which multiplies the result by two,
            // then finally gets clamped by the fixed function pipeline. With the default INI settings, only the red component gets clamped,
            // so the resulting color looks more orange than red.
            mColor *= 2;
            for (int i=0; i<3; ++i)
                mColor[i] = std::min(1.f, mColor[i]);
        }

        virtual void operator ()(osg::Node* node, osg::NodeVisitor* nv)
        {
            osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);

            float angleRadians = getAngleToSunInRadians(*cv->getCurrentRenderStage()->getInitialViewMatrix());
            float visibleRatio = getVisibleRatio(cv->getCurrentCamera());

            const float angleMaxRadians = osg::DegreesToRadians(mSunGlareFaderAngleMax);

            float value = 1.f - std::min(1.f, angleRadians / angleMaxRadians);
            float fade = value * mSunGlareFaderMax;

            fade *= mTimeOfDayFade * mGlareView * visibleRatio;

            if (fade == 0.f)
            {
                // no traverse
                return;
            }
            else
            {
                osg::ref_ptr<osg::StateSet> stateset (new osg::StateSet);

                osg::ref_ptr<osg::Material> mat (createUnlitMaterial());

                mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0,0,0,fade));
                mat->setEmission(osg::Material::FRONT_AND_BACK, mColor);

                stateset->setAttributeAndModes(mat, osg::StateAttribute::ON);

                cv->pushStateSet(stateset);
                traverse(node, nv);
                cv->popStateSet();
            }
        }

        void setTimeOfDayFade(float val)
        {
            mTimeOfDayFade = val;
        }

        void setGlareView(float glareView)
        {
            mGlareView = glareView;
        }

    private:
        float getAngleToSunInRadians(const osg::Matrix& viewMatrix) const
        {
            osg::Vec3d eye, center, up;
            viewMatrix.getLookAt(eye, center, up);

            osg::Vec3d forward = center - eye;
            osg::Vec3d sun = mSunTransform->getPosition();

            forward.normalize();
            sun.normalize();
            float angleRadians = std::acos(forward * sun);
            return angleRadians;
        }

        osg::ref_ptr<osg::PositionAttitudeTransform> mSunTransform;
        float mTimeOfDayFade;
        float mGlareView;
        osg::Vec4f mColor;
        float mSunGlareFaderMax;
        float mSunGlareFaderAngleMax;
    };

    osg::ref_ptr<Updater> mUpdater;
    osg::ref_ptr<SunFlashCallback> mSunFlashCallback;
    osg::ref_ptr<osg::Node> mSunFlashNode;
    osg::ref_ptr<SunGlareCallback> mSunGlareCallback;
    osg::ref_ptr<osg::Node> mSunGlareNode;
    osg::ref_ptr<osg::OcclusionQueryNode> mOcclusionQueryVisiblePixels;
    osg::ref_ptr<osg::OcclusionQueryNode> mOcclusionQueryTotalPixels;
};

class Moon : public CelestialBody
{
public:
    enum Type
    {
        Type_Masser = 0,
        Type_Secunda
    };

    Moon(osg::Group* parentNode, Resource::ImageManager& imageManager, float scaleFactor, Type type)
        : CelestialBody(parentNode, scaleFactor, 2)
        , mType(type)
        , mPhase(MoonState::Phase_Unspecified)
        , mUpdater(new Updater(imageManager))
    {
        setPhase(MoonState::Phase_Full);
        setVisible(true);

        mGeom->addUpdateCallback(mUpdater);
    }

    ~Moon()
    {
        mGeom->removeUpdateCallback(mUpdater);
    }

    virtual void adjustTransparency(const float ratio)
    {
        mUpdater->mTransparency *= ratio;
    }

    void setState(const MoonState& state)
    {
        float radsX = ((state.mRotationFromHorizon) * static_cast<float>(osg::PI)) / 180.0f;
        float radsZ = ((state.mRotationFromNorth) * static_cast<float>(osg::PI)) / 180.0f;

        osg::Quat rotX(radsX, osg::Vec3f(1.0f, 0.0f, 0.0f));
        osg::Quat rotZ(radsZ, osg::Vec3f(0.0f, 0.0f, 1.0f));

        osg::Vec3f direction = rotX * rotZ * osg::Vec3f(0.0f, 1.0f, 0.0f);
        mTransform->setPosition(direction * mDistance);

        // The moon quad is initially oriented facing down, so we need to offset its X-axis
        // rotation to rotate it to face the camera when sitting at the horizon.
        osg::Quat attX((-static_cast<float>(osg::PI) / 2.0f) + radsX, osg::Vec3f(1.0f, 0.0f, 0.0f));
        mTransform->setAttitude(attX * rotZ);

        setPhase(state.mPhase);
        mUpdater->mTransparency = state.mMoonAlpha;
        mUpdater->mShadowBlend = state.mShadowBlend;
    }

    void setAtmosphereColor(const osg::Vec4f& color)
    {
        mUpdater->mAtmosphereColor = color;
    }

    void setColor(const osg::Vec4f& color)
    {
        mUpdater->mMoonColor = color;
    }

    unsigned int getPhaseInt() const
    {
        if      (mPhase == MoonState::Phase_New)              return 0;
        else if (mPhase == MoonState::Phase_WaxingCrescent)   return 1;
        else if (mPhase == MoonState::Phase_WaningCrescent)   return 1;
        else if (mPhase == MoonState::Phase_FirstQuarter)     return 2;
        else if (mPhase == MoonState::Phase_ThirdQuarter)     return 2;
        else if (mPhase == MoonState::Phase_WaxingGibbous)    return 3;
        else if (mPhase == MoonState::Phase_WaningGibbous)    return 3;
        else if (mPhase == MoonState::Phase_Full)             return 4;
        return 0;
    }

private:
    struct Updater : public SceneUtil::StateSetUpdater
    {
        Resource::ImageManager& mImageManager;
        osg::ref_ptr<osg::Texture2D> mPhaseTex;
        osg::ref_ptr<osg::Texture2D> mCircleTex;
        float mTransparency;
        float mShadowBlend;
        osg::Vec4f mAtmosphereColor;
        osg::Vec4f mMoonColor;

        Updater(Resource::ImageManager& imageManager)
            : mImageManager(imageManager)
            , mPhaseTex()
            , mCircleTex()
            , mTransparency(1.0f)
            , mShadowBlend(1.0f)
            , mAtmosphereColor(1.0f, 1.0f, 1.0f, 1.0f)
            , mMoonColor(1.0f, 1.0f, 1.0f, 1.0f)
        {
        }

        virtual void setDefaults(osg::StateSet* stateset)
        {
            stateset->setTextureAttributeAndModes(0, mPhaseTex, osg::StateAttribute::ON);
            osg::ref_ptr<osg::TexEnvCombine> texEnv = new osg::TexEnvCombine;
            texEnv->setCombine_RGB(osg::TexEnvCombine::MODULATE);
            texEnv->setSource0_RGB(osg::TexEnvCombine::CONSTANT);
            texEnv->setSource1_RGB(osg::TexEnvCombine::TEXTURE);
            texEnv->setConstantColor(osg::Vec4f(1.f, 0.f, 0.f, 1.f)); // mShadowBlend * mMoonColor
            stateset->setTextureAttributeAndModes(0, texEnv, osg::StateAttribute::ON);

            stateset->setTextureAttributeAndModes(1, mCircleTex, osg::StateAttribute::ON);
            osg::ref_ptr<osg::TexEnvCombine> texEnv2 = new osg::TexEnvCombine;
            texEnv2->setCombine_RGB(osg::TexEnvCombine::ADD);
            texEnv2->setCombine_Alpha(osg::TexEnvCombine::MODULATE);
            texEnv2->setSource0_Alpha(osg::TexEnvCombine::TEXTURE);
            texEnv2->setSource1_Alpha(osg::TexEnvCombine::CONSTANT);
            texEnv2->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
            texEnv2->setSource1_RGB(osg::TexEnvCombine::CONSTANT);
            texEnv2->setConstantColor(osg::Vec4f(0.f, 0.f, 0.f, 1.f)); // mAtmosphereColor.rgb, mTransparency
            stateset->setTextureAttributeAndModes(1, texEnv2, osg::StateAttribute::ON);

            stateset->setAttributeAndModes(createUnlitMaterial(), osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
        }

        virtual void apply(osg::StateSet* stateset, osg::NodeVisitor*)
        {
            osg::TexEnvCombine* texEnv = static_cast<osg::TexEnvCombine*>(stateset->getTextureAttribute(0, osg::StateAttribute::TEXENV));
            texEnv->setConstantColor(mMoonColor * mShadowBlend);

            osg::TexEnvCombine* texEnv2 = static_cast<osg::TexEnvCombine*>(stateset->getTextureAttribute(1, osg::StateAttribute::TEXENV));
            texEnv2->setConstantColor(osg::Vec4f(mAtmosphereColor.x(), mAtmosphereColor.y(), mAtmosphereColor.z(), mTransparency));
        }

        void setTextures(const std::string& phaseTex, const std::string& circleTex)
        {
            mPhaseTex = new osg::Texture2D(mImageManager.getImage(phaseTex));
            mPhaseTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            mPhaseTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            mCircleTex = new osg::Texture2D(mImageManager.getImage(circleTex));
            mCircleTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            mCircleTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

            reset();
        }
    };

    Type mType;
    MoonState::Phase mPhase;
    osg::ref_ptr<Updater> mUpdater;

    void setPhase(const MoonState::Phase& phase)
    {
        if(mPhase == phase)
            return;

        mPhase = phase;

        std::string textureName = "textures/tx_";

        if (mType == Moon::Type_Secunda)
            textureName += "secunda_";
        else
            textureName += "masser_";

        if     (phase == MoonState::Phase_New)            textureName += "new";
        else if(phase == MoonState::Phase_WaxingCrescent) textureName += "one_wax";
        else if(phase == MoonState::Phase_FirstQuarter)   textureName += "half_wax";
        else if(phase == MoonState::Phase_WaxingGibbous)  textureName += "three_wax";
        else if(phase == MoonState::Phase_WaningCrescent) textureName += "one_wan";
        else if(phase == MoonState::Phase_ThirdQuarter)   textureName += "half_wan";
        else if(phase == MoonState::Phase_WaningGibbous)  textureName += "three_wan";
        else if(phase == MoonState::Phase_Full)           textureName += "full";

        textureName += ".dds";

        if (mType == Moon::Type_Secunda)
            mUpdater->setTextures(textureName, "textures/tx_mooncircle_full_s.dds");
        else
            mUpdater->setTextures(textureName, "textures/tx_mooncircle_full_m.dds");
    }
};

SkyManager::SkyManager(osg::Group* parentNode, Resource::SceneManager* sceneManager)
    : mSceneManager(sceneManager)
    , mCamera(nullptr)
    , mRainIntensityUniform(nullptr)
    , mAtmosphereNightRoll(0.f)
    , mCreated(false)
    , mIsStorm(false)
    , mDay(0)
    , mMonth(0)
    , mCloudAnimationTimer(0.f)
    , mRainTimer(0.f)
    , mStormDirection(0,1,0)
    , mClouds()
    , mNextClouds()
    , mCloudBlendFactor(0.0f)
    , mCloudSpeed(0.0f)
    , mStarsOpacity(0.0f)
    , mRemainingTransitionTime(0.0f)
    , mRainEnabled(false)
    , mRainSpeed(0)
    , mRainDiameter(0)
    , mRainMinHeight(0)
    , mRainMaxHeight(0)
    , mRainEntranceSpeed(1)
    , mRainMaxRaindrops(0)
    , mWindSpeed(0.f)
    , mEnabled(true)
    , mSunEnabled(true)
    , mWeatherAlpha(0.f)
{
    osg::ref_ptr<CameraRelativeTransform> skyroot (new CameraRelativeTransform);
    skyroot->setName("Sky Root");
    // Assign empty program to specify we don't want shaders
    // The shaders generated by the SceneManager can't handle everything we need
    skyroot->getOrCreateStateSet()->setAttributeAndModes(new osg::Program(), osg::StateAttribute::OVERRIDE|osg::StateAttribute::PROTECTED|osg::StateAttribute::ON);
    SceneUtil::ShadowManager::disableShadowsForStateSet(skyroot->getOrCreateStateSet());

    skyroot->setNodeMask(Mask_Sky);
    parentNode->addChild(skyroot);

    mRootNode = skyroot;

    mEarlyRenderBinRoot = new osg::Group;
    // render before the world is rendered
    mEarlyRenderBinRoot->getOrCreateStateSet()->setRenderBinDetails(RenderBin_Sky, "RenderBin");
    // Prevent unwanted clipping by water reflection camera's clipping plane
    mEarlyRenderBinRoot->getOrCreateStateSet()->setMode(GL_CLIP_PLANE0, osg::StateAttribute::OFF);
    mRootNode->addChild(mEarlyRenderBinRoot);

    mUnderwaterSwitch = new UnderwaterSwitchCallback(skyroot);
}

void SkyManager::setRainIntensityUniform(osg::Uniform *uniform)
{
    mRainIntensityUniform = uniform;
}

void SkyManager::create()
{
    assert(!mCreated);

    mAtmosphereDay = mSceneManager->getInstance("meshes/sky_atmosphere.nif", mEarlyRenderBinRoot);
    ModVertexAlphaVisitor modAtmosphere(0);
    mAtmosphereDay->accept(modAtmosphere);

    mAtmosphereUpdater = new AtmosphereUpdater;
    mAtmosphereDay->addUpdateCallback(mAtmosphereUpdater);

    mAtmosphereNightNode = new osg::PositionAttitudeTransform;
    mAtmosphereNightNode->setNodeMask(0);
    mEarlyRenderBinRoot->addChild(mAtmosphereNightNode);

    osg::ref_ptr<osg::Node> atmosphereNight;
    if (mSceneManager->getVFS()->exists("meshes/sky_night_02.nif"))
        atmosphereNight = mSceneManager->getInstance("meshes/sky_night_02.nif", mAtmosphereNightNode);
    else
        atmosphereNight = mSceneManager->getInstance("meshes/sky_night_01.nif", mAtmosphereNightNode);
    atmosphereNight->getOrCreateStateSet()->setAttributeAndModes(createAlphaTrackingUnlitMaterial(), osg::StateAttribute::ON|osg::StateAttribute::OVERRIDE);
    ModVertexAlphaVisitor modStars(2);
    atmosphereNight->accept(modStars);
    mAtmosphereNightUpdater = new AtmosphereNightUpdater(mSceneManager->getImageManager());
    atmosphereNight->addUpdateCallback(mAtmosphereNightUpdater);

    mSun.reset(new Sun(mEarlyRenderBinRoot, *mSceneManager->getImageManager()));

    mMasser.reset(new Moon(mEarlyRenderBinRoot, *mSceneManager->getImageManager(), Fallback::Map::getFloat("Moons_Masser_Size")/125, Moon::Type_Masser));
    mSecunda.reset(new Moon(mEarlyRenderBinRoot, *mSceneManager->getImageManager(), Fallback::Map::getFloat("Moons_Secunda_Size")/125, Moon::Type_Secunda));

    mCloudNode = new osg::PositionAttitudeTransform;
    mEarlyRenderBinRoot->addChild(mCloudNode);
    mCloudMesh = mSceneManager->getInstance("meshes/sky_clouds_01.nif", mCloudNode);
    ModVertexAlphaVisitor modClouds(1);
    mCloudMesh->accept(modClouds);
    mCloudUpdater = new CloudUpdater;
    mCloudUpdater->setOpacity(1.f);
    mCloudMesh->addUpdateCallback(mCloudUpdater);

    mCloudMesh2 = mSceneManager->getInstance("meshes/sky_clouds_01.nif", mCloudNode);
    mCloudMesh2->accept(modClouds);
    mCloudUpdater2 = new CloudUpdater;
    mCloudUpdater2->setOpacity(0.f);
    mCloudMesh2->addUpdateCallback(mCloudUpdater2);
    mCloudMesh2->setNodeMask(0);

    osg::ref_ptr<osg::Depth> depth = new osg::Depth;
    depth->setWriteMask(false);
    mEarlyRenderBinRoot->getOrCreateStateSet()->setAttributeAndModes(depth, osg::StateAttribute::ON);
    mEarlyRenderBinRoot->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    mEarlyRenderBinRoot->getOrCreateStateSet()->setMode(GL_FOG, osg::StateAttribute::OFF);

    mMoonScriptColor = Fallback::Map::getColour("Moons_Script_Color");

    mCreated = true;
}

class RainCounter : public osgParticle::ConstantRateCounter
{
public:
    virtual int numParticlesToCreate(double dt) const
    {
        // limit dt to avoid large particle emissions if there are jumps in the simulation time
        // 0.2 seconds is the same cap as used in Engine's frame loop
        dt = std::min(dt, 0.2);
        return ConstantRateCounter::numParticlesToCreate(dt);
    }
};

class RainShooter : public osgParticle::Shooter
{
public:
    RainShooter()
        : mAngle(0.f)
    {
    }

    virtual void shoot(osgParticle::Particle* particle) const
    {
        particle->setVelocity(mVelocity);
        particle->setAngle(osg::Vec3f(-mAngle, 0, (Misc::Rng::rollProbability() * 2 - 1) * osg::PI));
    }

    void setVelocity(const osg::Vec3f& velocity)
    {
        mVelocity = velocity;
    }

    void setAngle(float angle)
    {
        mAngle = angle;
    }

    virtual osg::Object* cloneType() const
    {
        return new RainShooter;
    }
    virtual osg::Object* clone(const osg::CopyOp &) const
    {
        return new RainShooter(*this);
    }

private:
    osg::Vec3f mVelocity;
    float mAngle;
};

// Updater for alpha value on a node's StateSet. Assumes the node has an existing Material StateAttribute.
class AlphaFader : public SceneUtil::StateSetUpdater
{
public:
    /// @param alphaUpdate variable which to update with alpha value
    AlphaFader(float *alphaUpdate)
        : mAlpha(1.f)
    {
        mAlphaUpdate = alphaUpdate;
    }

    void setAlpha(float alpha)
    {
        mAlpha = alpha;
    }

    virtual void setDefaults(osg::StateSet* stateset)
    {
        // need to create a deep copy of StateAttributes we will modify
        osg::Material* mat = static_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
        stateset->setAttribute(osg::clone(mat, osg::CopyOp::DEEP_COPY_ALL), osg::StateAttribute::ON);
    }

    virtual void apply(osg::StateSet* stateset, osg::NodeVisitor* nv)
    {
        osg::Material* mat = static_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
        mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0,0,0,mAlpha));

        if (mAlphaUpdate)
            *mAlphaUpdate = mAlpha;
    }

    // Helper for adding AlphaFaders to a subgraph
    class SetupVisitor : public osg::NodeVisitor
    {
    public:
        SetupVisitor(float *alphaUpdate)
            : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        {
            mAlphaUpdate = alphaUpdate;
        }

        virtual void apply(osg::Node &node)
        {
            if (osg::StateSet* stateset = node.getStateSet())
            {
                if (stateset->getAttribute(osg::StateAttribute::MATERIAL))
                {
                    SceneUtil::CompositeStateSetUpdater* composite = nullptr;
                    osg::Callback* callback = node.getUpdateCallback();

                    while (callback)
                    {
                        if ((composite = dynamic_cast<SceneUtil::CompositeStateSetUpdater*>(callback)))
                            break;

                        callback = callback->getNestedCallback();
                    }

                    osg::ref_ptr<AlphaFader> alphaFader (new AlphaFader(mAlphaUpdate));

                    if (composite)
                        composite->addController(alphaFader);
                    else
                        node.addUpdateCallback(alphaFader);

                    mAlphaFaders.push_back(alphaFader);
                }
            }

            traverse(node);
        }

        std::vector<osg::ref_ptr<AlphaFader> > getAlphaFaders()
        {
            return mAlphaFaders;
        }

    private:
        std::vector<osg::ref_ptr<AlphaFader> > mAlphaFaders;
        float *mAlphaUpdate;
    };

protected:
    float mAlpha;
    float *mAlphaUpdate;
};

class RainFader : public AlphaFader
{
public:
    RainFader(float *alphaUpdate): AlphaFader(alphaUpdate)
    {
    }

    virtual void setDefaults(osg::StateSet* stateset)
    {
        osg::ref_ptr<osg::Material> mat (new osg::Material);
        mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,1));
        mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(0,0,0,1));
        mat->setColorMode(osg::Material::OFF);
        stateset->setAttributeAndModes(mat, osg::StateAttribute::ON);
    }

    virtual void apply(osg::StateSet *stateset, osg::NodeVisitor *nv)
    {
        AlphaFader::apply(stateset,nv);
        *mAlphaUpdate = mAlpha * 2.0;  // mAlpha is limited to 0.6 so multiply by 2 to reach full intensity
    }
};

void SkyManager::setCamera(osg::Camera *camera)
{
    mCamera = camera;
}

class WrapAroundOperator : public osgParticle::Operator
{
public:
    WrapAroundOperator(osg::Camera *camera, const osg::Vec3 &wrapRange): osgParticle::Operator()
    {
        mCamera = camera;
        mWrapRange = wrapRange;
        mHalfWrapRange = mWrapRange / 2.0;
        mPreviousCameraPosition = getCameraPosition();
    }

    virtual osg::Object *cloneType() const override
    {
        return nullptr;
    }

    virtual osg::Object *clone(const osg::CopyOp &op) const override
    {
        return nullptr;
    }

    virtual void operate(osgParticle::Particle *P, double dt) override
    {
    }

    virtual void operateParticles(osgParticle::ParticleSystem *ps, double dt) override
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
            osgParticle::Particle *p = ps->getParticle(i);
            p->setPosition(toWorld.preMult(p->getPosition()));
            p->setPosition(p->getPosition() - positionDifference);

            for (int j = 0; j < 3; ++j)  // wrap-around in all 3 dimensions
            {
                osg::Vec3 pos = p->getPosition();

                if (pos[j] < -mHalfWrapRange[j])
                    pos[j] = mHalfWrapRange[j] + fmod(pos[j] - mHalfWrapRange[j],mWrapRange[j]);
                else if (pos[j] > mHalfWrapRange[j])
                    pos[j] = fmod(pos[j] + mHalfWrapRange[j],mWrapRange[j]) - mHalfWrapRange[j];

                p->setPosition(pos);
            }

            p->setPosition(toLocal.preMult(p->getPosition()));
        }

        mPreviousCameraPosition = position;
    }

protected:
    osg::Camera *mCamera;
    osg::Vec3 mPreviousCameraPosition;
    osg::Vec3 mWrapRange;
    osg::Vec3 mHalfWrapRange;

    osg::Vec3 getCameraPosition()
    {
        return mCamera->getInverseViewMatrix().getTrans();
    }
};

void SkyManager::createRain()
{
    if (mRainNode)
        return;

    mRainNode = new osg::Group;

    mRainParticleSystem = new osgParticle::ParticleSystem;
    osg::Vec3 rainRange = osg::Vec3(mRainDiameter, mRainDiameter, (mRainMinHeight+mRainMaxHeight)/2.f);

    mRainParticleSystem->setParticleAlignment(osgParticle::ParticleSystem::FIXED);
    mRainParticleSystem->setAlignVectorX(osg::Vec3f(0.1,0,0));
    mRainParticleSystem->setAlignVectorY(osg::Vec3f(0,0,1));

    osg::ref_ptr<osg::StateSet> stateset (mRainParticleSystem->getOrCreateStateSet());

    osg::ref_ptr<osg::Texture2D> raindropTex (new osg::Texture2D(mSceneManager->getImageManager()->getImage("textures/tx_raindrop_01.dds")));
    raindropTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    raindropTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

    stateset->setTextureAttributeAndModes(0, raindropTex, osg::StateAttribute::ON);
    stateset->setNestRenderBins(false);
    stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    stateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    stateset->setMode(GL_BLEND, osg::StateAttribute::ON);

    osgParticle::Particle& particleTemplate = mRainParticleSystem->getDefaultParticleTemplate();
    particleTemplate.setSizeRange(osgParticle::rangef(5.f, 15.f));
    particleTemplate.setAlphaRange(osgParticle::rangef(1.f, 1.f));
    particleTemplate.setLifeTime(1);

    osg::ref_ptr<osgParticle::ModularEmitter> emitter (new osgParticle::ModularEmitter);
    emitter->setParticleSystem(mRainParticleSystem);

    osg::ref_ptr<osgParticle::BoxPlacer> placer (new osgParticle::BoxPlacer);
    placer->setXRange(-rainRange.x() / 2, rainRange.x() / 2);
    placer->setYRange(-rainRange.y() / 2, rainRange.y() / 2);
    placer->setZRange(-rainRange.z() / 2, rainRange.z() / 2);
    emitter->setPlacer(placer);
    mPlacer = placer;

    // FIXME: vanilla engine does not use a particle system to handle rain, it uses a NIF-file with 20 raindrops in it.
    // It spawns the (maxRaindrops-getParticleSystem()->numParticles())*dt/rainEntranceSpeed batches every frame (near 1-2).
    // Since the rain is a regular geometry, it produces water ripples, also in theory it can be removed if collides with something.
    osg::ref_ptr<RainCounter> counter (new RainCounter);
    counter->setNumberOfParticlesPerSecondToCreate(mRainMaxRaindrops/mRainEntranceSpeed*20);
    emitter->setCounter(counter);
    mCounter = counter;

    osg::ref_ptr<RainShooter> shooter (new RainShooter);
    mRainShooter = shooter;
    emitter->setShooter(shooter);

    osg::ref_ptr<osgParticle::ParticleSystemUpdater> updater (new osgParticle::ParticleSystemUpdater);
    updater->addParticleSystem(mRainParticleSystem);

    osg::ref_ptr<osgParticle::ModularProgram> program (new osgParticle::ModularProgram);
    program->addOperator(new WrapAroundOperator(mCamera,rainRange));
    program->setParticleSystem(mRainParticleSystem);
    mRainNode->addChild(program);

    mRainNode->addChild(emitter);
    mRainNode->addChild(mRainParticleSystem);
    mRainNode->addChild(updater);

    mRainFader = new RainFader(&mWeatherAlpha);
    mRainNode->addUpdateCallback(mRainFader);
    mRainNode->addCullCallback(mUnderwaterSwitch);
    mRainNode->setNodeMask(Mask_WeatherParticles);

    mRootNode->addChild(mRainNode);
}

void SkyManager::destroyRain()
{
    if (!mRainNode)
        return;

    mRootNode->removeChild(mRainNode);
    mRainNode = nullptr;
    mPlacer = nullptr;
    mCounter = nullptr;
    mRainParticleSystem = nullptr;
    mRainShooter = nullptr;
    mRainFader = nullptr;
}

SkyManager::~SkyManager()
{
    if (mRootNode)
    {
        mRootNode->getParent(0)->removeChild(mRootNode);
        mRootNode = nullptr;
    }
}

int SkyManager::getMasserPhase() const
{
    if (!mCreated) return 0;
    return mMasser->getPhaseInt();
}

int SkyManager::getSecundaPhase() const
{
    if (!mCreated) return 0;
    return mSecunda->getPhaseInt();
}

bool SkyManager::isEnabled()
{
    return mEnabled;
}

bool SkyManager::hasRain()
{
    return mRainNode != nullptr;
}

void SkyManager::update(float duration)
{
    if (!mEnabled)
    {
        if (mRainIntensityUniform)
            mRainIntensityUniform->set((float) 0.0);

        return;
    }

    if (mRainIntensityUniform)
    {
        if (mIsStorm || (!hasRain() && !mParticleNode))
            mRainIntensityUniform->set((float) 0.0);
        else
            mRainIntensityUniform->set((float) mWeatherAlpha);
    }

    switchUnderwaterRain();

    if (mIsStorm)
    {
        osg::Quat quat;
        quat.makeRotate(osg::Vec3f(0,1,0), mStormDirection);

        mCloudNode->setAttitude(quat);
        if (mParticleNode)
        {
            // Morrowind deliberately rotates the blizzard mesh, so so should we.
            if (mCurrentParticleEffect == "meshes\\blizzard.nif")
                quat.makeRotate(osg::Vec3f(-1,0,0), mStormDirection);
            mParticleNode->setAttitude(quat);
        }
    }
    else
        mCloudNode->setAttitude(osg::Quat());

    // UV Scroll the clouds
    mCloudAnimationTimer += duration * mCloudSpeed * 0.003;
    mCloudUpdater->setAnimationTimer(mCloudAnimationTimer);
    mCloudUpdater2->setAnimationTimer(mCloudAnimationTimer);

    // rotate the stars by 360 degrees every 4 days
    mAtmosphereNightRoll += MWBase::Environment::get().getWorld()->getTimeScaleFactor()*duration*osg::DegreesToRadians(360.f) / (3600*96.f);
    if (mAtmosphereNightNode->getNodeMask() != 0)
        mAtmosphereNightNode->setAttitude(osg::Quat(mAtmosphereNightRoll, osg::Vec3f(0,0,1)));
}

void SkyManager::setEnabled(bool enabled)
{
    if (enabled && !mCreated)
        create();

    mRootNode->setNodeMask(enabled ? Mask_Sky : 0);

    mEnabled = enabled;
}

void SkyManager::setMoonColour (bool red)
{
    if (!mCreated) return;
    mSecunda->setColor(red ? mMoonScriptColor : osg::Vec4f(1,1,1,1));
}

void SkyManager::updateRainParameters()
{
    if (mRainShooter)
    {
        float angle = -std::atan(mWindSpeed/50.f);
        mRainShooter->setVelocity(osg::Vec3f(0, mRainSpeed*std::sin(angle), -mRainSpeed/std::cos(angle)));
        mRainShooter->setAngle(angle);

        osg::Vec3 rainRange = osg::Vec3(mRainDiameter, mRainDiameter, (mRainMinHeight+mRainMaxHeight)/2.f);

        mPlacer->setXRange(-rainRange.x() / 2, rainRange.x() / 2);
        mPlacer->setYRange(-rainRange.y() / 2, rainRange.y() / 2);
        mPlacer->setZRange(-rainRange.z() / 2, rainRange.z() / 2);

        mCounter->setNumberOfParticlesPerSecondToCreate(mRainMaxRaindrops/mRainEntranceSpeed*20);
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
    if (!mCreated) return;

    mRainEntranceSpeed = weather.mRainEntranceSpeed;
    mRainMaxRaindrops = weather.mRainMaxRaindrops;
    mRainDiameter = weather.mRainDiameter;
    mRainMinHeight = weather.mRainMinHeight;
    mRainMaxHeight = weather.mRainMaxHeight;
    mRainSpeed = weather.mRainSpeed;
    mWindSpeed = weather.mWindSpeed;

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

    if (mCurrentParticleEffect != weather.mParticleEffect)
    {
        mCurrentParticleEffect = weather.mParticleEffect;

        // cleanup old particles
        if (mParticleEffect)
        {
            mParticleNode->removeChild(mParticleEffect);
            mParticleEffect = nullptr;
            mParticleFaders.clear();
        }

        if (mCurrentParticleEffect.empty())
        {
            if (mParticleNode)
            {
                mRootNode->removeChild(mParticleNode);
                mParticleNode = nullptr;
            }
        }
        else
        {
            if (!mParticleNode)
            {
                mParticleNode = new osg::PositionAttitudeTransform;
                mParticleNode->addCullCallback(mUnderwaterSwitch);
                mParticleNode->setNodeMask(Mask_WeatherParticles);
                mRootNode->addChild(mParticleNode);
            }

            mParticleEffect = mSceneManager->getInstance(mCurrentParticleEffect, mParticleNode);

            SceneUtil::AssignControllerSourcesVisitor assignVisitor(std::shared_ptr<SceneUtil::ControllerSource>(new SceneUtil::FrameTimeSource));
            mParticleEffect->accept(assignVisitor);

            AlphaFader::SetupVisitor alphaFaderSetupVisitor(&mWeatherAlpha);

            mParticleEffect->accept(alphaFaderSetupVisitor);
            mParticleFaders = alphaFaderSetupVisitor.getAlphaFaders();

            SceneUtil::DisableFreezeOnCullVisitor disableFreezeOnCullVisitor;
            mParticleEffect->accept(disableFreezeOnCullVisitor);

            if (!weather.mIsStorm)
            {
                SceneUtil::FindByClassVisitor findPSVisitor(std::string("ParticleSystem"));
                mParticleEffect->accept(findPSVisitor);

                for (unsigned int i = 0; i < findPSVisitor.mFoundNodes.size(); ++i)
                {
                    osgParticle::ParticleSystem *ps = static_cast<osgParticle::ParticleSystem *>(findPSVisitor.mFoundNodes[i]);
                    
                    osg::ref_ptr<osgParticle::ModularProgram> program (new osgParticle::ModularProgram);
                    program->addOperator(new WrapAroundOperator(mCamera,osg::Vec3(1024,1024,800)));
                    program->setParticleSystem(ps);
                    mParticleNode->addChild(program);
                }
            }
        }
    }

    if (mClouds != weather.mCloudTexture)
    {
        mClouds = weather.mCloudTexture;

        std::string texture = Misc::ResourceHelpers::correctTexturePath(mClouds, mSceneManager->getVFS());

        osg::ref_ptr<osg::Texture2D> cloudTex (new osg::Texture2D(mSceneManager->getImageManager()->getImage(texture)));
        cloudTex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
        cloudTex->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);

        mCloudUpdater->setTexture(cloudTex);
    }

    if (mNextClouds != weather.mNextCloudTexture)
    {
        mNextClouds = weather.mNextCloudTexture;

        if (!mNextClouds.empty())
        {
            std::string texture = Misc::ResourceHelpers::correctTexturePath(mNextClouds, mSceneManager->getVFS());

            osg::ref_ptr<osg::Texture2D> cloudTex (new osg::Texture2D(mSceneManager->getImageManager()->getImage(texture)));
            cloudTex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
            cloudTex->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);

            mCloudUpdater2->setTexture(cloudTex);
        }
    }

    if (mCloudBlendFactor != weather.mCloudBlendFactor)
    {
        mCloudBlendFactor = weather.mCloudBlendFactor;

        mCloudUpdater->setOpacity((1.f-mCloudBlendFactor));
        mCloudUpdater2->setOpacity(mCloudBlendFactor);
        mCloudMesh2->setNodeMask(mCloudBlendFactor > 0.f ? ~0 : 0);
    }

    if (mCloudColour != weather.mFogColor)
    {
        osg::Vec4f clr (weather.mFogColor);
        clr += osg::Vec4f(0.13f, 0.13f, 0.13f, 0.f);

        mCloudUpdater->setEmissionColor(clr);
        mCloudUpdater2->setEmissionColor(clr);

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

    mAtmosphereNightNode->setNodeMask(weather.mNight ? ~0 : 0);

    if (mRainFader)
        mRainFader->setAlpha(weather.mEffectFade * 0.6); // * Rain_Threshold?

    for (AlphaFader* fader : mParticleFaders)
        fader->setAlpha(weather.mEffectFade);
}

void SkyManager::sunEnable()
{
    if (!mCreated) return;

    mSun->setVisible(true);
}

void SkyManager::sunDisable()
{
    if (!mCreated) return;

    mSun->setVisible(false);
}

void SkyManager::setStormDirection(const osg::Vec3f &direction)
{
    mStormDirection = direction;
}

void SkyManager::setSunDirection(const osg::Vec3f& direction)
{
    if (!mCreated) return;

    mSun->setDirection(direction);
}

void SkyManager::setMasserState(const MoonState& state)
{
    if(!mCreated) return;

    mMasser->setState(state);
}

void SkyManager::setSecundaState(const MoonState& state)
{
    if(!mCreated) return;

    mSecunda->setState(state);
}

void SkyManager::setDate(int day, int month)
{
    mDay = day;
    mMonth = month;
}

void SkyManager::setGlareTimeOfDayFade(float val)
{
    mSun->setGlareTimeOfDayFade(val);
}

void SkyManager::setWaterHeight(float height)
{
    mUnderwaterSwitch->setWaterLevel(height);
}

void SkyManager::listAssetsToPreload(std::vector<std::string>& models, std::vector<std::string>& textures)
{
    models.push_back("meshes/sky_atmosphere.nif");
    if (mSceneManager->getVFS()->exists("meshes/sky_night_02.nif"))
        models.push_back("meshes/sky_night_02.nif");
    models.push_back("meshes/sky_night_01.nif");
    models.push_back("meshes/sky_clouds_01.nif");

    models.push_back("meshes\\ashcloud.nif");
    models.push_back("meshes\\blightcloud.nif");
    models.push_back("meshes\\snow.nif");
    models.push_back("meshes\\blizzard.nif");

    textures.push_back("textures/tx_mooncircle_full_s.dds");
    textures.push_back("textures/tx_mooncircle_full_m.dds");

    textures.push_back("textures/tx_masser_new.dds");
    textures.push_back("textures/tx_masser_one_wax.dds");
    textures.push_back("textures/tx_masser_half_wax.dds");
    textures.push_back("textures/tx_masser_three_wax.dds");
    textures.push_back("textures/tx_masser_one_wan.dds");
    textures.push_back("textures/tx_masser_half_wan.dds");
    textures.push_back("textures/tx_masser_three_wan.dds");
    textures.push_back("textures/tx_masser_full.dds");

    textures.push_back("textures/tx_secunda_new.dds");
    textures.push_back("textures/tx_secunda_one_wax.dds");
    textures.push_back("textures/tx_secunda_half_wax.dds");
    textures.push_back("textures/tx_secunda_three_wax.dds");
    textures.push_back("textures/tx_secunda_one_wan.dds");
    textures.push_back("textures/tx_secunda_half_wan.dds");
    textures.push_back("textures/tx_secunda_three_wan.dds");
    textures.push_back("textures/tx_secunda_full.dds");

    textures.push_back("textures/tx_sun_05.dds");
    textures.push_back("textures/tx_sun_flash_grey_05.dds");

    textures.push_back("textures/tx_raindrop_01.dds");
}

void SkyManager::setWaterEnabled(bool enabled)
{
    mUnderwaterSwitch->setEnabled(enabled);
}

}
