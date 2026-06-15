#ifndef OPENMW_COMPONENTS_SPT_STREAM_HPP
#define OPENMW_COMPONENTS_SPT_STREAM_HPP

#include <cstdint>
#include <istream>
#include <string>

#include <components/files/istreamptr.hpp>

#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>

namespace Spt
{
    class SPTStream
    {
        Files::IStreamPtr mStream;

    public:
        explicit SPTStream(Files::IStreamPtr&& stream);

        void skip(size_t size);

        int32_t readInt();
        float readFloat();
        int8_t readByte();
        int16_t readShort();

        void readVec2f(osg::Vec2f& vec);
        void readVec3f(osg::Vec3f& vec);
        void readVec4f(osg::Vec4f& vec);

        int32_t readSection();
        std::string readIntString();
    };
}

#endif