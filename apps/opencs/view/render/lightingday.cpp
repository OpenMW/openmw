#include "lightingday.hpp"

#include <osgViewer/View>

CSVRender::LightingDay::LightingDay() : mView(NULL) {}

void CSVRender::LightingDay::activate (osgViewer::View* view,
                                       const osg::Vec4f *defaultAmbient)
{
    mView = view;

    osg::ref_ptr<osg::Light> light (new osg::Light);
    light->setDirection(osg::Vec3f(0.f, 0.f, -1.f));
    light->setDiffuse(osg::Vec4f(1.f, 1.f, 1.f, 1.f));
    light->setConstantAttenuation(1.f);

    if (defaultAmbient)
        light->setAmbient(*defaultAmbient);
    else
        light->setAmbient(osg::Vec4f(0.7f, 0.7f, 0.7f, 1.f));

    mView->setLight(light);
}

void CSVRender::LightingDay::deactivate()
{
}

void CSVRender::LightingDay::setDefaultAmbient (const osg::Vec4f& colour)
{
    if (mView)
        mView->getLight()->setAmbient(colour);
}
