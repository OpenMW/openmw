#ifndef MWRENDER_TERRAINSTORAGE_H
#define MWRENDER_TERRAINSTORAGE_H

#include <components/terrain/storage.hpp>

namespace MWRender
{

    class TerrainStorage : public Terrain::Storage
    {
    private:
        virtual ESM::Land* getLand (int cellX, int cellY);
        virtual const ESM::LandTexture* getLandTexture(int index, short plugin);
    public:
        virtual Ogre::AxisAlignedBox getBounds();
        ///< Get bounds in cell units
    };

}


#endif
