#ifndef OPENMW_COMPONENTS_NIF_NIFSTREAM_HPP
#define OPENMW_COMPONENTS_NIF_NIFSTREAM_HPP

#include <boost/static_assert.hpp>

namespace Nif
{

class NIFFile;

class NIFStream {

    /// Input stream
    Ogre::DataStreamPtr inp;

    template <typename value_t>
    value_t read_le ()
    {
        uint8_t buffer [sizeof (value_t)];

        if (inp->read (buffer, sizeof (buffer)) != sizeof (buffer))
            throw std::runtime_error ("unexpected");

        value_t Value = 0;
        value_t Shift = 0;

        for (size_t i = 0; i < sizeof (value_t); ++i)
        {
            Value |= value_t (buffer[i]) << Shift;
            Shift += 8;
        }

        return Value;
    }

public:

    /*
     *  This should be true for any processor/platform whose endianess, alignment
     *  and packing rules would be compatible with x86 and not fault or degrade
     *  with misaligned reads. This enables some pretty big savings when reading in
     *  animations and meshes.
     */
#if defined (__i386__) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64)
    static const bool FileCompatiblePlatform = true;
#else
    static const bool FileCompatiblePlatform = false;
#endif

    template <typename type>
    struct handler;

    template <typename presentation_type, typename backing_type>
    struct le_handler;

    template <typename presentation_type, typename backing_type>
    friend struct le_handler;

    template <typename store_type, typename final_type, bool SameLayout>
    struct conversion_handler;

    NIFFile * const file;

    NIFStream (NIFFile * file, Ogre::DataStreamPtr inp) : file (file), inp (inp) {}

    /*************************************************
               Parser functions
    ****************************************************/

    void skip(size_t size) { return inp->skip (size); }
    size_t read (void * data, size_t size) { return inp->read (data, size); }

    template <typename T>
    void uncheckedRead (T & Value)
    {
        typedef handler <T> handler_t;
        handler_t::extract (*this, Value);
    }

    template <typename T>
    T getValue ()
    {
        T Value;
        getValue (Value);
        return Value;
    }

    template <typename T>
    void getValue (T & Value)
    {
        typedef handler <T> handler_t;
        if (FileCompatiblePlatform && handler_t::FileCompatibleLayout)
        {
            BOOST_STATIC_ASSERT_MSG (handler_t::FixedLength, "non-fixed length encoding not supported...");
            BOOST_STATIC_ASSERT_MSG (handler_t::EncodedLength == sizeof (T), "unexpected structure size");

            inp->read (&Value, handler_t::EncodedLength);
        }
        else
        {
            handler_t::extract (*this, Value);
        }
    }

    template <typename element_type>
    void getArray (element_type * Array, size_t Size)
    {
        typedef handler <element_type> handler_t;

        if (FileCompatiblePlatform && handler_t::FileCompatibleLayout)
        {
            BOOST_STATIC_ASSERT_MSG (handler_t::FixedLength, "non-fixed length encoding not supported...");
            BOOST_STATIC_ASSERT_MSG (handler_t::EncodedLength == sizeof (element_type), "unexpected structure size");

            inp->read (Array, handler_t::EncodedLength * Size);
        }
        else
        {
            for(size_t i = 0; i < Size; i++)
                handler_t::extract (*this, Array[i]);
        }
    }

    template <typename element_type, typename allocator_type>
    void getStdVector (std::vector <element_type, allocator_type> & Vector, size_t Size)
    {
        Vector.resize(Size);

        getArray (&Vector.front (), Vector.size ());
    }

    template <typename length_type, typename element_type, typename allocator_type>
    void getStdVector (std::vector <element_type, allocator_type> & Vector)
    {
        length_type Length;

        getValue (Length);

        getStdVector (Vector, Length);
    }

    char getChar()              { return getValue <char> (); }
    signed int getInt()         { return getValue <int32_t> (); }
    unsigned int getUInt()      { return getValue <uint32_t> (); }
    signed short getShort()     { return getValue <int16_t> (); }
    unsigned short getUShort()  { return getValue <uint16_t> (); }
    //signed long getLong()     { return getValue <signed long> (); }
    //unsigned long getULong()  { return getValue <unsigned long> (); }
    float getFloat()            { return getValue <float> (); }

    Ogre::Vector2 getVector2()  { return getValue <Ogre::Vector2> (); }
    Ogre::Vector3 getVector3()  { return getValue <Ogre::Vector3> (); }
    Ogre::Vector4 getVector4()  { return getValue <Ogre::Vector4> (); }
    Ogre::Matrix3 getMatrix3()  { return getValue <Ogre::Matrix3> (); }
    Ogre::Quaternion getQuaternion() { return getValue <Ogre::Quaternion> (); }

    Transformation getTrafo()   { return getValue <Transformation> (); }

    void getShorts(std::vector<short> &vec, size_t size)                 { return getStdVector (vec, size); }
    void getFloats(std::vector<float> &vec, size_t size)                 { return getStdVector (vec, size); }
    void getVector2s(std::vector<Ogre::Vector2> &vec, size_t size)       { return getStdVector (vec, size); }
    void getVector3s(std::vector<Ogre::Vector3> &vec, size_t size)       { return getStdVector (vec, size); }
    void getVector4s(std::vector<Ogre::Vector4> &vec, size_t size)       { return getStdVector (vec, size); }
	void getQuaternions(std::vector<Ogre::Quaternion> &vec, size_t size) { return getStdVector (vec, size); }

    std::string getString(size_t length)
    {
        std::vector<char> str (length+1, 0);

        if(read(&str[0], length) != length)
            throw std::runtime_error ("string length in NIF file does not match");

        return &str[0];
    }
    std::string getString()
    {
        size_t size = getValue <uint32_t> ();
        return getString(size);
    }
};

/*
 * generic type handlers
 */

template <typename type, size_t Size>
struct NIFStream::handler < type [Size] >
{
    typedef handler <type> inner_handler;

    static const bool   FixedLength          = inner_handler::FixedLength;
    static const size_t EncodedLength        = inner_handler::EncodedLength * Size;
    static const bool   FileCompatibleLayout = inner_handler::FileCompatibleLayout;

    static void extract (NIFStream & Stream, type (&Value) [Size])
    {
        for (size_t i = 0; i < Size; ++i)
            inner_handler::extract (Stream, Value [i]);
    }
};

template <typename presentation_type, typename backing_type>
struct NIFStream::le_handler
{
    static const bool   FixedLength          = true;
    static const size_t EncodedLength        = sizeof (backing_type);
    static const bool   FileCompatibleLayout = true;

    static void extract (NIFStream & Stream, presentation_type & Value)
    {
        BOOST_STATIC_ASSERT_MSG(sizeof (presentation_type) == sizeof (backing_type), "Invalid use of NIFile::le_handler template");

        union {

            backing_type        Backing;
            presentation_type   Presentation;

        } u;
            
        u.Backing = Stream.read_le <backing_type> ();

        Value = u.Presentation;
    }
};

template <typename store_type, typename final_type>
struct NIFStream::conversion_handler <store_type, final_type, false>
{
    typedef handler <store_type> store_handler;

    static const bool   FixedLength          = store_handler::FixedLength;
    static const size_t EncodedLength        = store_handler::EncodedLength;
    static const bool   FileCompatibleLayout = false;

    static void extract (NIFStream & Stream, final_type & Value)
    {
        store_type StoreValue;
        store_handler::extract (Stream, StoreValue);
        Value = final_type (StoreValue);
    }
};

template <typename store_type, typename final_type>
struct NIFStream::conversion_handler <store_type, final_type, true>
{
    typedef handler <store_type> store_handler;

    static const bool   FixedLength          = store_handler::FixedLength;
    static const size_t EncodedLength        = store_handler::EncodedLength;
    static const bool   FileCompatibleLayout = store_handler::FileCompatibleLayout;

    static void extract (NIFStream & Stream, final_type & FinalValue)
    {
        store_handler::extract (Stream, reinterpret_cast <store_type &> (FinalValue));
    }
};

template <> struct NIFStream::handler <int8_t>              : NIFStream::le_handler <int8_t,    uint8_t> {};
template <> struct NIFStream::handler <uint8_t>             : NIFStream::le_handler <uint8_t,   uint8_t> {};
template <> struct NIFStream::handler <int16_t>             : NIFStream::le_handler <int16_t,   uint16_t> {};
template <> struct NIFStream::handler <uint16_t>            : NIFStream::le_handler <uint16_t,  uint16_t> {};
template <> struct NIFStream::handler <int32_t>             : NIFStream::le_handler <int32_t,   uint32_t> {};
template <> struct NIFStream::handler <uint32_t>            : NIFStream::le_handler <uint32_t,  uint32_t> {};

template <> struct NIFStream::handler <char>                : NIFStream::le_handler <char,      uint8_t> {};
template <> struct NIFStream::handler <float>               : NIFStream::le_handler <float,     uint32_t> {};

template <> struct NIFStream::handler <Ogre::Vector2>       : NIFStream::conversion_handler <float[2],      Ogre::Vector2,      true> {};
template <> struct NIFStream::handler <Ogre::Vector3>       : NIFStream::conversion_handler <float[3],      Ogre::Vector3,      true> {};
template <> struct NIFStream::handler <Ogre::Vector4>       : NIFStream::conversion_handler <float[4],      Ogre::Vector4,      true> {};
template <> struct NIFStream::handler <Ogre::Matrix3>       : NIFStream::conversion_handler <float[3][3],   Ogre::Matrix3,      true> {};
template <> struct NIFStream::handler <Ogre::Quaternion>    : NIFStream::conversion_handler <float[4],      Ogre::Quaternion,   true> {};

template <> struct NIFStream::handler <Transformation>
{
    static const bool FixedLength = true;
    static const size_t EncodedLength =
        handler <Ogre::Vector3>::EncodedLength + 
        handler <Ogre::Matrix3>::EncodedLength + 
        handler <Ogre::Real>::EncodedLength;
    static const bool FileCompatibleLayout = true;

    static void extract (NIFStream & stream, Transformation & value)
    {
        stream.uncheckedRead (value.pos);
        stream.uncheckedRead (value.rotation);
        stream.uncheckedRead (value.scale);
    }
};

}

#endif
