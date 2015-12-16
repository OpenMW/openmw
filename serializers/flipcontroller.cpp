// Virtual because of SceneUtil::StateSetUpdater and SceneUtil::Controller
#define OBJECT_CAST dynamic_cast

#include <boost/foreach.hpp>
#include <components/nifosg/controller.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

typedef std::vector<osg::ref_ptr<osg::Texture2D> > TextureVector;

REGISTER_OBJECT_WRAPPER2(NifOsg_FlipController_Serializer,
                         new NifOsg::FlipController,
                         NifOsg::FlipController,
                         "OpenMW::FlipController",
                         "osg::Object osg::NodeCallback OpenMW::StateSetUpdater OpenMW::Controller OpenMW::FlipController")
{
    SETUPMSG("OpenMW::FlipController");
    ADD_INT_SERIALIZER(TexSlot, 0);
    ADD_FLOAT_SERIALIZER(Delta, 0.0f);
    ADD_LIST_SERIALIZER(Textures, TextureVector);
}
