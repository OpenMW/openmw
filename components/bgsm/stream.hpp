#ifndef OPENMW_COMPONENTS_BGSM_STREAM_HPP
#define OPENMW_COMPONENTS_BGSM_STREAM_HPP

#include <array>
#include <cassert>
#include <format>
#include <istream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include <components/files/istreamptr.hpp>
#include <components/misc/endianness.hpp>

#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>

namespace Bgsm
{
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

    class BGSMStream
    {
        Files::IStreamPtr mStream;

    public:
        explicit BGSMStream(Files::IStreamPtr&& stream)
            : mStream(std::move(stream))
        {
        }

        void skip(size_t size) { mStream->ignore(size); }

        /// Read into a single instance of type
        template <class T>
        void read(T& data)
        {
            readBufferOfType<1>(mStream, &data);
        }

        /// Read multiple instances of type into an array
        template <class T, size_t size>
        void readArray(std::array<T, size>& arr)
        {
            readBufferOfType<size>(mStream, arr.data());
        }
    };

    template <>
    void BGSMStream::read<osg::Vec2f>(osg::Vec2f& vec);
    template <>
    void BGSMStream::read<osg::Vec3f>(osg::Vec3f& vec);
    template <>
    void BGSMStream::read<osg::Vec4f>(osg::Vec4f& vec);
    template <>
    void BGSMStream::read<std::string>(std::string& str);
}

#endif
