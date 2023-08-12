#ifndef OPENMW_ESM_LAND_H
#define OPENMW_ESM_LAND_H

#include <cstdint>

#include <components/misc/constants.hpp>

#include "components/esm/defs.hpp"
#include "components/esm/esmcommon.hpp"

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
        ~Land();

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

        // default height to use in case there is no Land record
        static constexpr int DEFAULT_HEIGHT = -2048;

        // number of vertices per side
        static constexpr int LAND_SIZE = 65;

        // cell terrain size in world coords
        static constexpr int REAL_SIZE = Constants::CellSizeInUnits;

        // total number of vertices
        static constexpr int LAND_NUM_VERTS = LAND_SIZE * LAND_SIZE;

        static constexpr int HEIGHT_SCALE = 8;

        // number of textures per side of land
        static constexpr int LAND_TEXTURE_SIZE = 16;

        // total number of textures per land
        static constexpr int LAND_NUM_TEXTURES = LAND_TEXTURE_SIZE * LAND_TEXTURE_SIZE;

        static constexpr int LAND_GLOBAL_MAP_LOD_SIZE = 81;

        static constexpr int LAND_GLOBAL_MAP_LOD_SIZE_SQRT = 9;

#pragma pack(push, 1)
        struct VHGT
        {
            float mHeightOffset;
            std::int8_t mHeightData[LAND_NUM_VERTS];
            std::uint16_t mUnk1;
            std::uint8_t mUnk2;
        };
#pragma pack(pop)

        struct LandData
        {
            // Initial reference height for the first vertex, only needed for filling mHeights
            float mHeightOffset = 0;
            // Height in world space for each vertex
            float mHeights[LAND_NUM_VERTS];
            float mMinHeight = 0;
            float mMaxHeight = 0;

            // 24-bit normals, these aren't always correct though. Edge and corner normals may be garbage.
            std::int8_t mNormals[LAND_NUM_VERTS * 3];

            // 2D array of texture indices. An index can be used to look up an LandTexture,
            // but to do so you must subtract 1 from the index first!
            // An index of 0 indicates the default texture.
            std::uint16_t mTextures[LAND_NUM_TEXTURES];

            // 24-bit RGB color for each vertex
            std::uint8_t mColours[3 * LAND_NUM_VERTS];

            // ???
            std::uint16_t mUnk1 = 0;
            std::uint8_t mUnk2 = 0;

            int mDataLoaded = 0;
        };

        // low-LOD heightmap (used for rendering the global map)
        std::int8_t mWnam[LAND_GLOBAL_MAP_LOD_SIZE];

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();

        /**
         * Actually loads data into target
         * If target is nullptr, assumed target is mLandData
         */
        void loadData(int flags, LandData* target = nullptr) const;

        /**
         * Frees memory allocated for mLandData
         */
        void unloadData() const;

        /// Check if given data type is loaded
        bool isDataLoaded(int flags) const;

        Land(const Land& land);

        Land& operator=(const Land& land);

        void swap(Land& land);

        /// Return land data with at least the data types specified in \a flags loaded (if they
        /// are available). Will return a 0-pointer if there is no data for any of the
        /// specified types.
        const LandData* getLandData(int flags) const;

        /// Return land data without loading first anything. Can return a 0-pointer.
        const LandData* getLandData() const
        {
            return mLandData;
        }

        /// Return land data without loading first anything. Can return a 0-pointer.
        LandData* getLandData()
        {
            return mLandData;
        }

        /// \attention Must not be called on objects that aren't fully loaded.
        ///
        /// \note Added data fields will be uninitialised
        void add(int flags);

    private:
        mutable LandData* mLandData = nullptr;
    };

}
#endif
