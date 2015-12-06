//#define SERIALIZER_DEBUG

#ifdef SERIALIZER_DEBUG
#include <iostream>
#endif

#include <boost/foreach.hpp>

#include <osgDB/ObjectWrapper>
#include <osgDB/InputStream>
#include <osgDB/OutputStream>
#include <osgDB/Serializer>
#include "fixes.hpp"

#include <components/nifosg/nifloader.hpp>
#include <components/nifosg/userdata.hpp>

static bool checkTextKeyMap(const NifOsg::TextKeyMapHolder& node) {
    if (node.mTextKeys.size() == 0) return false;
    return true;
}

static bool writeTextKeyMap(osgDB::OutputStream& os,
                            const NifOsg::TextKeyMapHolder& node) {
    os << node.mTextKeys.size() << os.BEGIN_BRACKET << std::endl;
    BOOST_FOREACH(const NifOsg::TextKeyMap::value_type& pair, node.mTextKeys) {
        os << pair.first;
        os.writeWrappedString(pair.second);
        os << std::endl;
#ifdef SERIALIZER_DEBUG
        std::cout << "Wrote NifOsg::TextKeyMapHolder.mTextKeys["
                  << pair.first << "] = " << pair.second << std::endl;
#endif
    }
    os << os.END_BRACKET << std::endl;
#ifdef SERIALIZER_DEBUG
    std::cout << "Wrote NifOsg::TextKeyMapHolder.mTextKeys" << std::endl;
#endif
    return true;
}

static bool readTextKeyMap(osgDB::InputStream& is,
                           NifOsg::TextKeyMapHolder& node) {
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
#ifdef SERIALIZER_DEBUG
        std::cout << "Read NifOsg::TextKeyMapHolder.mTextKeys["
                  << pair.first << "] = " << pair.second << std::endl;
#endif
    }
    is >> is.END_BRACKET;
#ifdef SERIALIZER_DEBUG
    std::cout << "Read NifOsg::TextKeyMapHolder.mTextKeys" << std::endl;
#endif
    return true;
}

#define MyClass NifOsg::TextKeyMapHolder
REGISTER_OBJECT_WRAPPER(NifOsg_TextKeyMapHolder_Serializer,
                        new NifOsg::TextKeyMapHolder,
                        NifOsg::TextKeyMapHolder,
                        "osg::Object NifOsg::TextKeyMapHolder")
{
#ifdef SERIALIZER_DEBUG
    std::cout << "Setting up NifOsg::TextKeyMapHolder serializer..." << std::endl;
#endif
    ADD_USER_SERIALIZER(TextKeyMap);
    // Complete.
}

#undef MyClass
#define MyClass NifOsg::FrameSwitch
REGISTER_OBJECT_WRAPPER(NifOsg_FrameSwitch_Serializer,
                        new NifOsg::FrameSwitch,
                        NifOsg::FrameSwitch,
                        "osg::Object osg::NodeCallback NifOsg::FrameSwitch")
{
#ifdef SERIALIZER_DEBUG
    std::cout << "Setting up NifOsg::FrameSwitch serializer..." << std::endl;
#endif
    // There are no members in the FrameSwitch callback.  It just needs to be created?
    // Scrawl said we didn't need to store this one in the object tree at all, while it
    // is, which seemed an appropriate way to suppress warnings about the lack of
    // serializer.
}

#undef MyClass
#define MyClass NifOsg::BillboardCallback
REGISTER_OBJECT_WRAPPER(NifOsg_BillboardCallback_Serializer,
                        new NifOsg::BillboardCallback,
                        NifOsg::BillboardCallback,
                        "osg::Object osg::NodeCallback NifOsg::BillboardCallback")
{
#ifdef SERIALIZER_DEBUG
    std::cout << "Setting up NifOsg::BillboardCallback serializer..." << std::endl;
#endif
    // There are no members in the Billboard callback.  It just needs to be created?
}
