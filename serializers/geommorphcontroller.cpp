// Virtual because of SceneUtil::Controller
#define OBJECT_CAST dynamic_cast

#include <boost/foreach.hpp>
#include <components/nifosg/controller.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"
#include "keymap.hpp"

static bool checkKeyFrames(const NifOsg::GeomMorpherController& node) {
    CHECKMSG("KeyFrames");

    if (node.mKeyFrames.size() == 0) return false;

    // This might represent a larger problem.  Why do we have interpolators with no keys?
    int real_size = 0;
    BOOST_FOREACH(const NifOsg::FloatInterpolator interpolator, node.mKeyFrames) {
        if (interpolator.getMapTPtr()->mKeys.size() > 0) real_size++;
    }
    if (real_size == 0) return false;

    return true;
}

static bool writeKeyFrames(osgDB::OutputStream& os,
                           const NifOsg::GeomMorpherController& node) {
    WRITEMSG("KeyFrames");

    // This might represent a larger problem.  Why do we have interpolators with no keys?
    int real_size = 0;
    BOOST_FOREACH(const NifOsg::FloatInterpolator interpolator, node.mKeyFrames) {
        if (interpolator.getMapTPtr()->mKeys.size() > 0) real_size++;
    }

    os << real_size << os.BEGIN_BRACKET << std::endl;
    BOOST_FOREACH(const NifOsg::FloatInterpolator interpolator, node.mKeyFrames) {
        if (interpolator.getMapTPtr()->mKeys.size() == 0) continue;
        os << os.PROPERTY("Interpolator") << os.BEGIN_BRACKET << std::endl;
        os << os.PROPERTY("Interpolation") << interpolator.getMapTPtr()->mInterpolationType << std::endl;
        writeKeyMap<NifOsg::FloatInterpolator::InnerMapType>(os, interpolator.getMapTPtr()->mKeys);
        os << os.END_BRACKET << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool readKeyFrames(osgDB::InputStream& is,
                          NifOsg::GeomMorpherController& node) {
    READMSG("KeyFrames");
    size_t size;
    is >> size;
    is >> is.BEGIN_BRACKET;
    for (size_t i = 0; i < size; i++) {
        NifOsg::FloatInterpolator interpolator;
        interpolator.initMapTPtr();
        is >> is.PROPERTY("Interpolator") >> is.BEGIN_BRACKET;
        is >> is.PROPERTY("Interpolation") >> interpolator.getMapTPtr()->mInterpolationType;
        readKeyMap<NifOsg::FloatInterpolator::InnerMapType, float>(is, interpolator.getMapTPtr()->mKeys);
        is >> is.END_BRACKET;
        node.mKeyFrames.push_back(interpolator);
    }
    is >> is.END_BRACKET;
    return true;
}

#if OSG_VERSION_GREATER_OR_EQUAL(3,3,3)
#define ASSOCIATES "osg::Object osg::Callback osg::UpdateCallback OpenMW::GeomMorpherController"
#else
#define ASSOCIATES "osg::Object OpenMW::GeomMorpherController"
#endif

REGISTER_OBJECT_WRAPPER2(NifOsg_GeomMorpherController_Serializer,
                         new NifOsg::GeomMorpherController,
                         NifOsg::GeomMorpherController,
                         "OpenMW::GeomMorpherController",
                         ASSOCIATES)
{
    SETUPMSG("OpenMW::GeomMorpherController");
    ADD_USER_SERIALIZER(KeyFrames);
}
