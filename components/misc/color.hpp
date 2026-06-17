#ifndef COMPONENTS_MISC_COLOR
#define COMPONENTS_MISC_COLOR

#include <string>

#include <osg/Vec4>

namespace Misc
{
    class Color
    {
        explicit Color(osg::Vec4&& value)
            : mValue(value)
        {
        }

    public:
        Color() = default;
        Color(float r, float g, float b, float a);

        float r() const { return mValue.r(); }
        float g() const { return mValue.g(); }
        float b() const { return mValue.b(); }
        float a() const { return mValue.a(); }

        std::string toString() const;

        static Color fromHex(std::string_view hex);
        static Color fromRGB(unsigned int value);
        static Color fromVec(osg::Vec4f value);

        std::string toHex() const;
        unsigned int toRGBA() const { return mValue.asRGBA(); }
        osg::Vec4f toVec() const { return mValue; }

        friend bool operator==(const Color& l, const Color& r);

    private:
        osg::Vec4 mValue;
    };
}

#endif // !COMPONENTS_MISC_COLOR
