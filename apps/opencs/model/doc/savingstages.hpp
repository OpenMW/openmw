#ifndef CSM_DOC_SAVINGSTAGES_H
#define CSM_DOC_SAVINGSTAGES_H

#include "stage.hpp"

#include "../world/record.hpp"
#include "../world/idcollection.hpp"

#include "../filter/filter.hpp"

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

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, Messages& messages);
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

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, Messages& messages);
            ///< Messages resulting from this stage will be appended to \a messages.
    };


    template<class CollectionT>
    class WriteCollectionStage : public Stage
    {
            const CollectionT& mCollection;
            SavingState& mState;

        public:

            WriteCollectionStage (const CollectionT& collection, SavingState& state);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, Messages& messages);
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    template<class CollectionT>
    WriteCollectionStage<CollectionT>::WriteCollectionStage (const CollectionT& collection,
        SavingState& state)
    : mCollection (collection), mState (state)
    {}

    template<class CollectionT>
    int WriteCollectionStage<CollectionT>::setup()
    {
        return mCollection.getSize();
    }

    template<class CollectionT>
    void WriteCollectionStage<CollectionT>::perform (int stage, Messages& messages)
    {
        CSMWorld::RecordBase::State state = mCollection.getRecord (stage).mState;

        if (state==CSMWorld::RecordBase::State_Modified ||
            state==CSMWorld::RecordBase::State_ModifiedOnly)
        {
            std::string type;
            for (int i=0; i<4; ++i)
                /// \todo make endianess agnostic (change ESMWriter interface?)
                type += reinterpret_cast<const char *> (&mCollection.getRecord (stage).mModified.sRecordId)[i];

            mState.getWriter().startRecord (mCollection.getRecord (stage).mModified.sRecordId);
            mState.getWriter().writeHNCString ("NAME", mCollection.getId (stage));
            mCollection.getRecord (stage).mModified.save (mState.getWriter());
            mState.getWriter().endRecord (mCollection.getRecord (stage).mModified.sRecordId);
        }
        else if (state==CSMWorld::RecordBase::State_Deleted)
        {
            /// \todo write record with delete flag
        }
    }


    class WriteDialogueCollectionStage : public Stage
    {
            Document& mDocument;
            SavingState& mState;
            const CSMWorld::IdCollection<ESM::Dialogue>& mTopics;
            CSMWorld::InfoCollection& mInfos;

        public:

            WriteDialogueCollectionStage (Document& document, SavingState& state, bool journal);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, Messages& messages);
            ///< Messages resulting from this stage will be appended to \a messages.
    };


    class WriteRefIdCollectionStage : public Stage
    {
            Document& mDocument;
            SavingState& mState;

        public:

            WriteRefIdCollectionStage (Document& document, SavingState& state);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, Messages& messages);
            ///< Messages resulting from this stage will be appended to \a messages.
    };


    class WriteFilterStage : public WriteCollectionStage<CSMWorld::IdCollection<CSMFilter::Filter> >
    {
            Document& mDocument;
            CSMFilter::Filter::Scope mScope;

        public:

            WriteFilterStage (Document& document, SavingState& state, CSMFilter::Filter::Scope scope);

            virtual void perform (int stage, Messages& messages);
            ///< Messages resulting from this stage will be appended to \a messages.

    };

    class CollectionReferencesStage : public Stage
    {
            Document& mDocument;
            SavingState& mState;

        public:

            CollectionReferencesStage (Document& document, SavingState& state);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, Messages& messages);
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    class WriteCellCollectionStage : public Stage
    {
            Document& mDocument;
            SavingState& mState;

        public:

            WriteCellCollectionStage (Document& document, SavingState& state);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, Messages& messages);
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    class CloseSaveStage : public Stage
    {
            SavingState& mState;

        public:

            CloseSaveStage (SavingState& state);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, Messages& messages);
            ///< Messages resulting from this stage will be appended to \a messages.
    };

    class FinalSavingStage : public Stage
    {
            Document& mDocument;
            SavingState& mState;

        public:

            FinalSavingStage (Document& document, SavingState& state);

            virtual int setup();
            ///< \return number of steps

            virtual void perform (int stage, Messages& messages);
            ///< Messages resulting from this stage will be appended to \a messages.
    };
}

#endif
