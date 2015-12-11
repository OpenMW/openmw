#include <boost/foreach.hpp>
#include <osg/Version>
#include <components/sceneutil/riggeometry.hpp>

#define SERIALIZER_DEBUG 0
#include "serializer.hpp"

typedef SceneUtil::RigGeometry::InfluenceMap InfluenceMap;
typedef SceneUtil::RigGeometry::InfluenceMapType InfluenceMapType;
typedef SceneUtil::RigGeometry::BoneWeightMap BoneWeightMap;

static bool checkInfluence(const SceneUtil::RigGeometry& node) {
    CHECKMSG("Influence");
    const osg::ref_ptr<const InfluenceMap> imap = node.getInfluenceMap();
    if (imap && imap->mMap.size() > 0) return true;
    return false;
}

static bool writeInfluence(osgDB::OutputStream& os,
                           const SceneUtil::RigGeometry& node) {
    WRITEMSG("Influence");
    const osg::ref_ptr<const InfluenceMap> imap = node.getInfluenceMap();
    if (!imap) return false; 
    os << imap->mMap.size() << os.BEGIN_BRACKET << std::endl;
    BOOST_FOREACH(const InfluenceMapType::value_type& pair, imap->mMap) {
        os.writeWrappedString(pair.first);
        os << os.BEGIN_BRACKET << std::endl;
        const SceneUtil::RigGeometry::BoneInfluence& bone = pair.second;
        os << os.PROPERTY("InvBindMatrix") << bone.mInvBindMatrix;
#if OSG_VERSION_GREATER_OR_EQUAL(3,3,3)
        // Added in commit: 8b485f0b588e6311a24b90711ae548e49e0468ae
        os << os.PROPERTY("BoundSphere") << bone.mBoundSphere << std::endl;
#else
        float x = bone.mBoundSphere.center().x();
        float y = bone.mBoundSphere.center().y();
        float z = bone.mBoundSphere.center().z();
        float r = bone.mBoundSphere.radius();
        os << os.PROPERTY("BoundSphere") << x << y << z << r << std::endl;
#endif
        os << os.PROPERTY("Weights") << bone.mWeights.size() << os.BEGIN_BRACKET << std::endl;
        BOOST_FOREACH(const BoneWeightMap::value_type& wpair, bone.mWeights) {
            os << wpair.first << wpair.second << std::endl;
        }
        os << os.END_BRACKET << std::endl;

        os << os.END_BRACKET << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool readInfluence(osgDB::InputStream& is,
                          SceneUtil::RigGeometry& node) {
    READMSG("Influence");
    size_t size;
    is >> size;
    osg::ref_ptr<InfluenceMap> imap = osg::ref_ptr<InfluenceMap>(new InfluenceMap);
    is >> is.BEGIN_BRACKET;
    for (size_t i = 0; i < size; i++) {
        SceneUtil::RigGeometry::BoneInfluence bone;
        std::string key;
        is.readWrappedString(key);
        is >> is.BEGIN_BRACKET;
        is >> is.PROPERTY("InvBindMatrix") >> bone.mInvBindMatrix;
#if OSG_VERSION_GREATER_OR_EQUAL(3,3,3)
        // Added in commit: 8b485f0b588e6311a24b90711ae548e49e0468ae
        is >> is.PROPERTY("BoundSphere") >> bone.mBoundSphere;
#else
        float x, y, z, r;
        is >> is.PROPERTY("BoundSphere") >> x >> y >> z >> r;
        bone.mBoundSphere.set(osg::Vec3f(x, y, z), r);
#endif
        size_t weight_size;
        is >> is.PROPERTY("Weights") >> weight_size >> is.BEGIN_BRACKET;
        for (size_t j = 0; j < weight_size; j++) {
            BoneWeightMap::key_type index;
            BoneWeightMap::mapped_type weight;
            is >> index >> weight;
            bone.mWeights[index] = weight;
        }
        is >> is.END_BRACKET;
        
        is >> is.END_BRACKET;
        imap->mMap[key] = bone;
    }
    is >> is.END_BRACKET;
    node.setInfluenceMap(imap);
    return true;
}

static bool checkSourceGeometry(const SceneUtil::RigGeometry& node) {
    CHECKMSG("SourceGeometry");
    return true;
}

static bool writeSourceGeometry(osgDB::OutputStream& os,
                                const SceneUtil::RigGeometry& node) {
    WRITEMSG("SourceGeometry");
    os << os.BEGIN_BRACKET << std::endl;
    os.writeObject(node.getSourceGeometry());
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool readSourceGeometry(osgDB::InputStream& is,
                               SceneUtil::RigGeometry& node) {
    READMSG("SourceGeometry");
    is >> is.BEGIN_BRACKET;
#if OSG_VERSION_GREATER_OR_EQUAL(3,3,3)
    osg::ref_ptr<osg::Geometry> geom = is.readObjectOfType<osg::Geometry>();
#else
    osg::ref_ptr<osg::Geometry> geom;
    is >> geom;
#endif
    node.setSourceGeometry(geom);
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER2(SceneUtil_RigGeometry_Serializer,
                         new SceneUtil::RigGeometry,
                         SceneUtil::RigGeometry,
                         "OpenMW::RigGeometry",
                         "osg::Object osg::Drawable osg::Geometry OpenMW::RigGeometry")
{
    SETUPMSG("OpenMW::RigGeometry");
    ADD_USER_SERIALIZER(Influence);
    ADD_USER_SERIALIZER(SourceGeometry);
}
