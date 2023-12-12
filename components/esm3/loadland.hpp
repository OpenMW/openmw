#ifndef OPENMW_ESM_LAND_H
#define OPENMW_ESM_LAND_H

#include <array>
#include <cstdint>
#include <memory>

#include <components/misc/constants.hpp>

#include "components/esm/defs.hpp"
#include "components/esm/esmcommon.hpp"

#include "landrecorddata.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    /*
     * Landscape data.
     */

    struct Land
    {
        constexpr static RecNameInts sRecordId = REC_LAND;

        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "Land"; }

        Land() = default;

        Land(const Land& land);

        Land(Land&& other) = default;

        Land& operator=(const Land& land);

        Land& operator=(Land&& land) = default;

        // Only first four bits seem to be used, don't know what they mean.
        std::uint32_t mFlags = 0;
        // Map coordinates.
        std::int32_t mX = 0;
        std::int32_t mY = 0;

        // Plugin index, used to reference the correct material palette.
        int getPlugin() const { return mContext.index; }
        void setPlugin(int index) { mContext.index = index; }

        // File context. This allows the ESM reader to be 'reset' to this
        // location later when we are ready to load the full data set.
        // In the editor, there may not be a file associated with the Land,
        // in which case the filename will be empty.
        ESM_Context mContext;

        int mDataTypes = 0;

        enum
        {
            DATA_VNML = 1,
            DATA_VHGT = 2,
            DATA_WNAM = 4,
            DATA_VCLR = 8,
            DATA_VTEX = 16
        };

        enum
        {
            FLAG_HEIGHT = 1,
            FLAG_COLOR = 2,
            FLAG_TEXTURE = 4
        };

        // default height to use in case there is no Land record
        static constexpr int DEFAULT_HEIGHT = -2048;

        // number of vertices per side
        static constexpr int LAND_SIZE = LandRecordData::sLandSize;

        // cell terrain size in world coords
        static constexpr int REAL_SIZE = Constants::CellSizeInUnits;

        // total number of vertices
        static constexpr int LAND_NUM_VERTS = LandRecordData::sLandNumVerts;

        static constexpr int HEIGHT_SCALE = 8;

        // number of textures per side of land
        static constexpr int LAND_TEXTURE_SIZE = LandRecordData::sLandTextureSize;

        // total number of textures per land
        static constexpr int LAND_NUM_TEXTURES = LandRecordData::sLandNumTextures;

        static constexpr unsigned LAND_GLOBAL_MAP_LOD_SIZE = 81;

        static constexpr unsigned LAND_GLOBAL_MAP_LOD_SIZE_SQRT = 9;

        using LandData = ESM::LandRecordData;

        // low-LOD heightmap (used for rendering the global map)
        std::array<std::int8_t, LAND_GLOBAL_MAP_LOD_SIZE> mWnam;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();

        void loadData(int flags) const;

        void loadData(int flags, LandData& data) const;

        /**
         * Frees memory allocated for mLandData
         */
        void unloadData();

        /// Check if given data type is loaded
        bool isDataLoaded(int flags) const;

        /// Return land data with at least the data types specified in \a flags loaded (if they
        /// are available). Will return a 0-pointer if there is no data for any of the
        /// specified types.
        const LandData* getLandData(int flags) const;

        /// Return land data without loading first anything. Can return a 0-pointer.
        const LandData* getLandData() const
        {
            return mLandData.get();
        }

        /// Return land data without loading first anything. Can return a 0-pointer.
        LandData* getLandData()
        {
            return mLandData.get();
        }

        /// \attention Must not be called on objects that aren't fully loaded.
        ///
        /// \note Added data fields will be uninitialised
        void add(int flags);

    private:
        mutable std::unique_ptr<LandData> mLandData;
    };

}
#endif
