#include "stateupdater.hpp"

#include <osg/Fog>
#include <osg/LightModel>
#include <osg/PolygonMode>

#include "depth.hpp"
#include "rtt.hpp"
#include "statesetupdater.hpp"

#include <components/resource/scenemanager.hpp>
#include <components/stereo/multiview.hpp>
#include <components/stereo/stereomanager.hpp>

namespace SceneUtil
{
    PerViewUniformStateUpdater::PerViewUniformStateUpdater(Resource::SceneManager* sceneManager, int opaqueTextureUnit)
        : mSceneManager(sceneManager)
        , mOpaqueTextureUnit(opaqueTextureUnit)
    {
    }

    void PerViewUniformStateUpdater::setDefaults(osg::StateSet* stateset)
    {
        stateset->addUniform(new osg::Uniform("projectionMatrix", osg::Matrixf{}));
        if (mSkyRTT)
            stateset->addUniform(new osg::Uniform("sky", mSkyTextureUnit));
    }

    void PerViewUniformStateUpdater::apply(osg::StateSet* stateset, osg::NodeVisitor* nv)
    {
        stateset->getUniform("projectionMatrix")->set(mProjectionMatrix);
        if (mSkyRTT && nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
        {
            osg::Texture* skyTexture = mSkyRTT->getColorTexture(static_cast<osgUtil::CullVisitor*>(nv));
            stateset->setTextureAttribute(
                mSkyTextureUnit, skyTexture, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        }

        if (mOpaqueTextureUnit > 0)
            stateset->setTextureAttribute(mOpaqueTextureUnit,
                mSceneManager->getOpaqueDepthTex(nv->getTraversalNumber()), osg::StateAttribute::ON);
    }

    void PerViewUniformStateUpdater::applyLeft(osg::StateSet* stateset, osgUtil::CullVisitor* nv)
    {
        stateset->getUniform("projectionMatrix")->set(getEyeProjectionMatrix(0));
    }

    void PerViewUniformStateUpdater::applyRight(osg::StateSet* stateset, osgUtil::CullVisitor* nv)
    {
        stateset->getUniform("projectionMatrix")->set(getEyeProjectionMatrix(1));
    }

    void PerViewUniformStateUpdater::setProjectionMatrix(const osg::Matrixf& projectionMatrix)
    {
        mProjectionMatrix = projectionMatrix;
    }

    const osg::Matrixf& PerViewUniformStateUpdater::getProjectionMatrix() const
    {
        return mProjectionMatrix;
    }

    void PerViewUniformStateUpdater::enableSkyRTT(int skyTextureUnit, RTTNode* skyRTT)
    {
        mSkyTextureUnit = skyTextureUnit;
        mSkyRTT = skyRTT;
    }

    osg::Matrixf PerViewUniformStateUpdater::getEyeProjectionMatrix(int view)
    {
        return Stereo::Manager::instance().computeEyeProjection(view, AutoDepth::isReversed());
    }

    SharedUniformStateUpdater::SharedUniformStateUpdater(float skyBlendingStartCoef)
        : mSkyBlendingStartCoef(skyBlendingStartCoef)
    {
    }

    void SharedUniformStateUpdater::setDefaults(osg::StateSet* stateset)
    {
        stateset->addUniform(new osg::Uniform("near", 0.f));
        stateset->addUniform(new osg::Uniform("far", 0.f));
        stateset->addUniform(new osg::Uniform("skyBlendingStart", 0.f));
        stateset->addUniform(new osg::Uniform("screenRes", osg::Vec2f{}));
        stateset->addUniform(new osg::Uniform("isReflection", false));
        stateset->addUniform(new osg::Uniform("windSpeed", 0.0f));
        stateset->addUniform(new osg::Uniform("playerPos", osg::Vec3f(0.f, 0.f, 0.f)));
        stateset->addUniform(new osg::Uniform("useTreeAnim", false));
    }

    void SharedUniformStateUpdater::apply(osg::StateSet* stateset, osg::NodeVisitor* nv)
    {
        stateset->getUniform("near")->set(mNear);
        stateset->getUniform("far")->set(mFar);
        stateset->getUniform("skyBlendingStart")->set(mFar * mSkyBlendingStartCoef);
        stateset->getUniform("screenRes")->set(mScreenRes);
        stateset->getUniform("windSpeed")->set(mWindSpeed);
        stateset->getUniform("playerPos")->set(mPlayerPos);
    }

    void SharedUniformStateUpdater::setNear(float near)
    {
        mNear = near;
    }

    void SharedUniformStateUpdater::setFar(float far)
    {
        mFar = far;
    }

    void SharedUniformStateUpdater::setScreenRes(float width, float height)
    {
        mScreenRes = osg::Vec2f(width, height);
    }

    void SharedUniformStateUpdater::setWindSpeed(float windSpeed)
    {
        mWindSpeed = windSpeed;
    }

    void SharedUniformStateUpdater::setPlayerPos(osg::Vec3f playerPos)
    {
        mPlayerPos = playerPos;
    }

    void StateUpdater::setDefaults(osg::StateSet* stateset)
    {
        osg::LightModel* lightModel = new osg::LightModel;
        stateset->setAttribute(lightModel, osg::StateAttribute::ON);
        osg::Fog* fog = new osg::Fog;
        fog->setMode(osg::Fog::LINEAR);
        stateset->setAttributeAndModes(fog, osg::StateAttribute::ON);
        if (mWireframe)
        {
            osg::PolygonMode* polygonmode = new osg::PolygonMode;
            polygonmode->setMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
            stateset->setAttributeAndModes(polygonmode, osg::StateAttribute::ON);
        }
        else
            stateset->removeAttribute(osg::StateAttribute::POLYGONMODE);
    }

    void StateUpdater::apply(osg::StateSet* stateset, osg::NodeVisitor*)
    {
        osg::LightModel* lightModel
            = static_cast<osg::LightModel*>(stateset->getAttribute(osg::StateAttribute::LIGHTMODEL));
        lightModel->setAmbientIntensity(mAmbientColor);
        osg::Fog* fog = static_cast<osg::Fog*>(stateset->getAttribute(osg::StateAttribute::FOG));
        fog->setColor(mFogColor);
        fog->setStart(mFogStart);
        fog->setEnd(mFogEnd);
    }

    void StateUpdater::setAmbientColor(const osg::Vec4f& col)
    {
        mAmbientColor = col;
    }

    void StateUpdater::setFogColor(const osg::Vec4f& col)
    {
        mFogColor = col;
    }

    void StateUpdater::setFogStart(float start)
    {
        mFogStart = start;
    }

    void StateUpdater::setFogEnd(float end)
    {
        mFogEnd = end;
    }

    void StateUpdater::setWireframe(bool wireframe)
    {
        if (mWireframe != wireframe)
        {
            mWireframe = wireframe;
            reset();
        }
    }

    bool StateUpdater::getWireframe() const
    {
        return mWireframe;
    }

}
