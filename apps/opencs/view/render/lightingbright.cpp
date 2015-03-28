
#include "lightingbright.hpp"

#include <OgreSceneManager.h>

#include <osgViewer/View>

CSVRender::LightingBright::LightingBright() : mView(NULL) {}

void CSVRender::LightingBright::activate (osgViewer::View* view,
 const osg::Vec4f* /*defaultAmbient*/)
{
    mView = view;

    // FIXME: ambient should be applied to LightModel instead of the light

    osg::ref_ptr<osg::Light> light (new osg::Light);
    light->setConstantAttenuation(1.f);
    light->setDirection(osg::Vec3f(0.f, 0.f, -1.f));
    light->setDiffuse(osg::Vec4f(1.f, 1.f, 1.f, 1.f));
    light->setAmbient(osg::Vec4f(1.f, 1.f, 1.f, 1.f));

    mView->setLight(light);
}

void CSVRender::LightingBright::deactivate()
{
}

void CSVRender::LightingBright::setDefaultAmbient (const osg::Vec4f& colour) {}
