//#define SERIALIZER_DEBUG

#ifdef SERIALIZER_DEBUG
#include <iostream>
#endif

#include <boost/foreach.hpp>

// Inconveniently, all of the controller classes are virtual because SceneUtil::Controller
// is virtual, which expects the controller class to override apply() or otehr functions,
// and as a result, we have to switch from static_cast to dynamic_cast in the Serializer
// for these classes.  This situation has been anticipated by the OSG developers, because
// they conveniently(?) provide a #define to change this behavior.
#undef OBJECT_CAST
#define OBJECT_CAST dynamic_cast

#include <osgDB/ObjectWrapper>
#include <osgDB/InputStream>
#include <osgDB/OutputStream>
#include <osgDB/Serializer>
#include "fixes.hpp"

#include <components/nifosg/nifloader.hpp>

#include "keymap.hpp"

SERIALIZER_KEYMAPT(XRotations, NifOsg::FloatInterpolator::InnerMapType, float, NifOsg::KeyframeController)
SERIALIZER_KEYMAPT(YRotations, NifOsg::FloatInterpolator::InnerMapType, float, NifOsg::KeyframeController)
SERIALIZER_KEYMAPT(ZRotations, NifOsg::FloatInterpolator::InnerMapType, float, NifOsg::KeyframeController)
SERIALIZER_KEYMAPT(Scales, NifOsg::FloatInterpolator::InnerMapType, float, NifOsg::KeyframeController)
SERIALIZER_KEYMAPT(Rotations, NifOsg::QuaternionInterpolator::InnerMapType, osg::Quat, NifOsg::KeyframeController)
SERIALIZER_KEYMAPT(Translations, NifOsg::Vec3Interpolator::InnerMapType, osg::Vec3, NifOsg::KeyframeController)

#define MyClass NifOsg::KeyframeController
REGISTER_OBJECT_WRAPPER(NifOsg_KeyframeController_Serializer,
                        new NifOsg::KeyframeController,
                        "OpenMW::KeyframeController",
                        "osg::Object osg::NodeCallback OpenMW::KeyframeController")
{
#ifdef SERIALIZER_DEBUG
    std::cout << "Setting up OpenMW::KeyframeController serializer..." << std::endl;
#endif
    ADD_USER_SERIALIZER(XRotations);
    ADD_USER_SERIALIZER(YRotations);
    ADD_USER_SERIALIZER(ZRotations);
    ADD_USER_SERIALIZER(Scales);
    ADD_USER_SERIALIZER(Rotations);
    ADD_USER_SERIALIZER(Translations);
}

SERIALIZER_KEYMAPT(UTrans, NifOsg::FloatInterpolator::InnerMapType, float, NifOsg::UVController)
SERIALIZER_KEYMAPT(VTrans, NifOsg::FloatInterpolator::InnerMapType, float, NifOsg::UVController)
SERIALIZER_KEYMAPT(UScale, NifOsg::FloatInterpolator::InnerMapType, float, NifOsg::UVController)
SERIALIZER_KEYMAPT(VScale, NifOsg::FloatInterpolator::InnerMapType, float, NifOsg::UVController)

static bool checkTextureUnits(const NifOsg::UVController& node) {
    if (node.mTextureUnits.size() > 0) return true;
    return false;
}

static bool writeTextureUnits(osgDB::OutputStream& os,
                           const NifOsg::UVController& node) {
    os << node.mTextureUnits.size() << os.BEGIN_BRACKET << std::endl;
    BOOST_FOREACH(const int i, node.mTextureUnits) {
        os << i << std::endl;
    }
    os << os.END_BRACKET << std::endl;
#ifdef SERIALIZER_DEBUG
    std::cout << "Wrote OpenMW::UVController.mTextureUnits" << std::endl;
#endif
    return true;
}

static bool readTextureUnits(osgDB::InputStream& is,
                          NifOsg::UVController& node) {
    size_t size;
    is >> size;
    is >> is.BEGIN_BRACKET;
    for (size_t i = 0; i < size; i++) {
        int u;
        is >> u;
        node.mTextureUnits.insert(u);
    }
    is >> is.END_BRACKET;
#ifdef SERIALIZER_DEBUG
    std::cout << "Read OpenMW::UVController.mTextureUnits" << std::endl;
#endif
    return true;
}

#undef MyClass
#define MyClass NifOsg::UVController
REGISTER_OBJECT_WRAPPER(NifOsg_UVController_Serializer,
                        new NifOsg::UVController,
                        "OpenMW::UVController",
                        // Maybe NOT really an osg::Object?  Through StateSetUpdater?
                        "osg::Object OpenMW::UVController")
{
#ifdef SERIALIZER_DEBUG
    std::cout << "Setting up OpenMW::UVController serializer..." << std::endl;
#endif
    ADD_USER_SERIALIZER(UTrans);
    ADD_USER_SERIALIZER(VTrans);
    ADD_USER_SERIALIZER(UScale);
    ADD_USER_SERIALIZER(VScale);
    ADD_USER_SERIALIZER(TextureUnits);
    // Doesn't work because the ListSerializer calls push_back, not insert. :-(
    //ADD_LIST_SERIALIZER(TextureUnits, std::set<int>); // std::set<int> mTextureUnits;
}

#undef MyClass
#define MyClass NifOsg::VisController
REGISTER_OBJECT_WRAPPER(NifOsg_VisController_Serializer,
                        new NifOsg::VisController,
                        "OpenMW::VisController",
                        "osg::Object osg::NodeCallback OpenMW::VisController")
{
#ifdef SERIALIZER_DEBUG
    std::cout << "Setting up OpenMW::VisController serializer..." << std::endl;
#endif
    // std::vector<Nif::NiVisData::VisData> mData;
}

