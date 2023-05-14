#ifndef COMPONENTS_ESM_ESMTERRAIN
#define COMPONENTS_ESM_ESMTERRAIN

#include <span>

namespace ESM
{
    class LandData
    {

    public:
        typedef signed char VNML;

        virtual std::span<const float> getHeights() const = 0;
        virtual std::span<const VNML> getNormals() const = 0;
        virtual std::span<const unsigned char> getColors() const = 0;
        virtual std::span<const uint16_t> getTextures() const = 0;
        virtual float getSize() const = 0;
        virtual float getMinHeight() const = 0;
        virtual float getMaxHeight() const = 0;
        virtual int getLandSize() const = 0;
    };

}
#endif // ! COMPNENTS_ESM_ESMTERRAIN
