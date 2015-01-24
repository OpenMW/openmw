#include "converter.hpp"

#include <stdexcept>

#include <OgreImage.h>

#include <components/esm/creaturestate.hpp>
#include <components/esm/containerstate.hpp>

#include "convertcrec.hpp"
#include "convertcntc.hpp"
#include "convertscri.hpp"

namespace
{

    void convertImage(char* data, int size, int width, int height, Ogre::PixelFormat pf, const std::string& out)
    {
        Ogre::Image screenshot;
        Ogre::DataStreamPtr stream (new Ogre::MemoryDataStream(data, size));
        screenshot.loadRawData(stream, width, height, 1, pf);
        screenshot.save(out);
    }


    void convertCellRef(const ESSImport::CellRef& cellref, ESM::ObjectState& objstate)
    {
        objstate.mEnabled = cellref.mEnabled;
        objstate.mPosition = cellref.mPos;
        objstate.mRef.mRefNum = cellref.mRefNum;
        if (cellref.mDeleted)
            objstate.mCount = 0;
        convertSCRI(cellref.mSCRI, objstate.mLocals);
        objstate.mHasLocals = !objstate.mLocals.mVariables.empty();
    }

    bool isIndexedRefId(const std::string& indexedRefId)
    {
        if (indexedRefId.size() <= 8)
            return false;

        if (indexedRefId.find_first_not_of("0123456789") == std::string::npos)
            return false; // entirely numeric refid, this is a reference to
                          // a dynamically created record e.g. player-enchanted weapon

        std::string index = indexedRefId.substr(indexedRefId.size()-8);
        if(index.find_first_not_of("0123456789ABCDEF") == std::string::npos )
            return true;
        return false;
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

        // I wonder what 0x40 does?
        if (cell.isExterior() && cell.mData.mFlags & 0x20)
        {
            mContext->mGlobalMapState.mMarkers.insert(std::make_pair(cell.mData.mX, cell.mData.mY));
        }

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


        if (cell.isExterior())
            mExtCells[std::make_pair(cell.mData.mX, cell.mData.mY)] = newcell;
        else
            mIntCells[id] = newcell;
    }

    void ConvertCell::writeCell(const Cell &cell, ESM::ESMWriter& esm)
    {
        ESM::Cell esmcell = cell.mCell;
        esm.startRecord(ESM::REC_CSTA);
        ESM::CellState csta;
        csta.mHasFogOfWar = 0;
        csta.mId = esmcell.getCellId();
        csta.mId.save(esm);
        // TODO csta.mLastRespawn;
        // shouldn't be needed if we respawn on global schedule like in original MW
        csta.mWaterLevel = esmcell.mWater;
        csta.save(esm);

        for (std::vector<CellRef>::const_iterator refIt = cell.mRefs.begin(); refIt != cell.mRefs.end(); ++refIt)
        {
            const CellRef& cellref = *refIt;
            ESM::CellRef out (cellref);

            if (!isIndexedRefId(cellref.mIndexedRefId))
            {
                // non-indexed RefNum, i.e. no CREC/NPCC/CNTC record associated with it
                // this could be any type of object really (even creatures/npcs too)
                out.mRefID = cellref.mIndexedRefId;
                std::string idLower = Misc::StringUtils::lowerCase(out.mRefID);

                ESM::ObjectState objstate;
                objstate.blank();
                objstate.mRef = out;
                objstate.mRef.mRefID = idLower;
                objstate.mHasCustomState = false;
                convertCellRef(cellref, objstate);
                esm.writeHNT ("OBJE", 0);
                objstate.save(esm);
                continue;
            }
            else
            {
                std::stringstream stream;
                stream << std::hex << cellref.mIndexedRefId.substr(cellref.mIndexedRefId.size()-8,8);
                int refIndex;
                stream >> refIndex;

                out.mRefID = cellref.mIndexedRefId.substr(0,cellref.mIndexedRefId.size()-8);
                std::string idLower = Misc::StringUtils::lowerCase(out.mRefID);

                std::map<std::pair<int, std::string>, NPCC>::const_iterator npccIt = mContext->mNpcChanges.find(
                            std::make_pair(refIndex, out.mRefID));
                if (npccIt != mContext->mNpcChanges.end())
                {
                    ESM::NpcState objstate;
                    objstate.blank();
                    objstate.mRef = out;
                    objstate.mRef.mRefID = idLower;
                    // probably need more micromanagement here so we don't overwrite values
                    // from the ESM with default values
                    convertACDT(cellref.mACDT, objstate.mCreatureStats);
                    convertNpcData(cellref, objstate.mNpcStats);
                    convertNPCC(npccIt->second, objstate);
                    convertCellRef(cellref, objstate);
                    esm.writeHNT ("OBJE", ESM::REC_NPC_);
                    objstate.save(esm);
                    continue;
                }

                std::map<std::pair<int, std::string>, CNTC>::const_iterator cntcIt = mContext->mContainerChanges.find(
                            std::make_pair(refIndex, out.mRefID));
                if (cntcIt != mContext->mContainerChanges.end())
                {
                    ESM::ContainerState objstate;
                    objstate.blank();
                    objstate.mRef = out;
                    objstate.mRef.mRefID = idLower;
                    convertCNTC(cntcIt->second, objstate);
                    convertCellRef(cellref, objstate);
                    esm.writeHNT ("OBJE", ESM::REC_CONT);
                    objstate.save(esm);
                    continue;
                }

                std::map<std::pair<int, std::string>, CREC>::const_iterator crecIt = mContext->mCreatureChanges.find(
                            std::make_pair(refIndex, out.mRefID));
                if (crecIt != mContext->mCreatureChanges.end())
                {
                    ESM::CreatureState objstate;
                    objstate.blank();
                    objstate.mRef = out;
                    objstate.mRef.mRefID = idLower;
                    convertACDT(cellref.mACDT, objstate.mCreatureStats);
                    // probably need more micromanagement here so we don't overwrite values
                    // from the ESM with default values
                    convertCREC(crecIt->second, objstate);
                    convertCellRef(cellref, objstate);
                    esm.writeHNT ("OBJE", ESM::REC_CREA);
                    objstate.save(esm);
                    continue;
                }

                std::stringstream error;
                error << "Can't find type for " << cellref.mIndexedRefId << std::endl;
                throw std::runtime_error(error.str());
            }
        }

        esm.endRecord(ESM::REC_CSTA);
    }

    void ConvertCell::write(ESM::ESMWriter &esm)
    {
        for (std::map<std::string, Cell>::const_iterator it = mIntCells.begin(); it != mIntCells.end(); ++it)
            writeCell(it->second, esm);

        for (std::map<std::pair<int, int>, Cell>::const_iterator it = mExtCells.begin(); it != mExtCells.end(); ++it)
            writeCell(it->second, esm);

        for (std::vector<ESM::CustomMarker>::const_iterator it = mMarkers.begin(); it != mMarkers.end(); ++it)
        {
            esm.startRecord(ESM::REC_MARK);
            it->save(esm);
            esm.endRecord(ESM::REC_MARK);
        }

        esm.startRecord(ESM::REC_GMAP);
        mContext->mGlobalMapState.save(esm);
        esm.endRecord(ESM::REC_GMAP);
    }

}
