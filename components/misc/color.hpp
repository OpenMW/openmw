#ifndef COMPONENTS_MISC_COLOR
#define COMPONENTS_MISC_COLOR

#include <string>

namespace Misc
{
    class Color
    {
        public:
            Color(float r, float g, float b, float a);

            float r() const { return mR; }
            float g() const { return mG; }
            float b() const { return mB; }
            float a() const { return mA; }

            std::string toString() const;

            static Color fromHex(std::string_view hex);

            std::string toHex() const;

            friend bool operator==(const Color& l, const Color& r);

        private:
            float mR;
            float mG;
            float mB;
            float mA;
    };
}

#endif // !COMPONENTS_MISC_COLOR
