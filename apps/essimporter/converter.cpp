#include "converter.hpp"

#include <OgreImage.h>

#include <components/esm/creaturestate.hpp>

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
        cell.mName = id;
        cell.load(esm, false);

        // note if the player is in a nameless exterior cell, we will assign the cellId later based on player position
        if (id == mContext->mPlayerCellName)
        {
            mContext->mPlayer.mCellId = cell.getCellId();
        }

        Cell newcell;
        newcell.mCell = cell;

        // fog of war
        // seems to be a 1-bit pixel format, 16*16 pixels
        // TODO: add bleeding of FOW into neighbouring cells (openmw handles this by writing to the textures,
        // MW handles it when rendering only)
        unsigned char nam8[32];
        // exterior has 1 NAM8, interior can have multiple ones, and have an extra 4 byte flag at the start
        // (probably offset of that specific fog texture?)
        while (esm.isNextSub("NAM8"))
        {
            esm.getSubHeader();

            if (esm.getSubSize() == 36)
            {
                // flag on interiors
                esm.skip(4);
            }

            esm.getExact(nam8, 32);

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

            if (cell.isExterior())
            {
                std::ostringstream filename;
                filename << "fog_" << cell.mData.mX << "_" << cell.mData.mY << ".tga";

                convertImage((char*)&newcell.mFogOfWar[0], newcell.mFogOfWar.size()*4, 16, 16, Ogre::PF_BYTE_RGBA, filename.str());
            }
        }

        // moved reference, not handled yet
        // NOTE: MVRF can also occur in within normal references (importcellref.cpp)?
        // this does not match the ESM file implementation,
        // verify if that can happen with ESM files too
        while (esm.isNextSub("MVRF"))
        {
            esm.skipHSub(); // skip MVRF
            esm.getSubName();
            esm.skipHSub(); // skip CNDT
        }

        std::vector<CellRef> cellrefs;
        while (esm.hasMoreSubs() && esm.isNextSub("FRMR"))
        {
            CellRef ref;
            ref.load (esm);
            if (esm.isNextSub("DELE"))
            {
                // strangely this can be e.g. 52 instead of just 1,
                std::cout << "deleted ref " << ref.mIndexedRefId << std::endl;
                esm.skipHSub();
            }
            cellrefs.push_back(ref);
        }

        while (esm.isNextSub("MPCD"))
        {
            float notepos[3];
            esm.getHT(notepos, 3*sizeof(float));

            // Markers seem to be arranged in a 32*32 grid, notepos has grid-indices.
            // This seems to be the reason markers can't be placed everywhere in interior cells,
            // i.e. when the grid is exceeded.
            // Converting the interior markers correctly could be rather tricky, but is probably similar logic
            // as used for the FoW texture placement, which we need to figure out anyway
            notepos[1] += 31.f;
            notepos[0] += 0.5;
            notepos[1] += 0.5;
            notepos[0] = 8192 * notepos[0] / 32.f;
            notepos[1] = 8192 * notepos[1] / 32.f;
            if (cell.isExterior())
            {
                notepos[0] += 8192 * cell.mData.mX;
                notepos[1] += 8192 * cell.mData.mY;
            }
            // TODO: what encoding is this in?
            std::string note = esm.getHNString("MPNT");
            ESM::CustomMarker marker;
            marker.mWorldX = notepos[0];
            marker.mWorldY = notepos[1];
            marker.mNote = note;
            marker.mCell = cell.getCellId();
            mMarkers.push_back(marker);
        }

        newcell.mRefs = cellrefs;

        // FIXME: map by ID for exterior cells
        mCells[id] = newcell;
    }

    void ConvertCell::write(ESM::ESMWriter &esm)
    {
        for (std::map<std::string, Cell>::const_iterator it = mCells.begin(); it != mCells.end(); ++it)
        {
            const ESM::Cell& cell = it->second.mCell;
            esm.startRecord(ESM::REC_CSTA);
            ESM::CellState csta;
            csta.mHasFogOfWar = 0;
            csta.mId = cell.getCellId();
            csta.mId.save(esm);
            // TODO csta.mLastRespawn;
            // shouldn't be needed if we respawn on global schedule like in original MW
            csta.mWaterLevel = cell.mWater;
            csta.save(esm);

            for (std::vector<CellRef>::const_iterator refIt = it->second.mRefs.begin(); refIt != it->second.mRefs.end(); ++refIt)
            {
                const CellRef& cellref = *refIt;
                ESM::CellRef out;
                out.blank();

                if (cellref.mIndexedRefId.size() < 8)
                {
                    std::cerr << "CellRef with no index?" << std::endl;
                    continue;
                }
                std::stringstream stream;
                stream << cellref.mIndexedRefId.substr(cellref.mIndexedRefId.size()-8,8);
                int refIndex;
                stream >> refIndex;

                out.mRefID = cellref.mIndexedRefId.substr(0,cellref.mIndexedRefId.size()-8);

                std::map<std::pair<int, std::string>, CREC>::const_iterator crecIt = mContext->mCreatureChanges.find(
                            std::make_pair(refIndex, out.mRefID));
                if (crecIt != mContext->mCreatureChanges.end())
                {
                    ESM::CreatureState objstate;
                    objstate.blank();
                    convertACDT(cellref.mActorData.mACDT, objstate.mCreatureStats);
                    objstate.mEnabled = cellref.mEnabled;
                    objstate.mPosition = cellref.mPos;
                    objstate.mRef = out;
                    objstate.mRef.mRefNum = cellref.mRefNum;
                    // FIXME: change save format to not require object type, instead look up it up using the RefId
                    esm.writeHNT ("OBJE", ESM::REC_CREA);
                    objstate.save(esm);
                    continue;
                }

                std::map<std::pair<int, std::string>, NPCC>::const_iterator npccIt = mContext->mNpcChanges.find(
                            std::make_pair(refIndex, out.mRefID));
                if (npccIt != mContext->mNpcChanges.end())
                {
                    ESM::NpcState objstate;
                    objstate.blank();
                    convertACDT(cellref.mActorData.mACDT, objstate.mCreatureStats);
                    convertNpcData(cellref.mActorData, objstate.mNpcStats);
                    objstate.mEnabled = cellref.mEnabled;
                    objstate.mPosition = cellref.mPos;
                    objstate.mRef = out;
                    objstate.mRef.mRefNum = cellref.mRefNum;
                    esm.writeHNT ("OBJE", ESM::REC_NPC_);
                    objstate.save(esm);
                    continue;
                }

                std::cerr << "Can't find type for " << refIndex << " " << out.mRefID << std::endl;
            }

            esm.endRecord(ESM::REC_CSTA);
        }

        for (std::vector<ESM::CustomMarker>::const_iterator it = mMarkers.begin(); it != mMarkers.end(); ++it)
        {
            esm.startRecord(ESM::REC_MARK);
            it->save(esm);
            esm.endRecord(ESM::REC_MARK);
        }
    }

}
