// Virtual because of SceneUtil::Controller
#define OBJECT_CAST dynamic_cast

#include <boost/foreach.hpp>
#include <components/nifosg/controller.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"
#include "keymap.hpp"

SERIALIZER_KEYMAPT(Data, NifOsg::FloatInterpolator::InnerMapType, float, NifOsg::AlphaController)

REGISTER_OBJECT_WRAPPER2(NifOsg_AlphaController_Serializer,
                         new NifOsg::AlphaController,
                         NifOsg::AlphaController,
                         "OpenMW::AlphaController",
                         "osg::Object osg::NodeCallback OpenMW::StateSetUpdater OpenMW::Controller OpenMW::AlphaController")
{
    SETUPMSG("OpenMW::AlphaController");
    ADD_USER_SERIALIZER(Data);
}
