#ifndef CSM_DOC_SAVINGSTAGES_H
#define CSM_DOC_SAVINGSTAGES_H

#include "stage.hpp"

#include "../world/record.hpp"
#include "../world/idcollection.hpp"
#include "../world/scope.hpp"

#include <components/esm/defs.hpp>

#include "savingstate.hpp"

namespace ESM
{
    struct Dialogue;
}

namespace CSMWorld
{
    class InfoCollection;
}

namespace CSMDoc
{
    class Document;
    class SavingState;

    class OpenSaveStage : public Stage
    {
            Document& mDocument;
            SavingState& mState;
            bool mProjectFile;

        public:

            OpenSaveStage (Document& document, SavingState& state, bool projectFile);
            ///< \param projectFile Saving the project file instead of the content file.

            int setup() override;
            ///< \return number of steps

            void perform (int stage, Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    class WriteHeaderStage : public Stage
    {
            Document& mDocument;
            SavingState& mState;
            bool mSimple;

        public:

            WriteHeaderStage (Document& document, SavingState& state, bool simple);
            ///< \param simple Simplified header (used for project files).

            int setup() override;
            ///< \return number of steps

            void perform (int stage, Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };


    template<class CollectionT>
    class WriteCollectionStage : public Stage
    {
            const CollectionT& mCollection;
            SavingState& mState;
            CSMWorld::Scope mScope;

        public:

            WriteCollectionStage (const CollectionT& collection, SavingState& state,
                CSMWorld::Scope scope = CSMWorld::Scope_Content);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    template<class CollectionT>
    WriteCollectionStage<CollectionT>::WriteCollectionStage (const CollectionT& collection,
        SavingState& state, CSMWorld::Scope scope)
    : mCollection (collection), mState (state), mScope (scope)
    {}

    template<class CollectionT>
    int WriteCollectionStage<CollectionT>::setup()
    {
        return mCollection.getSize();
    }

    template<class CollectionT>
    void WriteCollectionStage<CollectionT>::perform (int stage, Messages& messages)
    {
        if (CSMWorld::getScopeFromId (mCollection.getRecord (stage).get().mId)!=mScope)
            return;

        ESM::ESMWriter& writer = mState.getWriter();
        CSMWorld::RecordBase::State state = mCollection.getRecord (stage).mState;
        typename CollectionT::ESXRecord record = mCollection.getRecord (stage).get();

        if (state == CSMWorld::RecordBase::State_Modified ||
            state == CSMWorld::RecordBase::State_ModifiedOnly ||
            state == CSMWorld::RecordBase::State_Deleted)
        {
            writer.startRecord (record.sRecordId);
            record.save (writer, state == CSMWorld::RecordBase::State_Deleted);
            writer.endRecord (record.sRecordId);
        }
    }


    class WriteDialogueCollectionStage : public Stage
    {
            SavingState& mState;
            const CSMWorld::IdCollection<ESM::Dialogue>& mTopics;
            CSMWorld::InfoCollection& mInfos;

        public:

            WriteDialogueCollectionStage (Document& document, SavingState& state, bool journal);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };


    class WriteRefIdCollectionStage : public Stage
    {
            Document& mDocument;
            SavingState& mState;

        public:

            WriteRefIdCollectionStage (Document& document, SavingState& state);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };


    class CollectionReferencesStage : public Stage
    {
            Document& mDocument;
            SavingState& mState;

        public:

            CollectionReferencesStage (Document& document, SavingState& state);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    class WriteCellCollectionStage : public Stage
    {
            Document& mDocument;
            SavingState& mState;

        public:

            WriteCellCollectionStage (Document& document, SavingState& state);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };


    class WritePathgridCollectionStage : public Stage
    {
            Document& mDocument;
            SavingState& mState;

        public:

            WritePathgridCollectionStage (Document& document, SavingState& state);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };


    class WriteLandCollectionStage : public Stage
    {
            Document& mDocument;
            SavingState& mState;

        public:

            WriteLandCollectionStage (Document& document, SavingState& state);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };


    class WriteLandTextureCollectionStage : public Stage
    {
            Document& mDocument;
            SavingState& mState;

        public:

            WriteLandTextureCollectionStage (Document& document, SavingState& state);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    class CloseSaveStage : public Stage
    {
            SavingState& mState;

        public:

            CloseSaveStage (SavingState& state);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    class FinalSavingStage : public Stage
    {
            Document& mDocument;
            SavingState& mState;

        public:

            FinalSavingStage (Document& document, SavingState& state);

            int setup() override;
            ///< \return number of steps

            void perform (int stage, Messages& messages) override;
            ///< Messages resulting from this stage will be appended to \a messages.
    };
}

#endif
