#ifndef COMPONENTS_ESM_ESMTERRAIN
#define COMPONENTS_ESM_ESMTERRAIN

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace ESM4
{
    struct Land;
}

namespace ESM
{
    struct Land;
    struct LandRecordData;

    class LandData
    {
    public:
        LandData();
        LandData(const ESM::Land& Land, int loadFlags);
        LandData(const ESM4::Land& Land, int loadFlags);

        ~LandData();

        std::span<const float> getHeights() const { return mHeights; }
        std::span<const std::int8_t> getNormals() const { return mNormals; }
        std::span<const std::uint8_t> getColors() const { return mColors; }
        std::span<const std::uint16_t> getTextures() const { return mTextures; }
        float getSize() const { return mSize; }
        float getMinHeight() const { return mMinHeight; }
        float getMaxHeight() const { return mMaxHeight; }
        int getLandSize() const { return mLandSize; }
        int getLoadFlags() const { return mLoadFlags; }
        int getPlugin() const { return mPlugin; }

    private:
        std::unique_ptr<const ESM::LandRecordData> mData;
        int mLoadFlags = 0;
        std::vector<float> mHeightsData;
        float mMinHeight = 0.f;
        float mMaxHeight = 0.f;
        float mSize = 0.f;
        int mLandSize = 0;
        int mPlugin = 0;
        std::span<const float> mHeights;
        std::span<const std::int8_t> mNormals;
        std::span<const std::uint8_t> mColors;
        std::span<const std::uint16_t> mTextures;
    };

}
#endif // ! COMPNENTS_ESM_ESMTERRAIN
