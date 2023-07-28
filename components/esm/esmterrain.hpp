#ifndef COMPONENTS_ESM_ESMTERRAIN
#define COMPONENTS_ESM_ESMTERRAIN

#include <span>
#include <vector>

#include <components/esm3/loadland.hpp>
#include <components/esm4/loadland.hpp>

namespace ESM
{
    class LandData
    {

    public:
        ~LandData() = default;
        LandData() = default;
        LandData(const ESM::Land& Land, int loadFLags);
        LandData(const ESM4::Land& Land, int loadFLags);

        typedef signed char VNML;

        std::span<const float> getHeights() const { return mHeights; }
        std::span<const VNML> getNormals() const { return mNormals; }
        std::span<const unsigned char> getColors() const { return mColors; }
        std::span<const uint16_t> getTextures() const { return mTextures; }
        float getSize() const { return mSize; }
        float getMinHeight() const { return mMinHeight; }
        float getMaxHeight() const { return mMaxHeight; }
        int getLandSize() const { return mLandSize; }

        int mLoadFlags = 0;

    private:
        float mMinHeight = 0.f;
        float mMaxHeight = 0.f;
        float mSize = 0.f;
        int mLandSize = 0;
        std::vector<float> mHeights;
        std::vector<VNML> mNormals;
        std::vector<unsigned char> mColors;
        std::vector<uint16_t> mTextures;
    };

}
#endif // ! COMPNENTS_ESM_ESMTERRAIN
