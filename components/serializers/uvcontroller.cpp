// Virtual because of SceneUtil::Controller
#define OBJECT_CAST dynamic_cast

#include <boost/foreach.hpp>
#include <components/nifosg/nifloader.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"
#include "keymap.hpp"

SERIALIZER_KEYMAPT(UTrans, NifOsg::FloatInterpolator::InnerMapType, float, NifOsg::UVController)
SERIALIZER_KEYMAPT(VTrans, NifOsg::FloatInterpolator::InnerMapType, float, NifOsg::UVController)
SERIALIZER_KEYMAPT(UScale, NifOsg::FloatInterpolator::InnerMapType, float, NifOsg::UVController)
SERIALIZER_KEYMAPT(VScale, NifOsg::FloatInterpolator::InnerMapType, float, NifOsg::UVController)

static bool checkTextureUnits(const NifOsg::UVController& node) {
    CHECKMSG("TextureUnits");
    if (node.mTextureUnits.size() > 0) return true;
    return false;
}

static bool writeTextureUnits(osgDB::OutputStream& os,
                              const NifOsg::UVController& node) {
    WRITEMSG("TextureUnits");
    os << node.mTextureUnits.size() << os.BEGIN_BRACKET << std::endl;
    BOOST_FOREACH(const int i, node.mTextureUnits) {
        os << i << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool readTextureUnits(osgDB::InputStream& is,
                             NifOsg::UVController& node) {
    READMSG("TextureUnits");
    size_t size;
    is >> size;
    is >> is.BEGIN_BRACKET;
    for (size_t i = 0; i < size; i++) {
        int u;
        is >> u;
        node.mTextureUnits.insert(u);
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER2(NifOsg_UVController_Serializer,
                         new NifOsg::UVController,
                         NifOsg::UVController,
                         "OpenMW::UVController",
                         "osg::Object osg::NodeCallback OpenMW::StateSetUpdater OpenMW::UVController")
{
    SETUPMSG("OpenMW::UVController");
    ADD_USER_SERIALIZER(UTrans);
    ADD_USER_SERIALIZER(VTrans);
    ADD_USER_SERIALIZER(UScale);
    ADD_USER_SERIALIZER(VScale);
    ADD_USER_SERIALIZER(TextureUnits);
    // Doesn't work because the ListSerializer calls push_back, not insert. :-(
    //ADD_LIST_SERIALIZER(TextureUnits, std::set<int>); // std::set<int> mTextureUnits;
}
