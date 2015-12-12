// Virtual because of SceneUtil::StateSetUpdater
#define OBJECT_CAST dynamic_cast

#include <components/sceneutil/statesetupdater.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

static bool checkCtrls(const SceneUtil::CompositeStateSetUpdater& node) {
    CHECKMSG("Ctrls");
    if (node.getNumControllers() == 0) return false;
    return true;
}

static bool writeCtrls(osgDB::OutputStream& os,
                       const SceneUtil::CompositeStateSetUpdater& node) {
    WRITEMSG("Ctrls");
    unsigned int size = node.getNumControllers();
    os << size << os.BEGIN_BRACKET << std::endl;
    for (unsigned int i = 0; i != size; i++) {
        SceneUtil::StateSetUpdater* ssu = node.getController(i);
        os.writeObject(ssu);
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool readCtrls(osgDB::InputStream& is,
                      SceneUtil::CompositeStateSetUpdater& node) {
    READMSG("Ctrls");
    unsigned int size;
    is >> size;
    is >> is.BEGIN_BRACKET;
    for (unsigned int i = 0; i < size; i++) {
        std::string clsname;
        is >> clsname;
        // ssu = new StateSetUpdater?
        //node.mCtrls.push_back(ssu);
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER2(NifOsg_CompositeStateSetUpdater_Serializer,
                         new SceneUtil::CompositeStateSetUpdater,
                         SceneUtil::CompositeStateSetUpdater,
                         "OpenMW::CompositeStateSetUpdater",
                         "osg::Object osg::NodeCallback OpenMW::StateSetUpdater OpenMW::CompositeStateSetUpdater")
{
    SETUPMSG("OpenMW::CompositeStateSetUpdater");
    ADD_USER_SERIALIZER(Ctrls);
    // No serialization for: std::vector<osg::ref_ptr<StateSetUpdater> > mCtrls;  Transient?
}
