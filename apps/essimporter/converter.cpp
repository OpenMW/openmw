#include "converter.hpp"

#include <stdexcept>
#include <algorithm>

#include <osgDB/WriteFile>

#include <components/esm3/creaturestate.hpp>
#include <components/esm3/containerstate.hpp>

#include <components/misc/constants.hpp>

#include "convertcrec.hpp"
#include "convertcntc.hpp"
#include "convertscri.hpp"

namespace
{

    void convertImage(char* data, int size, int width, int height, GLenum pf, const std::string& out)
    {
        osg::ref_ptr<osg::Image> image (new osg::Image);
        image->allocateImage(width, height, 1, pf, GL_UNSIGNED_BYTE);
        memcpy(image->data(), data, size);
        image->flipVertical();

        osgDB::writeImageFile(*image, out);
    }


    void convertCellRef(const ESSImport::CellRef& cellref, ESM::ObjectState& objstate)
    {
        objstate.mEnabled = cellref.mEnabled;
        objstate.mPosition = cellref.mPos;
        objstate.mRef.mRefNum = cellref.mRefNum;
        if (cellref.mDeleted)
            objstate.mCount = 0;
        convertSCRI(cellref.mActorData.mSCRI, objstate.mLocals);
        objstate.mHasLocals = !objstate.mLocals.mVariables.empty();

        if (cellref.mActorData.mHasANIS)
            convertANIS(cellref.mActorData.mANIS, objstate.mAnimationState);
    }

    bool isIndexedRefId(const std::string& indexedRefId)
    {
        if (indexedRefId.size() <= 8)
            return false;

        if (indexedRefId.find_first_not_of("0123456789") == std::string::npos)
            return false; // entirely numeric refid, this is a reference to
                          // a dynamically created record e.g. player-enchanted weapon

        std::string index = indexedRefId.substr(indexedRefId.size()-8);
        return index.find_first_not_of("0123456789ABCDEF") == std::string::npos;
    }

    void splitIndexedRefId(const std::string& indexedRefId, int& refIndex, std::string& refId)
    {
        std::stringstream stream;
        stream << std::hex << indexedRefId.substr(indexedRefId.size()-8,8);
        stream >> refIndex;

        refId = indexedRefId.substr(0,indexedRefId.size()-8);
    }

    int convertActorId(const std::string& indexedRefId, ESSImport::Context& context)
    {
        if (isIndexedRefId(indexedRefId))
        {
            int refIndex = 0;
            std::string refId;
            splitIndexedRefId(indexedRefId, refIndex, refId);

            auto it = context.mActorIdMap.find(std::make_pair(refIndex, refId));
            if (it == context.mActorIdMap.end())
                return -1;
            return it->second;
        }
        else if (indexedRefId == "PlayerSaveGame")
        {
            return context.mPlayer.mObject.mCreatureStats.mActorId;
        }

        return -1;
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
        esm.getExact(data.data(), data.size());

        mGlobalMapImage = new osg::Image;
        mGlobalMapImage->allocateImage(maph.size, maph.size, 1, GL_RGB, GL_UNSIGNED_BYTE);
        memcpy(mGlobalMapImage->data(), data.data(), data.size());

        // to match openmw size
        // FIXME: filtering?
        mGlobalMapImage->scaleImage(maph.size*2, maph.size*2, 1, GL_UNSIGNED_BYTE);
    }

    void ConvertFMAP::write(ESM::ESMWriter &esm)
    {
        int numcells = mGlobalMapImage->s() / 18; // NB truncating, doesn't divide perfectly
                                                       // with the 512x512 map the game has by default
        int cellSize = mGlobalMapImage->s()/numcells;

        // Note the upper left corner of the (0,0) cell should be at (width/2, height/2)

        mContext->mGlobalMapState.mBounds.mMinX = -numcells/2;
        mContext->mGlobalMapState.mBounds.mMaxX = (numcells-1)/2;
        mContext->mGlobalMapState.mBounds.mMinY = -(numcells-1)/2;
        mContext->mGlobalMapState.mBounds.mMaxY = numcells/2;

        osg::ref_ptr<osg::Image> image2 (new osg::Image);
        int width = cellSize*numcells;
        int height = cellSize*numcells;
        std::vector<unsigned char> data;
        data.resize(width*height*4, 0);

        image2->allocateImage(width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE);
        memcpy(image2->data(), data.data(), data.size());

        for (const auto & exploredCell : mContext->mExploredCells)
        {
            if (exploredCell.first > mContext->mGlobalMapState.mBounds.mMaxX
                    || exploredCell.first < mContext->mGlobalMapState.mBounds.mMinX
                    || exploredCell.second > mContext->mGlobalMapState.mBounds.mMaxY
                    || exploredCell.second < mContext->mGlobalMapState.mBounds.mMinY)
            {
                // out of bounds, I think this could happen, since the original engine had a fixed-size map
                continue;
            }

            int imageLeftSrc = mGlobalMapImage->s()/2;
            int imageTopSrc = mGlobalMapImage->t()/2;
            imageLeftSrc += exploredCell.first * cellSize;
            imageTopSrc -= exploredCell.second * cellSize;
            int imageLeftDst = width/2;
            int imageTopDst = height/2;
            imageLeftDst += exploredCell.first * cellSize;
            imageTopDst -= exploredCell.second * cellSize;
            for (int x=0; x<cellSize; ++x)
                for (int y=0; y<cellSize; ++y)
                {
                    unsigned int col = *(unsigned int*)mGlobalMapImage->data(imageLeftSrc+x, imageTopSrc+y, 0);
                    *(unsigned int*)image2->data(imageLeftDst+x, imageTopDst+y, 0) = col;
                }
        }

        std::stringstream ostream;
        osgDB::ReaderWriter* readerwriter = osgDB::Registry::instance()->getReaderWriterForExtension("png");
        if (!readerwriter)
        {
            std::cerr << "Error: can't write global map image, no png readerwriter found" << std::endl;
            return;
        }

        image2->flipVertical();

        osgDB::ReaderWriter::WriteResult result = readerwriter->writeImage(*image2, ostream);
        if (!result.success())
        {
            std::cerr << "Error: can't write global map image: " << result.message() << " code " << result.status() << std::endl;
            return;
        }

        std::string outData = ostream.str();
        mContext->mGlobalMapState.mImageData = std::vector<char>(outData.begin(), outData.end());

        esm.startRecord(ESM::REC_GMAP);
        mContext->mGlobalMapState.save(esm);
        esm.endRecord(ESM::REC_GMAP);
    }

    void ConvertCell::read(ESM::ESMReader &esm)
    {
        ESM::Cell cell;
        bool isDeleted = false;

        cell.load(esm, isDeleted, false);

        // I wonder what 0x40 does?
        if (cell.isExterior() && cell.mData.mFlags & 0x20)
        {
            mContext->mGlobalMapState.mMarkers.insert(std::make_pair(cell.mData.mX, cell.mData.mY));
        }

        // note if the player is in a nameless exterior cell, we will assign the cellId later based on player position
        if (cell.mName == mContext->mPlayerCellName)
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
            if (cell.isExterior()) // TODO: NAM8 occasionally exists for cells that haven't been explored.
                                   // are there any flags marking explored cells?
                mContext->mExploredCells.insert(std::make_pair(cell.mData.mX, cell.mData.mY));

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

                convertImage((char*)&newcell.mFogOfWar[0], newcell.mFogOfWar.size()*4, 16, 16, GL_RGBA, filename.str());
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
        while (esm.hasMoreSubs() && esm.peekNextSub("FRMR"))
        {
            CellRef ref;
            ref.load (esm);
            cellrefs.push_back(ref);
        }

        while (esm.isNextSub("MPCD"))
        {
            float notepos[3];
            esm.getHTSized<3 * sizeof(float)>(notepos);

            // Markers seem to be arranged in a 32*32 grid, notepos has grid-indices.
            // This seems to be the reason markers can't be placed everywhere in interior cells,
            // i.e. when the grid is exceeded.
            // Converting the interior markers correctly could be rather tricky, but is probably similar logic
            // as used for the FoW texture placement, which we need to figure out anyway
            notepos[1] += 31.f;
            notepos[0] += 0.5;
            notepos[1] += 0.5;
            notepos[0] = Constants::CellSizeInUnits * notepos[0] / 32.f;
            notepos[1] = Constants::CellSizeInUnits * notepos[1] / 32.f;
            if (cell.isExterior())
            {
                notepos[0] += Constants::CellSizeInUnits * cell.mData.mX;
                notepos[1] += Constants::CellSizeInUnits * cell.mData.mY;
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
            mIntCells[cell.mName] = newcell;
    }

    void ConvertCell::writeCell(const Cell &cell, ESM::ESMWriter& esm)
    {
        ESM::Cell esmcell = cell.mCell;
        esm.startRecord(ESM::REC_CSTA);
        ESM::CellState csta;
        csta.mHasFogOfWar = 0;
        csta.mLastRespawn.mDay = 0;
        csta.mLastRespawn.mHour = 0;
        csta.mId = esmcell.getCellId();
        csta.mId.save(esm);
        // TODO csta.mLastRespawn;
        // shouldn't be needed if we respawn on global schedule like in original MW
        csta.mWaterLevel = esmcell.mWater;
        csta.save(esm);

        for (const auto & cellref : cell.mRefs)
        {
            ESM::CellRef out (cellref);

            // TODO: use mContext->mCreatures/mNpcs

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
                int refIndex = 0;
                splitIndexedRefId(cellref.mIndexedRefId, refIndex, out.mRefID);

                std::string idLower = Misc::StringUtils::lowerCase(out.mRefID);

                auto npccIt = mContext->mNpcChanges.find(
                            std::make_pair(refIndex, out.mRefID));
                if (npccIt != mContext->mNpcChanges.end())
                {
                    ESM::NpcState objstate;
                    objstate.blank();
                    objstate.mRef = out;
                    objstate.mRef.mRefID = idLower;
                    // TODO: need more micromanagement here so we don't overwrite values
                    // from the ESM with default values
                    if (cellref.mActorData.mHasACDT)
                        convertACDT(cellref.mActorData.mACDT, objstate.mCreatureStats);
                    else
                        objstate.mCreatureStats.mMissingACDT = true;
                    if (cellref.mActorData.mHasACSC)
                        convertACSC(cellref.mActorData.mACSC, objstate.mCreatureStats);
                    convertNpcData(cellref.mActorData, objstate.mNpcStats);
                    convertNPCC(npccIt->second, objstate);
                    convertCellRef(cellref, objstate);

                    objstate.mCreatureStats.mActorId = mContext->generateActorId();
                    mContext->mActorIdMap.insert(std::make_pair(std::make_pair(refIndex, out.mRefID), objstate.mCreatureStats.mActorId));

                    esm.writeHNT ("OBJE", ESM::REC_NPC_);
                    objstate.save(esm);
                    continue;
                }

                auto cntcIt = mContext->mContainerChanges.find(
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

                auto crecIt = mContext->mCreatureChanges.find(
                            std::make_pair(refIndex, out.mRefID));
                if (crecIt != mContext->mCreatureChanges.end())
                {
                    ESM::CreatureState objstate;
                    objstate.blank();
                    objstate.mRef = out;
                    objstate.mRef.mRefID = idLower;
                    // TODO: need more micromanagement here so we don't overwrite values
                    // from the ESM with default values
                    if (cellref.mActorData.mHasACDT)
                        convertACDT(cellref.mActorData.mACDT, objstate.mCreatureStats);
                    else
                        objstate.mCreatureStats.mMissingACDT = true;
                    if (cellref.mActorData.mHasACSC)
                        convertACSC(cellref.mActorData.mACSC, objstate.mCreatureStats);
                    convertCREC(crecIt->second, objstate);
                    convertCellRef(cellref, objstate);

                    objstate.mCreatureStats.mActorId = mContext->generateActorId();
                    mContext->mActorIdMap.insert(std::make_pair(std::make_pair(refIndex, out.mRefID), objstate.mCreatureStats.mActorId));

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
        for (const auto & cell : mIntCells)
            writeCell(cell.second, esm);

        for (const auto & cell : mExtCells)
            writeCell(cell.second, esm);

        for (const auto & marker : mMarkers)
        {
            esm.startRecord(ESM::REC_MARK);
            marker.save(esm);
            esm.endRecord(ESM::REC_MARK);
        }
    }

    void ConvertPROJ::read(ESM::ESMReader& esm)
    {
        mProj.load(esm);
    }

    void ConvertPROJ::write(ESM::ESMWriter& esm)
    {
        for (const PROJ::PNAM& pnam : mProj.mProjectiles)
        {
            if (!pnam.isMagic())
            {
                ESM::ProjectileState out;
                convertBaseState(out, pnam);

                out.mBowId = pnam.mBowId.toString();
                out.mVelocity = pnam.mVelocity;
                out.mAttackStrength = pnam.mAttackStrength;

                esm.startRecord(ESM::REC_PROJ);
                out.save(esm);
                esm.endRecord(ESM::REC_PROJ);
            }
            else
            {
                ESM::MagicBoltState out;
                convertBaseState(out, pnam);

                auto it = std::find_if(mContext->mActiveSpells.begin(), mContext->mActiveSpells.end(),
                                       [&pnam](const SPLM::ActiveSpell& spell) -> bool { return spell.mIndex == pnam.mSplmIndex; });

                if (it == mContext->mActiveSpells.end())
                {
                    std::cerr << "Warning: Skipped conversion for magic projectile \"" << pnam.mArrowId.toString() << "\" (invalid spell link)" << std::endl;
                    continue;
                }

                out.mSpellId = it->mSPDT.mId.toString();
                out.mSpeed = pnam.mSpeed * 0.001f; // not sure where this factor comes from
                out.mSlot = 0;

                esm.startRecord(ESM::REC_MPRJ);
                out.save(esm);
                esm.endRecord(ESM::REC_MPRJ);
            }
        }
    }

    void ConvertPROJ::convertBaseState(ESM::BaseProjectileState& base, const PROJ::PNAM& pnam)
    {
        base.mId = pnam.mArrowId.toString();
        base.mPosition = pnam.mPosition;

        osg::Quat orient;
        orient.makeRotate(osg::Vec3f(0,1,0), pnam.mVelocity);
        base.mOrientation = orient;

        base.mActorId = convertActorId(pnam.mActorId.toString(), *mContext);
    }

    void ConvertSPLM::read(ESM::ESMReader& esm)
    {
        mSPLM.load(esm);
        mContext->mActiveSpells = mSPLM.mActiveSpells;
    }

    void ConvertSPLM::write(ESM::ESMWriter& esm)
    {
        std::cerr << "Warning: Skipped active spell conversion (not implemented)" << std::endl;
    }

}
