// Virtual because of SceneUtil::Controller
#define OBJECT_CAST dynamic_cast

#include <boost/foreach.hpp>
#include <components/nifosg/controller.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

static bool checkData(const NifOsg::VisController& node) {
    CHECKMSG("Data");
    if (node.mData.size() == 0) return false;
    return true;
}

static bool writeData(osgDB::OutputStream& os,
                      const NifOsg::VisController& node) {
    WRITEMSG("Data");
    os << node.mData.size() << os.BEGIN_BRACKET << std::endl;
    BOOST_FOREACH(const Nif::NiVisData::VisData& vdata, node.mData) {
        os << vdata.time << vdata.isSet << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool readData(osgDB::InputStream& is,
                     NifOsg::VisController& node) {
    READMSG("Data");
    size_t size;
    is >> size;
    is >> is.BEGIN_BRACKET;
    for (size_t i = 0; i < size; i++) {
        Nif::NiVisData::VisData vdata;
        is >> vdata.time >> vdata.isSet;
        node.mData.push_back(vdata);
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER2(NifOsg_VisController_Serializer,
                         new NifOsg::VisController,
                         NifOsg::VisController,
                         "OpenMW::VisController",
                         "osg::Object osg::NodeCallback OpenMW::Controller OpenMW::VisController")
{
    SETUPMSG("OpenMW::VisController");
    ADD_USER_SERIALIZER(Data);
}
