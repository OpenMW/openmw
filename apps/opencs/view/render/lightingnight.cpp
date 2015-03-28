#include "lightingnight.hpp"

#include <osgViewer/View>

CSVRender::LightingNight::LightingNight() : mView(NULL) {}

void CSVRender::LightingNight::activate (osgViewer::View* view,
                                         const osg::Vec4f *defaultAmbient)
{
    mView = view;

    osg::ref_ptr<osg::Light> light (new osg::Light);
    light->setDirection(osg::Vec3f(0.f, 0.f, -1.f));
    light->setDiffuse(osg::Vec4f(0.2f, 0.2f, 0.2f, 1.f));
    light->setConstantAttenuation(1.f);

    if (defaultAmbient)
        light->setAmbient(*defaultAmbient);
    else
        light->setAmbient(osg::Vec4f(0.2f, 0.2f, 0.2f, 1.f));

    mView->setLight(light);
}

void CSVRender::LightingNight::deactivate()
{
}

void CSVRender::LightingNight::setDefaultAmbient (const osg::Vec4f& colour)
{
    if (mView)
        mView->getLight()->setAmbient(colour);
}
