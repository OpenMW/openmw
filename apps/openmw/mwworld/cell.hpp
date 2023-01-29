#ifndef OPENW_MWORLD_CELL
#define OPENW_MWORLD_CELL

#include <osg/Vec2i>

#include <components/esm/esmbridge.hpp>
#include <components/esm/refid.hpp>
#include <components/esm3/cellid.hpp>

namespace ESM
{
    struct Cell;
    struct CellId;
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
        bool isExterior() const { return mFlags.isExterior; }
        bool isQuasiExterior() const { return mFlags.isQuasiExterior; }
        bool hasWater() const { return mFlags.hasWater; }
        bool noSleep() const { return mFlags.noSleep; }
        const ESM::CellId& getCellId() const { return mCellId; }
        const ESM::RefId& getRegion() const { return mRegion; }
        std::string_view getNameId() const { return mNameID; }
        std::string_view getDisplayName() const { return mDisplayname; }
        std::string getDescription() const;
        const MoodData& getMood() const { return mMood; }

    private:
        struct
        {
            bool isExterior;
            bool isQuasiExterior;
            bool hasWater;
            bool noSleep;
        } mFlags;

        osg::Vec2i mGridPos;
        std::string mDisplayname; // How the game displays it
        std::string mNameID; // The name that will be used by the script and console commands
        ESM::RefId mRegion;
        ESM::CellId mCellId;
        MoodData mMood;
    };
}

#endif
