#ifndef OPENMW_COMPONENTS_NIF_NIFSTREAM_HPP
#define OPENMW_COMPONENTS_NIF_NIFSTREAM_HPP

namespace Nif
{

class NIFFile;

class NIFStream {

    /// Input stream
    Ogre::DataStreamPtr inp;

    uint8_t read_byte()
    {
        uint8_t byte;
        if(inp->read(&byte, 1) != 1) return 0;
        return byte;
    }
    uint16_t read_le16()
    {
        uint8_t buffer[2];
        if(inp->read(buffer, 2) != 2) return 0;
        return buffer[0] | (buffer[1]<<8);
    }
    uint32_t read_le32()
    {
        uint8_t buffer[4];
        if(inp->read(buffer, 4) != 4) return 0;
        return buffer[0] | (buffer[1]<<8) | (buffer[2]<<16) | (buffer[3]<<24);
    }
    float read_le32f()
    {
        union {
            uint32_t i;
            float f;
        } u = { read_le32() };
        return u.f;
    }

public:

    NIFFile * const file;

    NIFStream (NIFFile * file, Ogre::DataStreamPtr inp): file (file), inp (inp) {}

    /*************************************************
               Parser functions
    ****************************************************/

    template <typename T>
    struct GetHandler
    {
        typedef T (NIFStream::*fn_t)();

        static const fn_t sValue; // this is specialized per supported type in the .cpp file

        static T read (NIFStream* nif)
        {
            return (nif->*sValue) ();
        }
    };

    template <typename T>
    void read (NIFStream* nif, T & Value)
    {
        Value = GetHandler <T>::read (nif);
    }

    void skip(size_t size) { inp->skip(size); }
    void read (void * data, size_t size) { inp->read (data, size); }

    char getChar() { return read_byte(); }
    short getShort() { return read_le16(); }
    unsigned short getUShort() { return read_le16(); }
    int getInt() { return read_le32(); }
    int getUInt() { return read_le32(); }
    float getFloat() { return read_le32f(); }
    Ogre::Vector2 getVector2()
    {
        float a[2];
        for(size_t i = 0;i < 2;i++)
            a[i] = getFloat();
        return Ogre::Vector2(a);
    }
    Ogre::Vector3 getVector3()
    {
        float a[3];
        for(size_t i = 0;i < 3;i++)
            a[i] = getFloat();
        return Ogre::Vector3(a);
    }
    Ogre::Vector4 getVector4()
    {
        float a[4];
        for(size_t i = 0;i < 4;i++)
            a[i] = getFloat();
        return Ogre::Vector4(a);
    }
    Ogre::Matrix3 getMatrix3()
    {
        Ogre::Real a[3][3];
        for(size_t i = 0;i < 3;i++)
        {
            for(size_t j = 0;j < 3;j++)
                a[i][j] = Ogre::Real(getFloat());
        }
        return Ogre::Matrix3(a);
    }
    Ogre::Quaternion getQuaternion()
    {
        float a[4];
        for(size_t i = 0;i < 4;i++)
            a[i] = getFloat();
        return Ogre::Quaternion(a);
    }
    Transformation getTrafo()
    {
        Transformation t;
        t.pos = getVector3();
        t.rotation = getMatrix3();
        t.scale = getFloat();
        return t;
    }

    std::string getString(size_t length)
    {
        std::vector<char> str (length+1, 0);

        if(inp->read(&str[0], length) != length)
            throw std::runtime_error ("string length in NIF file does not match");

        return &str[0];
    }
    std::string getString()
    {
        size_t size = read_le32();
        return getString(size);
    }

    void getShorts(std::vector<short> &vec, size_t size)
    {
        vec.resize(size);
        for(size_t i = 0;i < vec.size();i++)
            vec[i] = getShort();
    }
    void getFloats(std::vector<float> &vec, size_t size)
    {
        vec.resize(size);
        for(size_t i = 0;i < vec.size();i++)
            vec[i] = getFloat();
    }
    void getVector2s(std::vector<Ogre::Vector2> &vec, size_t size)
    {
        vec.resize(size);
        for(size_t i = 0;i < vec.size();i++)
            vec[i] = getVector2();
    }
    void getVector3s(std::vector<Ogre::Vector3> &vec, size_t size)
    {
        vec.resize(size);
        for(size_t i = 0;i < vec.size();i++)
            vec[i] = getVector3();
    }
    void getVector4s(std::vector<Ogre::Vector4> &vec, size_t size)
    {
        vec.resize(size);
        for(size_t i = 0;i < vec.size();i++)
            vec[i] = getVector4();
    }
};

}

#endif
