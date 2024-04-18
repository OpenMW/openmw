#ifndef OPENMW_COMPONENTS_BGSM_STREAM_HPP
#define OPENMW_COMPONENTS_BGSM_STREAM_HPP

#include <array>
#include <cassert>
#include <cstdint>
#include <istream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <components/files/istreamptr.hpp>
#include <components/misc/endianness.hpp>

#include <osg/Vec3f>
#include <osg/Vec4f>

namespace Bgsm
{
    class Reader;

    template <std::size_t numInstances, typename T>
    inline void readBufferOfType(Files::IStreamPtr& pIStream, T* dest)
    {
        static_assert(std::is_arithmetic_v<T>, "Buffer element type is not arithmetic");
        pIStream->read((char*)dest, numInstances * sizeof(T));
        if (pIStream->bad())
            throw std::runtime_error("Failed to read typed (" + std::string(typeid(T).name()) + ") buffer of "
                + std::to_string(numInstances) + " instances");
        if constexpr (Misc::IS_BIG_ENDIAN)
            for (std::size_t i = 0; i < numInstances; i++)
                Misc::swapEndiannessInplace(dest[i]);
    }

    template <std::size_t numInstances, typename T>
    inline void readBufferOfType(Files::IStreamPtr& pIStream, T (&dest)[numInstances])
    {
        readBufferOfType<numInstances>(pIStream, static_cast<T*>(dest));
    }

    template <typename T>
    inline void readDynamicBufferOfType(Files::IStreamPtr& pIStream, T* dest, std::size_t numInstances)
    {
        static_assert(std::is_arithmetic_v<T>, "Buffer element type is not arithmetic");
        pIStream->read((char*)dest, numInstances * sizeof(T));
        if (pIStream->bad())
            throw std::runtime_error("Failed to read typed (" + std::string(typeid(T).name()) + ") dynamic buffer of "
                + std::to_string(numInstances) + " instances");
        if constexpr (Misc::IS_BIG_ENDIAN)
            for (std::size_t i = 0; i < numInstances; i++)
                Misc::swapEndiannessInplace(dest[i]);
    }

    class BGSMStream
    {
        const Reader& mReader;
        Files::IStreamPtr mStream;

    public:
        explicit BGSMStream(const Reader& reader, Files::IStreamPtr&& stream)
            : mReader(reader)
            , mStream(std::move(stream))
        {
        }

        const Reader& getFile() const { return mReader; }

        std::uint32_t getVersion() const;

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

        /// Read instances of type into a dynamic buffer
        template <class T>
        void read(T* dest, size_t size)
        {
            readDynamicBufferOfType<T>(mStream, dest, size);
        }

        /// Read multiple instances of type into a vector
        template <class T>
        void readVector(std::vector<T>& vec, size_t size)
        {
            if (size == 0)
                return;
            vec.resize(size);
            read(vec.data(), size);
        }

        /// Extract an instance of type
        template <class T>
        T get()
        {
            T data;
            read(data);
            return data;
        }

        /// Read a string of the given length
        std::string getSizedString(size_t length);

        /// Read a string of the length specified in the file
        std::string getSizedString() { return getSizedString(get<uint32_t>()); }

        /// Read a list of strings
        void getSizedStrings(std::vector<std::string>& vec, size_t size);
    };

    template <>
    void BGSMStream::read<osg::Vec2f>(osg::Vec2f& vec);
    template <>
    void BGSMStream::read<osg::Vec3f>(osg::Vec3f& vec);
    template <>
    void BGSMStream::read<osg::Vec4f>(osg::Vec4f& vec);
    template <>
    void BGSMStream::read<std::string>(std::string& str);

    template <>
    void BGSMStream::read<osg::Vec2f>(osg::Vec2f* dest, size_t size);
    template <>
    void BGSMStream::read<osg::Vec3f>(osg::Vec3f* dest, size_t size);
    template <>
    void BGSMStream::read<osg::Vec4f>(osg::Vec4f* dest, size_t size);
    template <>
    void BGSMStream::read<std::string>(std::string* dest, size_t size);
}

#endif
