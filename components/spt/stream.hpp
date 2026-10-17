#ifndef OPENMW_COMPONENTS_SPT_STREAM_HPP
#define OPENMW_COMPONENTS_SPT_STREAM_HPP

#include <cstdint>
#include <format>
#include <istream>
#include <stdexcept>
#include <string>

#include <components/files/istreamptr.hpp>
#include <components/misc/endianness.hpp>

#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>

namespace Spt
{
    struct SPTEOFError : std::runtime_error
    {
        SPTEOFError();
    };

    template <std::size_t numInstances, typename T>
    inline void readBufferOfType(Files::IStreamPtr& pIStream, T* dest)
    {
        static_assert(std::is_arithmetic_v<T>, "Buffer element type is not arithmetic");
        pIStream->read(reinterpret_cast<char*>(dest), numInstances * sizeof(T));
        if (pIStream->fail())
            throw std::runtime_error(std::format("Failed to read typed ({}) buffer of {} instances: {}",
                typeid(T).name(), numInstances, std::generic_category().message(errno)));
        if constexpr (Misc::IS_BIG_ENDIAN)
            for (std::size_t i = 0; i < numInstances; i++)
                Misc::swapEndiannessInplace(dest[i]);
    }

    template <std::size_t numInstances, typename T>
    inline void readBufferOfType(Files::IStreamPtr& pIStream, T (&dest)[numInstances])
    {
        readBufferOfType<numInstances>(pIStream, static_cast<T*>(dest));
    }

    class SPTStream
    {
        Files::IStreamPtr mStream;

    public:
        explicit SPTStream(Files::IStreamPtr&& stream);

        void skip(size_t size) { mStream->ignore(size); }

        template <class T>
            requires std::is_arithmetic_v<T> || std::is_same_v<T, osg::Vec2f> || std::is_same_v<T, osg::Vec3f>
            || std::is_same_v<T, osg::Vec4f>
        void read(T& data)
        {
            if constexpr (std::is_arithmetic_v<T>)
                readBufferOfType<1>(mStream, &data);
            else
                readBufferOfType(mStream, data._v);
        }

        int32_t readSection();
        std::string readIntString();
    };
}

#endif
