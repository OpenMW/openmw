#include "converter.hpp"

#include <OgreImage.h>

namespace
{

    void convertImage(char* data, int size, int width, int height, Ogre::PixelFormat pf, const std::string& out)
    {
        Ogre::Image screenshot;
        Ogre::DataStreamPtr stream (new Ogre::MemoryDataStream(data, size));
        screenshot.loadRawData(stream, width, height, 1, pf);
        screenshot.save(out);
    }

}

namespace ESSImport
{


    struct MAPH
    {
        unsigned int size;
        unsigned int value;
    };

    void ConvertFMAP::read(ESM::ESMReader &esm)
    {
        MAPH maph;
        esm.getHNT(maph, "MAPH");
        std::vector<char> data;
        esm.getSubNameIs("MAPD");
        esm.getSubHeader();
        data.resize(esm.getSubSize());
        esm.getExact(&data[0], data.size());
        convertImage(&data[0], data.size(), maph.size, maph.size, Ogre::PF_BYTE_RGB, "map.tga");
    }

    void ConvertCell::read(ESM::ESMReader &esm)
    {
        ESM::Cell cell;
        std::string id = esm.getHNString("NAME");
        cell.load(esm, false);

        Cell newcell;
        newcell.mCell = cell;

        // fog of war
        // seems to be a 1-bit pixel format, 16*16 pixels
        // TODO: add bleeding of FOW into neighbouring cells (openmw handles this by writing to the textures,
        // MW handles it when rendering only)
        unsigned char nam8[32];
        if (esm.isNextSub("NAM8"))
        {
            esm.getHExact(nam8, 32);

            newcell.mFogOfWar.reserve(16*16);
            for (int x=0; x<16; ++x)
            {
                for (int y=0; y<16; ++y)
                {
                    size_t pos = x*16+y;
                    size_t bytepos = pos/8;
                    assert(bytepos<32);
                    int bit = pos%8;
                    newcell.mFogOfWar.push_back(((nam8[bytepos] >> bit) & (0x1)) ? 0xffffffff : 0x000000ff);
                }
            }

            std::ostringstream filename;
            filename << "fog_" << cell.mData.mX << "_" << cell.mData.mY << ".tga";

            convertImage((char*)&newcell.mFogOfWar[0], newcell.mFogOfWar.size()*4, 16, 16, Ogre::PF_BYTE_RGBA, filename.str());
        }

        std::vector<CellRef> cellrefs;
        while (esm.hasMoreSubs())
        {
            CellRef ref;
            ref.load (esm);
            if (esm.isNextSub("DELE"))
                std::cout << "deleted ref " << ref.mIndexedRefId << std::endl;
        }

        newcell.mRefs = cellrefs;
        mCells[id] = newcell;
    }

}
