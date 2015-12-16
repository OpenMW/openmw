// Virtual because of virtual osg::Object
#define OBJECT_CAST dynamic_cast

#include <boost/foreach.hpp>
#include <components/sceneutil/controller.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

REGISTER_OBJECT_WRAPPER2(SceneUtil_Controller_Serializer,
                         new SceneUtil::Controller,
                         SceneUtil::Controller,
                         "OpenMW::Controller",
                         "osg::Object OpenMW::Controller")
{
    SETUPMSG("OpenMW::Controller");
    ADD_OBJECT_SERIALIZER(Source, SceneUtil::ControllerSource, NULL);
    ADD_OBJECT_SERIALIZER(Function, SceneUtil::ControllerFunction, NULL);
}
