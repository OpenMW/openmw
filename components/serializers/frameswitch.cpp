#include <components/nifosg/nifloader.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

REGISTER_OBJECT_WRAPPER2(NifOsg_FrameSwitch_Serializer,
                         new NifOsg::FrameSwitch,
                         NifOsg::FrameSwitch,
                         "OpenMW::FrameSwitch",
                         "osg::Object osg::Node osg::Group OpenMW::FrameSwitch")
{
    SETUPMSG("OpenMW::FrameSwitch");
    // There are no members in the FrameSwitch callback.  It just needs to be created?
    // Scrawl said we didn't need to store this one in the object tree at all, while it
    // is, which seemed an appropriate way to suppress warnings about the lack of
    // serializer.
}
