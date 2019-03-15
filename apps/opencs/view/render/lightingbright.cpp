#include "lightingbright.hpp"

#include <osg/LightSource>

CSVRender::LightingBright::LightingBright() {}

void CSVRender::LightingBright::activate (osg::Group* rootNode, bool /*isExterior*/)
{
    mRootNode = rootNode;

    mLightSource = (new osg::LightSource);

    osg::ref_ptr<osg::Light> light (new osg::Light);
    light->setAmbient(osg::Vec4f(0.f, 0.f, 0.f, 1.f));
    light->setPosition(osg::Vec4f(0.f, 0.f, 1.f, 0.f));
    light->setDiffuse(osg::Vec4f(1.f, 1.f, 1.f, 1.f));
    light->setSpecular(osg::Vec4f(0.f, 0.f, 0.f, 0.f));
    light->setConstantAttenuation(1.f);

    mLightSource->setLight(light);

    mRootNode->addChild(mLightSource);

    updateDayNightMode(0);
}

void CSVRender::LightingBright::deactivate()
{
    if (mRootNode && mLightSource.get())
        mRootNode->removeChild(mLightSource);
}

osg::Vec4f CSVRender::LightingBright::getAmbientColour(osg::Vec4f* /*defaultAmbient*/)
{
    return osg::Vec4f(1.f, 1.f, 1.f, 1.f);
}
