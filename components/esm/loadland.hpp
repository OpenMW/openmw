#ifndef _ESM_LAND_H
#define _ESM_LAND_H

#include "record.hpp"
#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{
/*
 * Landscape data.
 */

struct Land : public Record
{
    Land();
    ~Land();

    int flags; // Only first four bits seem to be used, don't know what
    // they mean.
    int X, Y; // Map coordinates.

    // File context. This allows the ESM reader to be 'reset' to this
    // location later when we are ready to load the full data set.
    ESMReader* mEsm;
    ESM_Context context;

    bool hasData;
    int dataTypes;
    bool dataLoaded;

    enum
    {
        DATA_VNML = 1,
        DATA_VHGT = 2,
        DATA_WNAM = 4,
        DATA_VCLR = 8,
        DATA_VTEX = 16
    };

    // number of vertices per side
    static const int LAND_SIZE = 65;

    // cell terrain size in world coords
    static const int REAL_SIZE = 8192;

    // total number of vertices
    static const int LAND_NUM_VERTS = LAND_SIZE * LAND_SIZE;

    static const int HEIGHT_SCALE = 8;

    //number of textures per side of land
    static const int LAND_TEXTURE_SIZE = 16;

    //total number of textures per land
    static const int LAND_NUM_TEXTURES = LAND_TEXTURE_SIZE * LAND_TEXTURE_SIZE;

#pragma pack(push,1)
    struct VHGT
    {
        float heightOffset;
        int8_t heightData[LAND_NUM_VERTS];
        short unknown1;
        char unknown2;
    };
#pragma pack(pop)

    typedef uint8_t VNML[LAND_NUM_VERTS * 3];

    struct LandData
    {
        float heightOffset;
        float heights[LAND_NUM_VERTS];
        VNML normals;
        uint16_t textures[LAND_NUM_TEXTURES];

        bool usingColours;
        char colours[3 * LAND_NUM_VERTS];
        int dataTypes;

        void save(ESMWriter &esm);
    };

    LandData *landData;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);

    int getName() { return REC_LAND; }

    /**
     * Actually loads data
     */
    void loadData();

    /**
     * Frees memory allocated for land data
     */
    void unloadData();

    private:
        Land(const Land& land);
        Land& operator=(const Land& land);
};

}
#endif
