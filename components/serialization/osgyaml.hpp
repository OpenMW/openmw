#ifndef OPENMW_COMPONENTS_SERIALIZATION_OSGYAML_H
#define OPENMW_COMPONENTS_SERIALIZATION_OSGYAML_H

#include <yaml-cpp/yaml.h>

#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>

namespace Serialization
{
    template <class OSGVec>
    YAML::Node encodeOSGVec(const OSGVec& rhs)
    {
        YAML::Node node;
        for (int i = 0; i < OSGVec::num_components; ++i)
            node.push_back(rhs[i]);

        return node;
    }

    template <class OSGVec>
    bool decodeOSGVec(const YAML::Node& node, OSGVec& rhs)
    {
        if (!node.IsSequence() || node.size() != OSGVec::num_components)
            return false;

        for (int i = 0; i < OSGVec::num_components; ++i)
            rhs[i] = node[i].as<typename OSGVec::value_type>();

        return true;
    }
}

namespace YAML
{

    template<>
    struct convert<osg::Vec2f>
    {
        static Node encode(const osg::Vec2f& rhs) { return Serialization::encodeOSGVec(rhs); }

        static bool decode(const Node& node, osg::Vec2f& rhs) { return Serialization::decodeOSGVec(node, rhs); }
    };

    template<>
    struct convert<osg::Vec3f>
    {
        static Node encode(const osg::Vec3f& rhs) { return Serialization::encodeOSGVec(rhs); }

        static bool decode(const Node& node, osg::Vec3f& rhs) { return Serialization::decodeOSGVec(node, rhs); }
    };

    template<>
    struct convert<osg::Vec4f>
    {
        static Node encode(const osg::Vec4f& rhs) { return Serialization::encodeOSGVec(rhs); }

        static bool decode(const Node& node, osg::Vec4f& rhs) { return Serialization::decodeOSGVec(node, rhs); }
    };

}

#endif