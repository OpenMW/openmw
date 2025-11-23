#include "skyutil.hpp"

#include <array>
#include <cmath>

#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/ColorMask>
#include <osg/Depth>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/OcclusionQueryNode>
#include <osg/PositionAttitudeTransform>
#include <osg/TexEnvCombine>
#include <osg/TexMat>
#include <osg/Transform>
#include <osg/observer_ptr>

#include <osgUtil/CullVisitor>

#include <osgParticle/Particle>

#include <components/misc/rng.hpp>
#include <components/stereo/multiview.hpp>
#include <components/stereo/stereomanager.hpp>

#include <components/resource/imagemanager.hpp>
#include <components/resource/scenemanager.hpp>

#include <components/sceneutil/depth.hpp>
#include <components/sceneutil/texturetype.hpp>

#include <components/fallback/fallback.hpp>

#include <components/sceneutil/statesetupdater.hpp>

#include "../mwbase/environment.hpp"

#include "renderbin.hpp"
#include "vismask.hpp"

namespace
{
    enum class Pass
    {
        Atmosphere,
        Atmosphere_Night,
        Clouds,
        Moon,
        Sun,
        Sunflash_Query,
        Sunglare,
    };

    osg::ref_ptr<osg::Geometry> createTexturedQuad(int numUvSets = 1, float scale = 1.f)
    {
        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;

        osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array;
        verts->push_back(osg::Vec3f(-0.5f * scale, -0.5f * scale, 0.f));
        verts->push_back(osg::Vec3f(-0.5f * scale, 0.5f * scale, 0.f));
        verts->push_back(osg::Vec3f(0.5f * scale, 0.5f * scale, 0.f));
        verts->push_back(osg::Vec3f(0.5f * scale, -0.5f * scale, 0.f));

        geom->setVertexArray(verts);

        osg::ref_ptr<osg::Vec2Array> texcoords = new osg::Vec2Array;
        texcoords->push_back(osg::Vec2f(0, 1));
        texcoords->push_back(osg::Vec2f(0, 0));
        texcoords->push_back(osg::Vec2f(1, 0));
        texcoords->push_back(osg::Vec2f(1, 1));

        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
        colors->push_back(osg::Vec4(1.f, 1.f, 1.f, 1.f));
        geom->setColorArray(colors, osg::Array::BIND_OVERALL);

        for (int i = 0; i < numUvSets; ++i)
            geom->setTexCoordArray(i, texcoords, osg::Array::BIND_PER_VERTEX);

        geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));

        return geom;
    }

    struct DummyComputeBoundCallback : osg::Node::ComputeBoundingSphereCallback
    {
        osg::BoundingSphere computeBound(const osg::Node& node) const override { return osg::BoundingSphere(); }
    };
}

namespace MWRender
{
    osg::ref_ptr<osg::Material> createUnlitMaterial(osg::Material::ColorMode colorMode)
    {
        osg::ref_ptr<osg::Material> mat = new osg::Material;
        mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 1.f));
        mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 1.f));
        mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(1.f, 1.f, 1.f, 1.f));
        mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 0.f));
        mat->setColorMode(colorMode);
        return mat;
    }

    osg::ref_ptr<osg::Material> createAlphaTrackingUnlitMaterial()
    {
        return createUnlitMaterial(osg::Material::DIFFUSE);
    }

    class SunUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        osg::Vec4f mColor;

        SunUpdater()
            : mColor(1.f, 1.f, 1.f, 1.f)
        {
        }

        void setDefaults(osg::StateSet* stateset) override { stateset->setAttributeAndModes(createUnlitMaterial()); }

        void apply(osg::StateSet* stateset, osg::NodeVisitor*) override
        {
            osg::Material* mat = static_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
            mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0, 0, 0, mColor.a()));
            mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(mColor.r(), mColor.g(), mColor.b(), 1));
        }
    };

    OcclusionCallback::OcclusionCallback(
        osg::ref_ptr<osg::OcclusionQueryNode> oqnVisible, osg::ref_ptr<osg::OcclusionQueryNode> oqnTotal)
        : mOcclusionQueryVisiblePixels(std::move(oqnVisible))
        , mOcclusionQueryTotalPixels(std::move(oqnTotal))
    {
    }

    float OcclusionCallback::getVisibleRatio(osg::Camera* camera)
    {
        int visible = mOcclusionQueryVisiblePixels->getQueryGeometry()->getNumPixels(camera);
        int total = mOcclusionQueryTotalPixels->getQueryGeometry()->getNumPixels(camera);

        float visibleRatio = 0.f;
        if (total > 0)
            visibleRatio = static_cast<float>(visible) / static_cast<float>(total);

        float dt = MWBase::Environment::get().getFrameDuration();

        float lastRatio = mLastRatio[osg::observer_ptr<osg::Camera>(camera)];

        float change = dt * 10;

        if (visibleRatio > lastRatio)
            visibleRatio = std::min(visibleRatio, lastRatio + change);
        else
            visibleRatio = std::max(visibleRatio, lastRatio - change);

        mLastRatio[osg::observer_ptr<osg::Camera>(camera)] = visibleRatio;

        return visibleRatio;
    }

    /// SunFlashCallback handles fading/scaling of a node depending on occlusion query result. Must be attached as a
    /// cull callback.
    class SunFlashCallback : public OcclusionCallback,
                             public SceneUtil::NodeCallback<SunFlashCallback, osg::Node*, osgUtil::CullVisitor*>
    {
    public:
        SunFlashCallback(
            osg::ref_ptr<osg::OcclusionQueryNode> oqnVisible, osg::ref_ptr<osg::OcclusionQueryNode> oqnTotal)
            : OcclusionCallback(std::move(oqnVisible), std::move(oqnTotal))
            , mGlareView(1.f)
        {
        }

        void operator()(osg::Node* node, osgUtil::CullVisitor* cv)
        {
            float visibleRatio = getVisibleRatio(cv->getCurrentCamera());

            osg::ref_ptr<osg::StateSet> stateset;

            if (visibleRatio > 0.f)
            {
                const float fadeThreshold = 0.1f;
                if (visibleRatio < fadeThreshold)
                {
                    float fade = 1.f - (fadeThreshold - visibleRatio) / fadeThreshold;
                    osg::ref_ptr<osg::Material> mat(createUnlitMaterial());
                    mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0, 0, 0, fade * mGlareView));
                    stateset = new osg::StateSet;
                    stateset->setAttributeAndModes(mat, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
                }
                else if (visibleRatio < 1.f)
                {
                    const float threshold = 0.6f;
                    visibleRatio = visibleRatio * (1.f - threshold) + threshold;
                }
            }

            float scale = visibleRatio;

            if (scale == 0.f)
            {
                // no traverse
                return;
            }
            else if (scale == 1.f)
                traverse(node, cv);
            else
            {
                osg::Matrix modelView = *cv->getModelViewMatrix();

                modelView.preMultScale(osg::Vec3f(scale, scale, scale));

                if (stateset)
                    cv->pushStateSet(stateset);

                cv->pushModelViewMatrix(new osg::RefMatrix(modelView), osg::Transform::RELATIVE_RF);

                traverse(node, cv);

                cv->popModelViewMatrix();

                if (stateset)
                    cv->popStateSet();
            }
        }

        void setGlareView(float value) { mGlareView = value; }

    private:
        float mGlareView;
    };

    /// SunGlareCallback controls a full-screen glare effect depending on occlusion query result and the angle between
    /// sun and camera. Must be attached as a cull callback to the node above the glare node.
    class SunGlareCallback : public OcclusionCallback,
                             public SceneUtil::NodeCallback<SunGlareCallback, osg::Node*, osgUtil::CullVisitor*>
    {
    public:
        SunGlareCallback(osg::ref_ptr<osg::OcclusionQueryNode> oqnVisible,
            osg::ref_ptr<osg::OcclusionQueryNode> oqnTotal, osg::ref_ptr<osg::PositionAttitudeTransform> sunTransform)
            : OcclusionCallback(std::move(oqnVisible), std::move(oqnTotal))
            , mSunTransform(std::move(sunTransform))
            , mTimeOfDayFade(1.f)
            , mGlareView(1.f)
        {
            mColor = Fallback::Map::getColour("Weather_Sun_Glare_Fader_Color");
            mSunGlareFaderMax = Fallback::Map::getFloat("Weather_Sun_Glare_Fader_Max");
            mSunGlareFaderAngleMax = Fallback::Map::getFloat("Weather_Sun_Glare_Fader_Angle_Max");

            // Replicating a design flaw in MW. The color was being set on both ambient and emissive properties, which
            // multiplies the result by two, then finally gets clamped by the fixed function pipeline. With the default
            // INI settings, only the red component gets clamped, so the resulting color looks more orange than red.
            mColor *= 2;
            for (int i = 0; i < 3; ++i)
                mColor[i] = std::min(1.f, mColor[i]);
        }

        void operator()(osg::Node* node, osgUtil::CullVisitor* cv)
        {
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
                osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet;

                osg::ref_ptr<osg::Material> mat = createUnlitMaterial();

                mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0, 0, 0, fade));
                mat->setEmission(osg::Material::FRONT_AND_BACK, mColor);

                stateset->setAttributeAndModes(mat);

                cv->pushStateSet(stateset);
                traverse(node, cv);
                cv->popStateSet();
            }
        }

        void setTimeOfDayFade(float val) { mTimeOfDayFade = val; }

        void setGlareView(float glareView) { mGlareView = glareView; }

    private:
        float getAngleToSunInRadians(const osg::Matrix& viewMatrix) const
        {
            osg::Vec3d eye, center, up;
            viewMatrix.getLookAt(eye, center, up);

            osg::Vec3d forward = center - eye;
            osg::Vec3d sun = mSunTransform->getPosition();

            forward.normalize();
            sun.normalize();
            double angleRadians = std::acos(forward * sun);
            return static_cast<float>(angleRadians);
        }

        osg::ref_ptr<osg::PositionAttitudeTransform> mSunTransform;
        float mTimeOfDayFade;
        float mGlareView;
        osg::Vec4f mColor;
        float mSunGlareFaderMax;
        float mSunGlareFaderAngleMax;
    };

    struct MoonUpdater : SceneUtil::StateSetUpdater
    {
        Resource::ImageManager& mImageManager;
        osg::ref_ptr<osg::Texture2D> mPhaseTex;
        osg::ref_ptr<osg::Texture2D> mCircleTex;
        float mTransparency;
        float mShadowBlend;
        osg::Vec4f mAtmosphereColor;
        osg::Vec4f mMoonColor;
        bool mForceShaders;

        MoonUpdater(Resource::ImageManager& imageManager, bool forceShaders)
            : mImageManager(imageManager)
            , mPhaseTex()
            , mCircleTex()
            , mTransparency(1.0f)
            , mShadowBlend(1.0f)
            , mAtmosphereColor(1.0f, 1.0f, 1.0f, 1.0f)
            , mMoonColor(1.0f, 1.0f, 1.0f, 1.0f)
            , mForceShaders(forceShaders)
        {
        }

        void setDefaults(osg::StateSet* stateset) override
        {
            if (mForceShaders)
            {
                stateset->addUniform(new osg::Uniform("pass", static_cast<int>(Pass::Moon)));
                stateset->setTextureAttributeAndModes(0, mPhaseTex);
                stateset->setTextureAttributeAndModes(1, mCircleTex);
                stateset->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
                stateset->setTextureMode(1, GL_TEXTURE_2D, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
                stateset->addUniform(new osg::Uniform("moonBlend", osg::Vec4f{}));
                stateset->addUniform(new osg::Uniform("atmosphereFade", osg::Vec4f{}));
                stateset->addUniform(new osg::Uniform("diffuseMap", 0));
                stateset->addUniform(new osg::Uniform("maskMap", 1));
                stateset->setAttributeAndModes(
                    createUnlitMaterial(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            }
            else
            {
                stateset->setTextureAttributeAndModes(0, mPhaseTex);
                osg::ref_ptr<osg::TexEnvCombine> texEnv = new osg::TexEnvCombine;
                texEnv->setCombine_RGB(osg::TexEnvCombine::MODULATE);
                texEnv->setSource0_RGB(osg::TexEnvCombine::CONSTANT);
                texEnv->setSource1_RGB(osg::TexEnvCombine::TEXTURE);
                texEnv->setConstantColor(osg::Vec4f(1.f, 0.f, 0.f, 1.f)); // mShadowBlend * mMoonColor
                stateset->setTextureAttributeAndModes(0, texEnv);

                stateset->setTextureAttributeAndModes(1, mCircleTex);
                osg::ref_ptr<osg::TexEnvCombine> texEnv2 = new osg::TexEnvCombine;
                texEnv2->setCombine_RGB(osg::TexEnvCombine::ADD);
                texEnv2->setCombine_Alpha(osg::TexEnvCombine::MODULATE);
                texEnv2->setSource0_Alpha(osg::TexEnvCombine::TEXTURE);
                texEnv2->setSource1_Alpha(osg::TexEnvCombine::CONSTANT);
                texEnv2->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
                texEnv2->setSource1_RGB(osg::TexEnvCombine::CONSTANT);
                texEnv2->setConstantColor(osg::Vec4f(0.f, 0.f, 0.f, 1.f)); // mAtmosphereColor.rgb, mTransparency
                stateset->setTextureAttributeAndModes(1, texEnv2);
                stateset->setAttributeAndModes(
                    createUnlitMaterial(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            }
        }

        void apply(osg::StateSet* stateset, osg::NodeVisitor*) override
        {
            if (mForceShaders)
            {
                stateset->setTextureAttribute(0, mPhaseTex, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
                stateset->setTextureAttribute(1, mCircleTex, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

                if (auto* uMoonBlend = stateset->getUniform("moonBlend"))
                    uMoonBlend->set(mMoonColor * mShadowBlend);
                if (auto* uAtmosphereFade = stateset->getUniform("atmosphereFade"))
                    uAtmosphereFade->set(
                        osg::Vec4f(mAtmosphereColor.x(), mAtmosphereColor.y(), mAtmosphereColor.z(), mTransparency));
            }
            else
            {
                osg::TexEnvCombine* texEnv
                    = static_cast<osg::TexEnvCombine*>(stateset->getTextureAttribute(0, osg::StateAttribute::TEXENV));
                texEnv->setConstantColor(mMoonColor * mShadowBlend);

                osg::TexEnvCombine* texEnv2
                    = static_cast<osg::TexEnvCombine*>(stateset->getTextureAttribute(1, osg::StateAttribute::TEXENV));
                texEnv2->setConstantColor(
                    osg::Vec4f(mAtmosphereColor.x(), mAtmosphereColor.y(), mAtmosphereColor.z(), mTransparency));
            }
        }

        void setTextures(VFS::Path::NormalizedView phaseTex, VFS::Path::NormalizedView circleTex)
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

    class CameraRelativeTransformCullCallback
        : public SceneUtil::NodeCallback<CameraRelativeTransformCullCallback, osg::Node*, osgUtil::CullVisitor*>
    {
    public:
        void operator()(osg::Node* node, osgUtil::CullVisitor* cv)
        {
            // XXX have to remove unwanted culling plane of the water reflection camera

            // Remove all planes that aren't from the standard frustum
            unsigned int numPlanes = 4;
            if (cv->getCullingMode() & osg::CullSettings::NEAR_PLANE_CULLING)
                ++numPlanes;
            if (cv->getCullingMode() & osg::CullSettings::FAR_PLANE_CULLING)
                ++numPlanes;

            unsigned int mask = 0x1;
            unsigned int resultMask = cv->getProjectionCullingStack().back().getFrustum().getResultMask();
            for (unsigned int i = 0; i < cv->getProjectionCullingStack().back().getFrustum().getPlaneList().size(); ++i)
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

            traverse(node, cv);

            cv->getProjectionCullingStack().back().popCurrentMask();
            cv->getCurrentCullingSet().popCurrentMask();
        }
    };

    void AtmosphereUpdater::setEmissionColor(const osg::Vec4f& emissionColor)
    {
        mEmissionColor = emissionColor;
    }

    void AtmosphereUpdater::setDefaults(osg::StateSet* stateset)
    {
        stateset->setAttributeAndModes(
            createAlphaTrackingUnlitMaterial(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        stateset->addUniform(new osg::Uniform("pass", static_cast<int>(Pass::Atmosphere)));
    }

    void AtmosphereUpdater::apply(osg::StateSet* stateset, osg::NodeVisitor* /*nv*/)
    {
        osg::Material* mat = static_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
        mat->setEmission(osg::Material::FRONT_AND_BACK, mEmissionColor);
    }

    AtmosphereNightUpdater::AtmosphereNightUpdater(Resource::ImageManager* imageManager, bool forceShaders)
        : mColor(osg::Vec4f(0, 0, 0, 0))
        , mTexture(new osg::Texture2D(imageManager->getWarningImage()))
        , mForceShaders(forceShaders)
    {
        mTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        mTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    }

    void AtmosphereNightUpdater::setFade(float fade)
    {
        mColor.a() = fade;
    }

    void AtmosphereNightUpdater::setDefaults(osg::StateSet* stateset)
    {
        if (mForceShaders)
        {
            stateset->addUniform(new osg::Uniform("opacity", 0.f));
            stateset->addUniform(new osg::Uniform("pass", static_cast<int>(Pass::Atmosphere_Night)));
        }
        else
        {
            osg::ref_ptr<osg::TexEnvCombine> texEnv = new osg::TexEnvCombine;
            texEnv->setCombine_Alpha(osg::TexEnvCombine::MODULATE);
            texEnv->setSource0_Alpha(osg::TexEnvCombine::PREVIOUS);
            texEnv->setSource1_Alpha(osg::TexEnvCombine::CONSTANT);
            texEnv->setCombine_RGB(osg::TexEnvCombine::REPLACE);
            texEnv->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);

            stateset->setTextureAttributeAndModes(1, mTexture, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            stateset->setTextureAttributeAndModes(1, texEnv, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        }
    }

    void AtmosphereNightUpdater::apply(osg::StateSet* stateset, osg::NodeVisitor* /*nv*/)
    {
        if (mForceShaders)
        {
            stateset->getUniform("opacity")->set(mColor.a());
        }
        else
        {
            osg::TexEnvCombine* texEnv
                = static_cast<osg::TexEnvCombine*>(stateset->getTextureAttribute(1, osg::StateAttribute::TEXENV));
            texEnv->setConstantColor(mColor);
        }
    }

    CloudUpdater::CloudUpdater(bool forceShaders)
        : mOpacity(0.f)
        , mForceShaders(forceShaders)
    {
    }

    void CloudUpdater::setTexture(osg::ref_ptr<osg::Texture2D> texture)
    {
        mTexture = texture;
    }

    void CloudUpdater::setEmissionColor(const osg::Vec4f& emissionColor)
    {
        mEmissionColor = emissionColor;
    }

    void CloudUpdater::setOpacity(float opacity)
    {
        mOpacity = opacity;
    }

    void CloudUpdater::setTextureCoord(float timer)
    {
        mTexMat = osg::Matrixf::translate(osg::Vec3f(0.f, -timer, 0.f));
    }

    void CloudUpdater::setDefaults(osg::StateSet* stateset)
    {
        stateset->setAttribute(
            createAlphaTrackingUnlitMaterial(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

        osg::ref_ptr<osg::TexMat> texmat = new osg::TexMat;
        stateset->setTextureAttributeAndModes(0, texmat);

        if (mForceShaders)
        {
            stateset->setTextureAttribute(0, mTexture, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

            stateset->addUniform(new osg::Uniform("opacity", 1.f));
            stateset->addUniform(new osg::Uniform("pass", static_cast<int>(Pass::Clouds)));
        }
        else
        {
            stateset->setTextureAttributeAndModes(1, texmat);
            // need to set opacity on a separate texture unit, diffuse alpha is used by the vertex colors already
            osg::ref_ptr<osg::TexEnvCombine> texEnvCombine = new osg::TexEnvCombine;
            texEnvCombine->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
            texEnvCombine->setSource0_Alpha(osg::TexEnvCombine::PREVIOUS);
            texEnvCombine->setSource1_Alpha(osg::TexEnvCombine::CONSTANT);
            texEnvCombine->setConstantColor(osg::Vec4f(1, 1, 1, 1));
            texEnvCombine->setCombine_Alpha(osg::TexEnvCombine::MODULATE);
            texEnvCombine->setCombine_RGB(osg::TexEnvCombine::REPLACE);

            stateset->setTextureAttributeAndModes(1, texEnvCombine);

            stateset->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            stateset->setTextureMode(1, GL_TEXTURE_2D, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        }
    }

    void CloudUpdater::apply(osg::StateSet* stateset, osg::NodeVisitor* nv)
    {
        stateset->setTextureAttribute(0, mTexture, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

        osg::Material* mat = static_cast<osg::Material*>(stateset->getAttribute(osg::StateAttribute::MATERIAL));
        mat->setEmission(osg::Material::FRONT_AND_BACK, mEmissionColor);

        osg::TexMat* texMat = static_cast<osg::TexMat*>(stateset->getTextureAttribute(0, osg::StateAttribute::TEXMAT));
        texMat->setMatrix(mTexMat);

        if (mForceShaders)
        {
            stateset->getUniform("opacity")->set(mOpacity);
        }
        else
        {
            stateset->setTextureAttribute(1, mTexture, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

            osg::TexEnvCombine* texEnv
                = static_cast<osg::TexEnvCombine*>(stateset->getTextureAttribute(1, osg::StateAttribute::TEXENV));
            texEnv->setConstantColor(osg::Vec4f(1, 1, 1, mOpacity));
        }
    }

    class SkyStereoStatesetUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        SkyStereoStatesetUpdater() {}

    protected:
        void setDefaults(osg::StateSet* stateset) override
        {
            if (!Stereo::getMultiview())
                stateset->addUniform(
                    new osg::Uniform(osg::Uniform::FLOAT_MAT4, "projectionMatrix"), osg::StateAttribute::OVERRIDE);
        }

        void apply(osg::StateSet* stateset, osg::NodeVisitor* /*nv*/) override
        {
            if (Stereo::getMultiview())
            {
                std::array<osg::Matrix, 2> projectionMatrices;
                auto& sm = Stereo::Manager::instance();

                for (int view : { 0, 1 })
                {
                    auto projectionMatrix = sm.computeEyeProjection(view, SceneUtil::AutoDepth::isReversed());
                    auto viewOffsetMatrix = sm.computeEyeViewOffset(view);
                    for (int col : { 0, 1, 2 })
                        viewOffsetMatrix(3, col) = 0;

                    projectionMatrices[view] = viewOffsetMatrix * projectionMatrix;
                }

                Stereo::setMultiviewMatrices(stateset, projectionMatrices);
            }
        }
        void applyLeft(osg::StateSet* stateset, osgUtil::CullVisitor* /*cv*/) override
        {
            auto& sm = Stereo::Manager::instance();
            auto* projectionMatrixUniform = stateset->getUniform("projectionMatrix");
            auto projectionMatrix = sm.computeEyeProjection(0, SceneUtil::AutoDepth::isReversed());
            projectionMatrixUniform->set(projectionMatrix);
        }
        void applyRight(osg::StateSet* stateset, osgUtil::CullVisitor* /*cv*/) override
        {
            auto& sm = Stereo::Manager::instance();
            auto* projectionMatrixUniform = stateset->getUniform("projectionMatrix");
            auto projectionMatrix = sm.computeEyeProjection(1, SceneUtil::AutoDepth::isReversed());
            projectionMatrixUniform->set(projectionMatrix);
        }

    private:
    };

    CameraRelativeTransform::CameraRelativeTransform()
    {
        // Culling works in node-local space, not in camera space, so we can't cull this node correctly
        // That's not a problem though, children of this node can be culled just fine
        // Just make sure you do not place a CameraRelativeTransform deep in the scene graph
        setCullingActive(false);

        addCullCallback(new CameraRelativeTransformCullCallback);
        if (Stereo::getStereo())
            addCullCallback(new SkyStereoStatesetUpdater);
    }

    CameraRelativeTransform::CameraRelativeTransform(const CameraRelativeTransform& copy, const osg::CopyOp& copyop)
        : osg::Transform(copy, copyop)
    {
    }

    const osg::Vec3f& CameraRelativeTransform::getLastViewPoint() const
    {
        return mViewPoint;
    }

    bool CameraRelativeTransform::computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
    {
        if (nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
        {
            mViewPoint = static_cast<osgUtil::CullVisitor*>(nv)->getViewPoint();
        }

        if (_referenceFrame == RELATIVE_RF)
        {
            matrix.setTrans(osg::Vec3f(0.f, 0.f, 0.f));
            return false;
        }
        else // absolute
        {
            matrix.makeIdentity();
            return true;
        }
    }

    osg::BoundingSphere CameraRelativeTransform::computeBound() const
    {
        return osg::BoundingSphere();
    }

    UnderwaterSwitchCallback::UnderwaterSwitchCallback(CameraRelativeTransform* cameraRelativeTransform)
        : mCameraRelativeTransform(cameraRelativeTransform)
        , mEnabled(true)
        , mWaterLevel(0.f)
    {
    }

    bool UnderwaterSwitchCallback::isUnderwater()
    {
        osg::Vec3f viewPoint = mCameraRelativeTransform->getLastViewPoint();
        return mEnabled && viewPoint.z() < mWaterLevel;
    }

    void UnderwaterSwitchCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        if (isUnderwater())
            return;

        traverse(node, nv);
    }

    void UnderwaterSwitchCallback::setEnabled(bool enabled)
    {
        mEnabled = enabled;
    }
    void UnderwaterSwitchCallback::setWaterLevel(float waterLevel)
    {
        mWaterLevel = waterLevel;
    }

    const float CelestialBody::mDistance = 1000.0f;

    CelestialBody::CelestialBody(osg::Group* parentNode, float scaleFactor, int numUvSets, unsigned int visibleMask)
        : mVisibleMask(visibleMask)
    {
        mGeom = createTexturedQuad(numUvSets);
        mGeom->getOrCreateStateSet();
        mTransform = new osg::PositionAttitudeTransform;
        mTransform->setNodeMask(mVisibleMask);
        mTransform->setScale(osg::Vec3f(450, 450, 450) * scaleFactor);
        mTransform->addChild(mGeom);

        parentNode->addChild(mTransform);
    }

    void CelestialBody::setVisible(bool visible)
    {
        mTransform->setNodeMask(visible ? mVisibleMask : 0);
    }

    Sun::Sun(osg::Group* parentNode, Resource::SceneManager& sceneManager)
        : CelestialBody(parentNode, 1.0f, 1, Mask_Sun)
        , mUpdater(new SunUpdater)
    {
        mTransform->addUpdateCallback(mUpdater);

        Resource::ImageManager& imageManager = *sceneManager.getImageManager();

        constexpr VFS::Path::NormalizedView image("textures/tx_sun_05.dds");

        osg::ref_ptr<osg::Texture2D> sunTex = new osg::Texture2D(imageManager.getImage(image));
        sunTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        sunTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

        mGeom->getOrCreateStateSet()->setTextureAttributeAndModes(0, sunTex);
        mGeom->getOrCreateStateSet()->setTextureAttributeAndModes(
            0, new SceneUtil::TextureType("diffuseMap"), osg::StateAttribute::ON);
        mGeom->getOrCreateStateSet()->addUniform(new osg::Uniform("pass", static_cast<int>(Pass::Sun)));

        osg::ref_ptr<osg::Group> queryNode = new osg::Group;
        // Need to render after the world geometry so we can correctly test for occlusions
        osg::StateSet* stateset = queryNode->getOrCreateStateSet();
        stateset->setRenderBinDetails(RenderBin_OcclusionQuery, "RenderBin");
        stateset->setNestRenderBins(false);
        // Set up alpha testing on the occlusion testing subgraph, that way we can get the occlusion tested fragments to
        // match the circular shape of the sun
        if (!sceneManager.getForceShaders())
        {
            osg::ref_ptr<osg::AlphaFunc> alphaFunc = new osg::AlphaFunc;
            alphaFunc->setFunction(osg::AlphaFunc::GREATER, 0.8f);
            stateset->setAttributeAndModes(alphaFunc);
        }
        stateset->setTextureAttributeAndModes(0, sunTex);
        stateset->setTextureAttributeAndModes(0, new SceneUtil::TextureType("diffuseMap"), osg::StateAttribute::ON);
        stateset->setAttributeAndModes(createUnlitMaterial());
        stateset->addUniform(new osg::Uniform("pass", static_cast<int>(Pass::Sunflash_Query)));

        // Disable writing to the color buffer. We are using this geometry for visibility tests only.
        osg::ref_ptr<osg::ColorMask> colormask = new osg::ColorMask(0, 0, 0, 0);
        stateset->setAttributeAndModes(colormask);
        sceneManager.setUpNormalsRTForStateSet(stateset, false);
        mTransform->addChild(queryNode);

        mOcclusionQueryVisiblePixels = createOcclusionQueryNode(queryNode, true);
        mOcclusionQueryTotalPixels = createOcclusionQueryNode(queryNode, false);

        createSunFlash(imageManager);
        createSunGlare();
    }

    Sun::~Sun()
    {
        mTransform->removeUpdateCallback(mUpdater);
        destroySunFlash();
        destroySunGlare();
    }

    void Sun::setColor(const osg::Vec4f& color)
    {
        mUpdater->mColor.r() = color.r();
        mUpdater->mColor.g() = color.g();
        mUpdater->mColor.b() = color.b();
    }

    void Sun::adjustTransparency(const float ratio)
    {
        mUpdater->mColor.a() = ratio;
        if (mSunGlareCallback)
            mSunGlareCallback->setGlareView(ratio);
        if (mSunFlashCallback)
            mSunFlashCallback->setGlareView(ratio);
    }

    void Sun::setDirection(const osg::Vec3f& direction)
    {
        osg::Vec3f normalizedDirection = direction / direction.length();
        mTransform->setPosition(normalizedDirection * mDistance);

        osg::Quat quat;
        quat.makeRotate(osg::Vec3f(0.0f, 0.0f, 1.0f), normalizedDirection);
        mTransform->setAttitude(quat);
    }

    void Sun::setGlareTimeOfDayFade(float val)
    {
        if (mSunGlareCallback)
            mSunGlareCallback->setTimeOfDayFade(val);
    }

    void Sun::setSunglare(bool enabled)
    {
        mSunGlareNode->setNodeMask(enabled ? ~0u : 0);
        mSunFlashNode->setNodeMask(enabled ? ~0u : 0);
    }

    osg::ref_ptr<osg::OcclusionQueryNode> Sun::createOcclusionQueryNode(osg::Group* parent, bool queryVisible)
    {
        osg::ref_ptr<osg::OcclusionQueryNode> oqn = new osg::OcclusionQueryNode;
        oqn->setQueriesEnabled(true);

        // With OSG 3.6.5, the method of providing user defined query geometry has been completely replaced
        osg::ref_ptr<osg::QueryGeometry> queryGeom = new osg::QueryGeometry(oqn->getName());

        // Make it fast! A DYNAMIC query geometry means we can't break frame until the flare is rendered (which is
        // rendered after all the other geometry, so that would be pretty bad). STATIC should be safe, since our node's
        // local bounds are static, thus computeBounds() which modifies the queryGeometry is only called once. Note the
        // debug geometry setDebugDisplay(true) is always DYNAMIC and that can't be changed, not a big deal.
        queryGeom->setDataVariance(osg::Object::STATIC);

        // Set up the query geometry to match the actual sun's rendering shape. osg::OcclusionQueryNode wasn't
        // originally intended to allow this, normally it would automatically adjust the query geometry to match the sub
        // graph's bounding box. The below hack is needed to circumvent this.
        queryGeom->setVertexArray(mGeom->getVertexArray());
        queryGeom->setTexCoordArray(0, mGeom->getTexCoordArray(0), osg::Array::BIND_PER_VERTEX);
        queryGeom->removePrimitiveSet(0, queryGeom->getNumPrimitiveSets());
        queryGeom->addPrimitiveSet(mGeom->getPrimitiveSet(0));

        // Hack to disable unwanted awful code inside OcclusionQueryNode::computeBound.
        oqn->setComputeBoundingSphereCallback(new DummyComputeBoundCallback);
        // Still need a proper bounding sphere.
        oqn->setInitialBound(queryGeom->getBound());

        oqn->setQueryGeometry(queryGeom.release());

        osg::StateSet* queryStateSet = new osg::StateSet;
        if (queryVisible)
        {
            osg::ref_ptr<osg::Depth> depth = new SceneUtil::AutoDepth;
            // This is a trick to make fragments written by the query always use the maximum depth value,
            // without having to retrieve the current far clipping distance.
            // We want the sun glare to be "infinitely" far away.
            double far = SceneUtil::AutoDepth::isReversed() ? 0.0 : 1.0;
            depth->setZNear(far);
            depth->setZFar(far);
            depth->setWriteMask(false);
            queryStateSet->setAttributeAndModes(depth);
        }
        else
        {
            queryStateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
        }
        oqn->setQueryStateSet(queryStateSet);

        parent->addChild(oqn);

        return oqn;
    }

    void Sun::createSunFlash(Resource::ImageManager& imageManager)
    {
        constexpr VFS::Path::NormalizedView image("textures/tx_sun_flash_grey_05.dds");
        osg::ref_ptr<osg::Texture2D> tex = new osg::Texture2D(imageManager.getImage(image));
        tex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        tex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);

        osg::ref_ptr<osg::Group> group(new osg::Group);

        mTransform->addChild(group);

        const float scale = 2.6f;
        osg::ref_ptr<osg::Geometry> geom = createTexturedQuad(1, scale);
        group->addChild(geom);

        osg::StateSet* stateset = geom->getOrCreateStateSet();

        stateset->setTextureAttributeAndModes(0, tex);
        stateset->setTextureAttributeAndModes(0, new SceneUtil::TextureType("diffuseMap"), osg::StateAttribute::ON);
        stateset->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
        stateset->setRenderBinDetails(RenderBin_SunGlare, "RenderBin");
        stateset->setNestRenderBins(false);
        stateset->addUniform(new osg::Uniform("pass", static_cast<int>(Pass::Sun)));

        mSunFlashNode = group;

        mSunFlashCallback = new SunFlashCallback(mOcclusionQueryVisiblePixels, mOcclusionQueryTotalPixels);
        mSunFlashNode->addCullCallback(mSunFlashCallback);
    }

    void Sun::destroySunFlash()
    {
        if (mSunFlashNode)
        {
            mSunFlashNode->removeCullCallback(mSunFlashCallback);
            mSunFlashCallback = nullptr;
        }
    }

    void Sun::createSunGlare()
    {
        osg::ref_ptr<osg::Camera> camera = new osg::Camera;
        camera->setProjectionMatrix(osg::Matrix::identity());
        camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF); // add to skyRoot instead?
        camera->setViewMatrix(osg::Matrix::identity());
        camera->setClearMask(0);
        camera->setRenderOrder(osg::Camera::NESTED_RENDER);
        camera->setAllowEventFocus(false);
        camera->getOrCreateStateSet()->addUniform(
            new osg::Uniform("projectionMatrix", static_cast<osg::Matrixf>(camera->getProjectionMatrix())));
        SceneUtil::setCameraClearDepth(camera);

        osg::ref_ptr<osg::Geometry> geom
            = osg::createTexturedQuadGeometry(osg::Vec3f(-1, -1, 0), osg::Vec3f(2, 0, 0), osg::Vec3f(0, 2, 0));
        camera->addChild(geom);

        osg::StateSet* stateset = geom->getOrCreateStateSet();

        stateset->setRenderBinDetails(RenderBin_SunGlare, "RenderBin");
        stateset->setNestRenderBins(false);
        stateset->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
        stateset->addUniform(new osg::Uniform("pass", static_cast<int>(Pass::Sunglare)));

        // set up additive blending
        osg::ref_ptr<osg::BlendFunc> blendFunc = new osg::BlendFunc;
        blendFunc->setSource(osg::BlendFunc::SRC_ALPHA);
        blendFunc->setDestination(osg::BlendFunc::ONE);
        stateset->setAttributeAndModes(blendFunc);

        mSunGlareCallback = new SunGlareCallback(mOcclusionQueryVisiblePixels, mOcclusionQueryTotalPixels, mTransform);
        mSunGlareNode = camera;

        mSunGlareNode->addCullCallback(mSunGlareCallback);

        mTransform->addChild(camera);
    }

    void Sun::destroySunGlare()
    {
        if (mSunGlareNode)
        {
            mSunGlareNode->removeCullCallback(mSunGlareCallback);
            mSunGlareCallback = nullptr;
        }
    }

    Moon::Moon(osg::Group* parentNode, Resource::SceneManager& sceneManager, float scaleFactor, Type type)
        : CelestialBody(parentNode, scaleFactor, 2)
        , mType(type)
        , mPhase(MoonState::Phase::Unspecified)
        , mUpdater(new MoonUpdater(*sceneManager.getImageManager(), sceneManager.getForceShaders()))
    {
        setPhase(MoonState::Phase::Full);
        setVisible(true);

        mGeom->addUpdateCallback(mUpdater);
    }

    Moon::~Moon()
    {
        mGeom->removeUpdateCallback(mUpdater);
    }

    void Moon::adjustTransparency(const float ratio)
    {
        mUpdater->mTransparency *= ratio;
    }

    void Moon::setState(const MoonState state)
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

    void Moon::setAtmosphereColor(const osg::Vec4f& color)
    {
        mUpdater->mAtmosphereColor = color;
    }

    void Moon::setColor(const osg::Vec4f& color)
    {
        mUpdater->mMoonColor = color;
    }

    unsigned int Moon::getPhaseInt() const
    {
        switch (mPhase)
        {
            case MoonState::Phase::New:
                return 0;
            case MoonState::Phase::WaxingCrescent:
                return 1;
            case MoonState::Phase::WaningCrescent:
                return 1;
            case MoonState::Phase::FirstQuarter:
                return 2;
            case MoonState::Phase::ThirdQuarter:
                return 2;
            case MoonState::Phase::WaxingGibbous:
                return 3;
            case MoonState::Phase::WaningGibbous:
                return 3;
            case MoonState::Phase::Full:
                return 4;
            default:
                return 0;
        }
    }

    void Moon::setPhase(const MoonState::Phase& phase)
    {
        if (mPhase == phase)
            return;

        mPhase = phase;

        std::string textureName = "textures/tx_";

        if (mType == Moon::Type_Secunda)
            textureName += "secunda_";
        else
            textureName += "masser_";

        switch (mPhase)
        {
            case MoonState::Phase::New:
                textureName += "new";
                break;
            case MoonState::Phase::WaxingCrescent:
                textureName += "one_wax";
                break;
            case MoonState::Phase::FirstQuarter:
                textureName += "half_wax";
                break;
            case MoonState::Phase::WaxingGibbous:
                textureName += "three_wax";
                break;
            case MoonState::Phase::WaningCrescent:
                textureName += "one_wan";
                break;
            case MoonState::Phase::ThirdQuarter:
                textureName += "half_wan";
                break;
            case MoonState::Phase::WaningGibbous:
                textureName += "three_wan";
                break;
            case MoonState::Phase::Full:
                textureName += "full";
                break;
            default:
                break;
        }

        textureName += ".dds";

        const VFS::Path::Normalized texturePath(std::move(textureName));

        if (mType == Moon::Type_Secunda)
        {
            constexpr VFS::Path::NormalizedView secunda("textures/tx_mooncircle_full_s.dds");
            mUpdater->setTextures(texturePath, secunda);
        }
        else
        {
            constexpr VFS::Path::NormalizedView masser("textures/tx_mooncircle_full_m.dds");
            mUpdater->setTextures(texturePath, masser);
        }
    }

    int RainCounter::numParticlesToCreate(double dt) const
    {
        // limit dt to avoid large particle emissions if there are jumps in the simulation time
        // 0.2 seconds is the same cap as used in Engine's frame loop
        dt = std::min(dt, 0.2);
        return ConstantRateCounter::numParticlesToCreate(dt);
    }

    RainShooter::RainShooter()
        : mAngle(0.f)
    {
    }

    void RainShooter::shoot(osgParticle::Particle* particle) const
    {
        particle->setVelocity(mVelocity);
        particle->setAngle(osg::Vec3f(-mAngle, 0, (Misc::Rng::rollProbability() * 2 - 1) * osg::PIf));
    }

    void RainShooter::setVelocity(const osg::Vec3f& velocity)
    {
        mVelocity = velocity;
    }

    void RainShooter::setAngle(float angle)
    {
        mAngle = angle;
    }

    osg::Object* RainShooter::cloneType() const
    {
        return new RainShooter;
    }

    osg::Object* RainShooter::clone(const osg::CopyOp&) const
    {
        return new RainShooter(*this);
    }

    ModVertexAlphaVisitor::ModVertexAlphaVisitor(ModVertexAlphaVisitor::MeshType type)
        : osg::NodeVisitor(TRAVERSE_ALL_CHILDREN)
        , mType(type)
    {
    }

    void ModVertexAlphaVisitor::apply(osg::Geometry& geometry)
    {
        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array(geometry.getVertexArray()->getNumElements());
        for (unsigned int i = 0; i < colors->size(); ++i)
        {
            float alpha = 1.f;

            switch (mType)
            {
                case ModVertexAlphaVisitor::Atmosphere:
                {
                    // this is a cylinder, so every second vertex belongs to the bottom-most row
                    alpha = (i % 2) ? 0.f : 1.f;
                    break;
                }
                case ModVertexAlphaVisitor::Clouds:
                {
                    if (i >= 49 && i <= 64)
                        alpha = 0.f; // bottom-most row
                    else if (i >= 33 && i <= 48)
                        alpha = 0.25098f; // second row
                    else
                        alpha = 1.f;
                    break;
                }
                case ModVertexAlphaVisitor::Stars:
                {
                    if (geometry.getColorArray())
                    {
                        osg::Vec4Array* origColors = static_cast<osg::Vec4Array*>(geometry.getColorArray());
                        alpha = ((*origColors)[i].x() == 1.f) ? 1.f : 0.f;
                    }
                    else
                        alpha = 1.f;
                    break;
                }
            }

            (*colors)[i] = osg::Vec4f(0.f, 0.f, 0.f, alpha);
        }

        geometry.setColorArray(colors, osg::Array::BIND_PER_VERTEX);
    }
}
