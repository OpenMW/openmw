#include <components/nifosg/nifloader.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

REGISTER_OBJECT_WRAPPER2(NifOsg_BillboardCallback_Serializer,
                         new NifOsg::BillboardCallback,
                         NifOsg::BillboardCallback,
                         "OpenMW::BillboardCallback",
                         "osg::Object osg::NodeCallback OpenMW::BillboardCallback")
{
    SETUPMSG("OpenMW::BillboardCallback");
    // There are no members in the Billboard callback.  It just needs to be created?
}
