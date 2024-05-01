#ifndef OPENW_MWORLD_CELL
#define OPENW_MWORLD_CELL

#include <osg/Vec2i>

#include <components/esm/esmbridge.hpp>
#include <components/esm/exteriorcelllocation.hpp>
#include <components/esm/refid.hpp>

namespace ESM
{
    struct Cell;
}

namespace ESM4
{
    struct Cell;
}

namespace MWWorld
{
    class CellStore;

    class Cell : public ESM::CellVariant
    {
        struct MoodData
        {
            uint32_t mAmbiantColor;
            uint32_t mDirectionalColor;
            uint32_t mFogColor;
            float mFogDensity;
        };

    public:
        explicit Cell(const ESM4::Cell& cell);
        explicit Cell(const ESM::Cell& cell);

        int getGridX() const { return mGridPos.x(); }
        int getGridY() const { return mGridPos.y(); }
        bool isExterior() const { return mIsExterior; }
        bool isQuasiExterior() const { return mIsQuasiExterior; }
        bool hasWater() const { return mHasWater; }
        bool noSleep() const { return mNoSleep; }
        const ESM::RefId& getRegion() const { return mRegion; }
        std::string_view getNameId() const { return mNameID; }
        std::string_view getDisplayName() const { return mDisplayname; }
        std::string_view getDescription() const { return mDescription; }
        const MoodData& getMood() const { return mMood; }
        float getWaterHeight() const { return mWaterHeight; }
        const ESM::RefId& getId() const { return mId; }
        ESM::RefId getWorldSpace() const { return mIsExterior ? mParent : mId; }

        ESM::ExteriorCellLocation getExteriorCellLocation() const
        {
            return ESM::ExteriorCellLocation(mGridPos.x(), mGridPos.y(), getWorldSpace());
        }

    private:
        bool mIsExterior;
        bool mIsQuasiExterior;
        bool mHasWater;
        bool mNoSleep;

        osg::Vec2i mGridPos;
        std::string mDisplayname; // How the game displays it
        std::string mNameID; // The name that will be used by the script and console commands
        ESM::RefId mRegion;
        ESM::RefId mId;
        ESM::RefId mParent;
        float mWaterHeight;
        std::string mDescription;
        MoodData mMood;
    };
}

#endif
