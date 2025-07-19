#include "savingstages.hpp"

#include <QUndoStack>

#include <filesystem>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <apps/opencs/model/doc/savingstate.hpp>
#include <apps/opencs/model/world/cell.hpp>
#include <apps/opencs/model/world/data.hpp>
#include <apps/opencs/model/world/idcollection.hpp>
#include <apps/opencs/model/world/info.hpp>
#include <apps/opencs/model/world/land.hpp>
#include <apps/opencs/model/world/metadata.hpp>
#include <apps/opencs/model/world/pathgrid.hpp>
#include <apps/opencs/model/world/record.hpp>
#include <apps/opencs/model/world/ref.hpp>
#include <apps/opencs/model/world/refcollection.hpp>
#include <apps/opencs/model/world/refidcollection.hpp>
#include <apps/opencs/model/world/refiddata.hpp>
#include <apps/opencs/model/world/universalid.hpp>

#include <components/esm/esmcommon.hpp>
#include <components/esm3/cellref.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm3/loaddial.hpp>
#include <components/esm3/loadinfo.hpp>
#include <components/esm3/loadpgrd.hpp>
#include <components/files/conversion.hpp>
#include <components/misc/strings/lower.hpp>

#include "../world/cellcoordinates.hpp"

#include "document.hpp"

CSMDoc::OpenSaveStage::OpenSaveStage(Document& document, SavingState& state, bool projectFile)
    : mDocument(document)
    , mState(state)
    , mProjectFile(projectFile)
{
}

int CSMDoc::OpenSaveStage::setup()
{
    return 1;
}

void CSMDoc::OpenSaveStage::perform(int stage, Messages& messages)
{
    mState.start(mDocument, mProjectFile);

    mState.getStream().open(mProjectFile ? mState.getPath() : mState.getTmpPath(), std::ios::binary);

    if (!mState.getStream().is_open())
        throw std::runtime_error("failed to open stream for saving");
}

CSMDoc::WriteHeaderStage::WriteHeaderStage(Document& document, SavingState& state, bool simple)
    : mDocument(document)
    , mState(state)
    , mSimple(simple)
{
}

int CSMDoc::WriteHeaderStage::setup()
{
    return 1;
}

void CSMDoc::WriteHeaderStage::perform(int stage, Messages& messages)
{
    mState.getWriter().setVersion();

    mState.getWriter().clearMaster();

    if (mSimple)
    {
        mState.getWriter().setAuthor("");
        mState.getWriter().setDescription("");
        mState.getWriter().setRecordCount(0);

        // ESM::Header::CurrentFormat is `1` but since new records are not yet used in opencs
        // we use the format `0` for compatibility with old versions.
        mState.getWriter().setFormatVersion(ESM::DefaultFormatVersion);
    }
    else
    {
        mDocument.getData().getMetaData().save(mState.getWriter());
        mState.getWriter().setRecordCount(mDocument.getData().count(CSMWorld::RecordBase::State_Modified)
            + mDocument.getData().count(CSMWorld::RecordBase::State_ModifiedOnly)
            + mDocument.getData().count(CSMWorld::RecordBase::State_Deleted));

        /// \todo refine dependency list (at least remove redundant dependencies)
        std::vector<std::filesystem::path> dependencies = mDocument.getContentFiles();
        std::vector<std::filesystem::path>::const_iterator end(--dependencies.end());

        for (std::vector<std::filesystem::path>::const_iterator iter(dependencies.begin()); iter != end; ++iter)
        {
            auto name = Files::pathToUnicodeString(iter->filename());
            auto size = std::filesystem::file_size(*iter);

            mState.getWriter().addMaster(name, size);
        }
    }

    mState.getWriter().save(mState.getStream());
}

CSMDoc::WriteDialogueCollectionStage::WriteDialogueCollectionStage(Document& document, SavingState& state, bool journal)
    : mState(state)
    , mTopics(journal ? document.getData().getJournals() : document.getData().getTopics())
    , mInfos(journal ? document.getData().getJournalInfos() : document.getData().getTopicInfos())
{
}

int CSMDoc::WriteDialogueCollectionStage::setup()
{
    mInfosByTopic = mInfos.getInfosByTopic();
    return mTopics.getSize();
}

void CSMDoc::WriteDialogueCollectionStage::perform(int stage, Messages& messages)
{
    ESM::ESMWriter& writer = mState.getWriter();
    const CSMWorld::Record<ESM::Dialogue>& topic = mTopics.getRecord(stage);

    if (topic.mState == CSMWorld::RecordBase::State_Deleted)
    {
        // if the topic is deleted, we do not need to bother with INFO records.
        const ESM::Dialogue& dialogue = topic.get();
        writer.startRecord(dialogue.sRecordId);
        dialogue.save(writer, true);
        writer.endRecord(dialogue.sRecordId);
        return;
    }

    // Test, if we need to save anything associated info records.
    bool infoModified = false;
    const auto topicInfos = mInfosByTopic.find(topic.get().mId);

    if (topicInfos != mInfosByTopic.end())
    {
        for (const auto& record : topicInfos->second)
        {
            if (record->isModified() || record->mState == CSMWorld::RecordBase::State_Deleted)
            {
                infoModified = true;
                break;
            }
        }
    }

    if (topic.isModified() || infoModified)
    {
        if (infoModified && topic.mState != CSMWorld::RecordBase::State_Modified
            && topic.mState != CSMWorld::RecordBase::State_ModifiedOnly)
        {
            mState.getWriter().startRecord(topic.mBase.sRecordId);
            topic.mBase.save(mState.getWriter(), topic.mState == CSMWorld::RecordBase::State_Deleted);
            mState.getWriter().endRecord(topic.mBase.sRecordId);
        }
        else
        {
            mState.getWriter().startRecord(topic.mModified.sRecordId);
            topic.mModified.save(mState.getWriter(), topic.mState == CSMWorld::RecordBase::State_Deleted);
            mState.getWriter().endRecord(topic.mModified.sRecordId);
        }

        // write modified selected info records
        if (topicInfos != mInfosByTopic.end())
        {
            const std::vector<const CSMWorld::Record<CSMWorld::Info>*>& infos = topicInfos->second;

            for (auto iter = infos.begin(); iter != infos.end(); ++iter)
            {
                const CSMWorld::Record<CSMWorld::Info>& record = **iter;

                if (record.isModified() || record.mState == CSMWorld::RecordBase::State_Deleted)
                {
                    ESM::DialInfo info = record.get();
                    info.mId = record.get().mOriginalId;
                    info.mData.mType = topic.get().mType;

                    if (iter == infos.begin())
                        info.mPrev = ESM::RefId();
                    else
                        info.mPrev = (*std::prev(iter))->get().mOriginalId;

                    const auto next = std::next(iter);

                    if (next == infos.end())
                        info.mNext = ESM::RefId();
                    else
                        info.mNext = (*next)->get().mOriginalId;

                    writer.startRecord(info.sRecordId);
                    info.save(writer, record.mState == CSMWorld::RecordBase::State_Deleted);
                    writer.endRecord(info.sRecordId);
                }
            }
        }
    }
}

CSMDoc::WriteRefIdCollectionStage::WriteRefIdCollectionStage(Document& document, SavingState& state)
    : mDocument(document)
    , mState(state)
{
}

int CSMDoc::WriteRefIdCollectionStage::setup()
{
    return mDocument.getData().getReferenceables().getSize();
}

void CSMDoc::WriteRefIdCollectionStage::perform(int stage, Messages& messages)
{
    mDocument.getData().getReferenceables().save(stage, mState.getWriter());
}

CSMDoc::CollectionReferencesStage::CollectionReferencesStage(Document& document, SavingState& state)
    : mDocument(document)
    , mState(state)
{
}

int CSMDoc::CollectionReferencesStage::setup()
{
    mState.clearSubRecords();

    int size = mDocument.getData().getReferences().getSize();

    int steps = size / 100;
    if (size % 100)
        ++steps;

    return steps;
}

void CSMDoc::CollectionReferencesStage::perform(int stage, Messages& messages)
{
    int size = mDocument.getData().getReferences().getSize();

    for (int i = stage * 100; i < stage * 100 + 100 && i < size; ++i)
    {
        const CSMWorld::Record<CSMWorld::CellRef>& record = mDocument.getData().getReferences().getRecord(i);

        if (record.isModified() || record.mState == CSMWorld::RecordBase::State_Deleted)
        {
            ESM::RefId cellId = record.get().mOriginalCell.empty() ? record.get().mCell : record.get().mOriginalCell;

            std::deque<int>& indices = mState.getOrInsertSubRecord(cellId);

            // collect moved references at the end of the container

            const bool interior = !cellId.startsWith("#");
            std::ostringstream stream;
            if (!interior)
            {
                // recalculate the ref's cell location
                std::pair<int, int> index = record.get().getCellIndex();
                cellId = ESM::RefId::stringRefId(ESM::RefId::esm3ExteriorCell(index.first, index.second).toString());
            }

            // An empty mOriginalCell is meant to indicate that it is the same as
            // the current cell.  It is possible that a moved ref is moved again.
            if ((record.get().mOriginalCell.empty() ? record.get().mCell : record.get().mOriginalCell) != cellId
                && !interior && record.mState != CSMWorld::RecordBase::State_ModifiedOnly && !record.get().mNew)
                indices.push_back(i);
            else
                indices.push_front(i);
        }
    }
}

CSMDoc::WriteCellCollectionStage::WriteCellCollectionStage(Document& document, SavingState& state)
    : mDocument(document)
    , mState(state)
{
}

int CSMDoc::WriteCellCollectionStage::setup()
{
    return mDocument.getData().getCells().getSize();
}

void CSMDoc::WriteCellCollectionStage::writeReferences(
    const std::deque<int>& references, bool interior, unsigned int& newRefNum)
{
    ESM::ESMWriter& writer = mState.getWriter();

    for (std::deque<int>::const_iterator iter(references.begin()); iter != references.end(); ++iter)
    {
        const CSMWorld::Record<CSMWorld::CellRef>& ref = mDocument.getData().getReferences().getRecord(*iter);

        if (ref.isModified() || ref.mState == CSMWorld::RecordBase::State_Deleted)
        {
            CSMWorld::CellRef refRecord = ref.get();

            const bool isLocal = refRecord.mRefNum.mContentFile == 0;

            // -1 is the current file, saved indices are 1-based
            refRecord.mRefNum.mContentFile++;

            // recalculate the ref's cell location
            std::ostringstream stream;
            if (!interior)
            {
                std::pair<int, int> index = refRecord.getCellIndex();
                stream << "#" << index.first << " " << index.second;
            }

            ESM::RefId streamId = ESM::RefId::stringRefId(stream.str());
            if (!isLocal && (refRecord.mOriginalCell.empty() ? refRecord.mCell : refRecord.mOriginalCell) != streamId
                && !interior)
            {
                // An empty mOriginalCell is meant to indicate that it is the same as
                // the current cell.  It is possible that a moved ref is moved again.

                ESM::MovedCellRef moved;
                moved.mRefNum = refRecord.mRefNum;

                // Need to fill mTarget with the ref's new position.
                std::istringstream istream(stream.str().c_str());

                char ignore;
                istream >> ignore >> moved.mTarget[0] >> moved.mTarget[1];

                writer.writeFormId(refRecord.mRefNum, false, "MVRF");
                writer.writeHNT("CNDT", moved.mTarget);
            }

            refRecord.save(writer, false, false, ref.mState == CSMWorld::RecordBase::State_Deleted);
        }
    }
}

void CSMDoc::WriteCellCollectionStage::perform(int stage, Messages& messages)
{
    ESM::ESMWriter& writer = mState.getWriter();
    const CSMWorld::Record<CSMWorld::Cell>& cell = mDocument.getData().getCells().getRecord(stage);
    const CSMWorld::RefIdCollection& referenceables = mDocument.getData().getReferenceables();
    const CSMWorld::RefIdData& refIdData = referenceables.getDataSet();

    std::deque<int> tempRefs;
    std::deque<int> persistentRefs;

    const std::deque<int>* references = mState.findSubRecord(cell.get().mId);

    if (cell.isModified() || cell.mState == CSMWorld::RecordBase::State_Deleted || references != nullptr)
    {
        CSMWorld::Cell cellRecord = cell.get();
        const bool interior = !cellRecord.mId.startsWith("#");

        // count new references and adjust RefNumCount accordingsly
        unsigned int newRefNum = cellRecord.mRefNumCounter;

        if (references != nullptr)
        {
            for (std::deque<int>::const_iterator iter(references->begin()); iter != references->end(); ++iter)
            {
                const CSMWorld::Record<CSMWorld::CellRef>& ref = mDocument.getData().getReferences().getRecord(*iter);

                CSMWorld::CellRef refRecord = ref.get();

                CSMWorld::RefIdData::LocalIndex localIndex = refIdData.searchId(refRecord.mRefID);
                unsigned int recordFlags = refIdData.getRecordFlags(refRecord.mRefID);
                bool isPersistent = ((recordFlags & ESM::FLAG_Persistent) != 0) || refRecord.mTeleport
                    || localIndex.second == CSMWorld::UniversalId::Type_Creature
                    || localIndex.second == CSMWorld::UniversalId::Type_Npc;

                if (isPersistent)
                    persistentRefs.push_back(*iter);
                else
                    tempRefs.push_back(*iter);

                if (refRecord.mNew
                    || (!interior && ref.mState == CSMWorld::RecordBase::State_ModifiedOnly &&
                        /// \todo consider worldspace
                        ESM::RefId::stringRefId(CSMWorld::CellCoordinates(refRecord.getCellIndex()).getId(""))
                            != refRecord.mCell))
                    ++cellRecord.mRefNumCounter;

                if (refRecord.mRefNum.mIndex >= newRefNum)
                    newRefNum = refRecord.mRefNum.mIndex + 1;
            }
        }

        // write cell data
        writer.startRecord(cellRecord.sRecordId);

        if (interior)
            cellRecord.mData.mFlags |= ESM::Cell::Interior;
        else
        {
            cellRecord.mData.mFlags &= ~ESM::Cell::Interior;

            std::istringstream stream(cellRecord.mId.getRefIdString().c_str());
            char ignore;
            stream >> ignore >> cellRecord.mData.mX >> cellRecord.mData.mY;
        }

        cellRecord.save(writer, cell.mState == CSMWorld::RecordBase::State_Deleted);

        // write references
        if (references != nullptr)
        {
            writeReferences(persistentRefs, interior, newRefNum);
            cellRecord.saveTempMarker(writer, static_cast<int>(references->size()) - persistentRefs.size());
            writeReferences(tempRefs, interior, newRefNum);
        }

        writer.endRecord(cellRecord.sRecordId);
    }
}

CSMDoc::WritePathgridCollectionStage::WritePathgridCollectionStage(Document& document, SavingState& state)
    : mDocument(document)
    , mState(state)
{
}

int CSMDoc::WritePathgridCollectionStage::setup()
{
    return mDocument.getData().getPathgrids().getSize();
}

void CSMDoc::WritePathgridCollectionStage::perform(int stage, Messages& messages)
{
    ESM::ESMWriter& writer = mState.getWriter();
    const CSMWorld::Record<CSMWorld::Pathgrid>& pathgrid = mDocument.getData().getPathgrids().getRecord(stage);

    if (pathgrid.isModified() || pathgrid.mState == CSMWorld::RecordBase::State_Deleted)
    {
        CSMWorld::Pathgrid record = pathgrid.get();
        if (record.mId.startsWith("#"))
        {
            std::istringstream stream(record.mId.getRefIdString());
            char ignore;
            stream >> ignore >> record.mData.mX >> record.mData.mY;
        }
        else
            record.mCell = record.mId;

        writer.startRecord(record.sRecordId);
        record.save(writer, pathgrid.mState == CSMWorld::RecordBase::State_Deleted);
        writer.endRecord(record.sRecordId);
    }
}

CSMDoc::WriteLandCollectionStage::WriteLandCollectionStage(Document& document, SavingState& state)
    : mDocument(document)
    , mState(state)
{
}

int CSMDoc::WriteLandCollectionStage::setup()
{
    return mDocument.getData().getLand().getSize();
}

void CSMDoc::WriteLandCollectionStage::perform(int stage, Messages& messages)
{
    ESM::ESMWriter& writer = mState.getWriter();
    const CSMWorld::Record<CSMWorld::Land>& land = mDocument.getData().getLand().getRecord(stage);

    if (land.isModified() || land.mState == CSMWorld::RecordBase::State_Deleted)
    {
        CSMWorld::Land record = land.get();
        writer.startRecord(record.sRecordId);
        record.save(writer, land.mState == CSMWorld::RecordBase::State_Deleted);
        writer.endRecord(record.sRecordId);
    }
}

CSMDoc::WriteLandTextureCollectionStage::WriteLandTextureCollectionStage(Document& document, SavingState& state)
    : mDocument(document)
    , mState(state)
{
}

int CSMDoc::WriteLandTextureCollectionStage::setup()
{
    return mDocument.getData().getLandTextures().getSize();
}

void CSMDoc::WriteLandTextureCollectionStage::perform(int stage, Messages& messages)
{
    ESM::ESMWriter& writer = mState.getWriter();
    const CSMWorld::Record<ESM::LandTexture>& landTexture = mDocument.getData().getLandTextures().getRecord(stage);

    if (landTexture.isModified() || landTexture.mState == CSMWorld::RecordBase::State_Deleted)
    {
        ESM::LandTexture record = landTexture.get();
        writer.startRecord(record.sRecordId);
        record.save(writer, landTexture.mState == CSMWorld::RecordBase::State_Deleted);
        writer.endRecord(record.sRecordId);
    }
}

CSMDoc::CloseSaveStage::CloseSaveStage(SavingState& state)
    : mState(state)
{
}

int CSMDoc::CloseSaveStage::setup()
{
    return 1;
}

void CSMDoc::CloseSaveStage::perform(int stage, Messages& messages)
{
    mState.getStream().close();

    if (!mState.getStream())
        throw std::runtime_error("saving failed");
}

CSMDoc::FinalSavingStage::FinalSavingStage(Document& document, SavingState& state)
    : mDocument(document)
    , mState(state)
{
}

int CSMDoc::FinalSavingStage::setup()
{
    return 1;
}

void CSMDoc::FinalSavingStage::perform(int stage, Messages& messages)
{
    if (mState.hasError())
    {
        mState.getWriter().close();
        mState.getStream().close();

        if (std::filesystem::exists(mState.getTmpPath()))
            std::filesystem::remove(mState.getTmpPath());
    }
    else if (!mState.isProjectFile())
    {
        if (std::filesystem::exists(mState.getPath()))
            std::filesystem::remove(mState.getPath());

        std::filesystem::rename(mState.getTmpPath(), mState.getPath());

        mDocument.getUndoStack().setClean();
    }
}
