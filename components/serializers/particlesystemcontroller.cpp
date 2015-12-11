// Virtual because of SceneUtil::Controller
#define OBJECT_CAST dynamic_cast

#include <boost/foreach.hpp>
#include <components/nifosg/controller.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

REGISTER_OBJECT_WRAPPER2(NifOsg_ParticleSystemController_Serializer,
                         new NifOsg::ParticleSystemController,
                         NifOsg::ParticleSystemController,
                         "OpenMW::ParticleSystemController",
                         "osg::Object osg::NodeCallback OpenMW::ParticleSystemController")
{
    SETUPMSG("OpenMW::ParticleSystemController");
    ADD_FLOAT_SERIALIZER(EmitStart, 0.0f);
    ADD_FLOAT_SERIALIZER(EmitStop, 0.0f);
}
