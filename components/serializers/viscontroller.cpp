// Virtual because of SceneUtil::Controller
#define OBJECT_CAST dynamic_cast

#include <components/nifosg/controller.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

REGISTER_OBJECT_WRAPPER2(NifOsg_VisController_Serializer,
                         new NifOsg::VisController,
                         NifOsg::VisController,
                         "OpenMW::VisController",
                         "osg::Object osg::NodeCallback OpenMW::VisController")
{
    SETUPMSG("OpenMW::VisController");
    // std::vector<Nif::NiVisData::VisData> mData;
}
