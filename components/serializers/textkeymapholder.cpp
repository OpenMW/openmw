#include <boost/foreach.hpp>
#include <components/nifosg/nifloader.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

static bool checkTextKeyMap(const NifOsg::TextKeyMapHolder& node) {
    CHECKMSG("TextKeys");
    if (node.mTextKeys.size() == 0) return false;
    return true;
}

static bool writeTextKeyMap(osgDB::OutputStream& os,
                            const NifOsg::TextKeyMapHolder& node) {
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
                           NifOsg::TextKeyMapHolder& node) {
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

REGISTER_OBJECT_WRAPPER2(NifOsg_TextKeyMapHolder_Serializer,
                         new NifOsg::TextKeyMapHolder,
                         NifOsg::TextKeyMapHolder,
                         "OpenMW::TextKeyMapHolder",
                         "osg::Object OpenMW::TextKeyMapHolder")
{
    SETUPMSG("OpenMW::TextKeyMapHolder");
    ADD_USER_SERIALIZER(TextKeyMap);
    // Complete.
}
