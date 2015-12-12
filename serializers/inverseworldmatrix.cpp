#include <components/nifosg/particle.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

REGISTER_OBJECT_WRAPPER2(NifOsg_InverseWorldMatrix_Serializer,
                         new NifOsg::InverseWorldMatrix,
                         NifOsg::InverseWorldMatrix,
                         "OpenMW::InverseWorldMatrix",
                         "osg::Object osg::NodeCallback OpenMW::InverseWorldMatrix")
{
    SETUPMSG("OpenMW::InverseWorldMatrix");
    // There are no members in the InverseWorldMatrix callback.  It just needs to be created?
}
