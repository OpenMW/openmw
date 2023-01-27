#ifndef OPENW_MWORLD_CELL
#define OPENW_MWORLD_CELL

#include <components/esm/esm3esm4bridge.hpp>
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

    struct Cell : public ESM::CellVariant
    {
        friend MWWorld::CellStore;
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
        std::string_view getEditorName() const { return mNameID; }
        std::string getDescription() const;
        const MoodData& getMood() const { return mMood; }

    private:
        struct
        {
            uint8_t isExterior : 1;
            uint8_t isQuasiExterior : 1;
            uint8_t hasWater : 1;
            uint8_t noSleep : 1;
            uint8_t _free : 4;
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
