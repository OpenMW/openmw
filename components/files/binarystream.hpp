#ifndef OPENMW_COMPONENTS_FILES_BINARYSTREAM_HPP
#define OPENMW_COMPONENTS_FILES_BINARYSTREAM_HPP

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <format>
#include <istream>
#include <span>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <typeinfo>

#include <components/files/istreamptr.hpp>
#include <components/misc/endianness.hpp>

namespace Files
{
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
                readArray(std::span(&data, 1));
            else
                readArray(std::span(data._v));
        }

        /// Read numbers into any contiguous storage
        template <class T, std::size_t extent>
        void readArray(std::span<T, extent> dest)
        {
            static_assert(std::is_arithmetic_v<T>, "Buffer element type is not arithmetic");
            mStream->read(reinterpret_cast<char*>(dest.data()), dest.size_bytes());
            if (mStream->fail())
                throw std::runtime_error(std::format("Failed to read typed ({}) buffer of {} instances: {}",
                    typeid(T).name(), dest.size(), std::generic_category().message(errno)));
            if constexpr (Misc::IS_BIG_ENDIAN)
                for (T& value : dest)
                    Misc::swapEndiannessInplace(value);
        }

        /// Read a u32 length and that many bytes, refusing lengths above maxLength
        std::string readSizedString(std::uint32_t maxLength);

    protected:
        IStreamPtr mStream;
    };
}

#endif
