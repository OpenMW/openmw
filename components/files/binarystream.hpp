#ifndef OPENMW_COMPONENTS_FILES_BINARYSTREAM_HPP
#define OPENMW_COMPONENTS_FILES_BINARYSTREAM_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <istream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <typeinfo>

#include <components/files/istreamptr.hpp>
#include <components/misc/endianness.hpp>

namespace Files
{
    template <std::size_t numInstances, typename T>
    inline void readBufferOfType(IStreamPtr& stream, T* dest)
    {
        static_assert(std::is_arithmetic_v<T>, "Buffer element type is not arithmetic");
        stream->read(reinterpret_cast<char*>(dest), numInstances * sizeof(T));
        if (stream->fail())
            throw std::runtime_error(std::format("Failed to read typed ({}) buffer of {} instances: {}",
                typeid(T).name(), numInstances, std::generic_category().message(errno)));
        if constexpr (Misc::IS_BIG_ENDIAN)
            for (std::size_t i = 0; i < numInstances; i++)
                Misc::swapEndiannessInplace(dest[i]);
    }

    template <std::size_t numInstances, typename T>
    inline void readBufferOfType(IStreamPtr& stream, T (&dest)[numInstances])
    {
        readBufferOfType<numInstances>(stream, static_cast<T*>(dest));
    }

    // Reads little-endian values from a binary file
    class BinaryStream
    {
    public:
        explicit BinaryStream(IStreamPtr&& stream)
            : mStream(std::move(stream))
        {
        }

        void skip(std::size_t size) { mStream->ignore(size); }

        /// Read a single number, or a fixed-size vector such as osg::Vec3f through its _v array
        template <class T>
        void read(T& data)
        {
            if constexpr (std::is_arithmetic_v<T>)
                readBufferOfType<1>(mStream, &data);
            else
                readBufferOfType(mStream, data._v);
        }

        /// Read multiple instances of type into an array
        template <class T, std::size_t size>
        void readArray(std::array<T, size>& arr)
        {
            readBufferOfType<size>(mStream, arr.data());
        }

        /// Read a u32 length and that many bytes, refusing lengths above maxLength
        std::string readSizedString(std::uint32_t maxLength);

    protected:
        IStreamPtr mStream;
    };
}

#endif
