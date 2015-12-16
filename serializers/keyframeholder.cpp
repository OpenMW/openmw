#include <boost/foreach.hpp>
#include <components/nifosg/nifloader.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

static bool checkTextKeyMap(const NifOsg::KeyframeHolder& node) {
    CHECKMSG("TextKeys");
    if (node.mTextKeys.size() == 0) return false;
    return true;
}

static bool writeTextKeyMap(osgDB::OutputStream& os,
                            const NifOsg::KeyframeHolder& node) {
    WRITEMSG("TextKeys");
    os << node.mTextKeys.size() << os.BEGIN_BRACKET << std::endl;
    BOOST_FOREACH(const NifOsg::TextKeyMap::value_type& pair, node.mTextKeys) {
        os << pair.first;
        os.writeWrappedString(pair.second);
        os << std::endl;
#if SERIALIZER_DEBUG==3
        WRITEVALUE << "TextKeys[" << pair.first << "] = " << pair.second << std::endl;
#endif
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool readTextKeyMap(osgDB::InputStream& is,
                           NifOsg::KeyframeHolder& node) {
    READMSG("TextKeys");
    // Improve performance by hinting, since keys should be in order.
    NifOsg::TextKeyMap::iterator it;
    it = node.mTextKeys.begin();

    size_t size;
    is >> size;
    is >> is.BEGIN_BRACKET;
    for (size_t i = 0; i < size; i++) {
        float time;
        is >> time;
        std::string label;
        is.readWrappedString(label);
        NifOsg::TextKeyMap::value_type pair = NifOsg::TextKeyMap::value_type(time, label);
        it = node.mTextKeys.insert(it, pair);
#if SERIALIZER_DEBUG==3
        READVALUE << "TextKeys[" << pair.first << "] = " << pair.second << std::endl;
#endif
    }
    is >> is.END_BRACKET;
    return true;
}

typedef NifOsg::KeyframeHolder::KeyframeControllerMap KFCMap;

static bool checkControllers(const NifOsg::KeyframeHolder& node) {
    CHECKMSG("Controllers");
    if (node.mKeyframeControllers.size() == 0) return false;
    return true;
}

static bool writeControllers(osgDB::OutputStream& os,
                             const NifOsg::KeyframeHolder& node) {
    WRITEMSG("Controllers");
    os << node.mKeyframeControllers.size() << os.BEGIN_BRACKET << std::endl;
    BOOST_FOREACH(const KFCMap::value_type& pair, node.mKeyframeControllers) {
        os.writeWrappedString(pair.first);
#if SERIALIZER_DEBUG==3
        WRITEVALUE << "KeyframeContollers[" << pair.first << "] = ???" << std::endl;
#endif
        os.writeObject(pair.second);
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool readControllers(osgDB::InputStream& is,
                            NifOsg::KeyframeHolder& node) {
    READMSG("Controllers");
    // Improve performance by hinting, since keys should be in order.
    KFCMap::iterator it;
    it = node.mKeyframeControllers.begin();

    size_t size;
    is >> size;
    is >> is.BEGIN_BRACKET;
    for (size_t i = 0; i < size; i++) {
        std::string label;
        is.readWrappedString(label);
#if OSG_VERSION_GREATER_OR_EQUAL(3,3,3)
        osg::ref_ptr<NifOsg::KeyframeController> ctrl = is.readObjectOfType<NifOsg::KeyframeController>();
#else
        // Silence OSG 3.2 compilation failure until I can investigate further.
        //osg::ref_ptr<NifOsg::KeyframeController> ctrl;
        //is >> ctrl;
#endif
        KFCMap::value_type pair = KFCMap::value_type(label, ctrl);
        it = node.mKeyframeControllers.insert(it, pair);
#if SERIALIZER_DEBUG==3
        READVALUE << "KeyframeContollers[" << pair.first << "] = ???" << std::endl;
#endif
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER2(NifOsg_KeyframeHolder_Serializer,
                         new NifOsg::KeyframeHolder,
                         NifOsg::KeyframeHolder,
                         "OpenMW::KeyframeHolder",
                         "osg::Object OpenMW::KeyframeHolder")
{
    SETUPMSG("OpenMW::KeyframeHolder");
    ADD_USER_SERIALIZER(TextKeyMap);
    ADD_USER_SERIALIZER(Controllers);
}
