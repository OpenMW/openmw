#ifndef COMPONENTS_ESM_ESMTERRAIN
#define COMPONENTS_ESM_ESMTERRAIN

#include <cstdint>
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

        std::span<const float> getHeights() const { return mHeights; }
        std::span<const std::int8_t> getNormals() const { return mNormals; }
        std::span<const std::uint8_t> getColors() const { return mColors; }
        std::span<const std::uint16_t> getTextures() const { return mTextures; }
        float getSize() const { return mSize; }
        float getMinHeight() const { return mMinHeight; }
        float getMaxHeight() const { return mMaxHeight; }
        int getLandSize() const { return mLandSize; }
        int getLoadFlags() const { return mLoadFlags; }

    private:
        int mLoadFlags = 0;
        float mMinHeight = 0.f;
        float mMaxHeight = 0.f;
        float mSize = 0.f;
        int mLandSize = 0;
        std::vector<float> mHeights;
        std::vector<std::int8_t> mNormals;
        std::vector<std::uint8_t> mColors;
        std::vector<std::uint16_t> mTextures;
    };

}
#endif // ! COMPNENTS_ESM_ESMTERRAIN
