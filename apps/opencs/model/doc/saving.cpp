
#include "saving.hpp"

#include "../world/data.hpp"
#include "../world/idcollection.hpp"

#include "state.hpp"
#include "savingstages.hpp"
#include "document.hpp"

CSMDoc::Saving::Saving (Document& document, const boost::filesystem::path& projectPath)
: Operation (State_Saving, true, true), mDocument (document), mState (*this, projectPath)
{
    // save project file
    appendStage (new OpenSaveStage (mDocument, mState, true));

    appendStage (new WriteHeaderStage (mDocument, mState, true));

    appendStage (new WriteFilterStage (mDocument, mState, CSMFilter::Filter::Scope_Project));

    appendStage (new CloseSaveStage (mState));

    // save content file
    appendStage (new OpenSaveStage (mDocument, mState, false));

    appendStage (new WriteHeaderStage (mDocument, mState, false));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Global> >
        (mDocument.getData().getGlobals(), mState));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::GameSetting> >
        (mDocument.getData().getGmsts(), mState));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Skill> >
        (mDocument.getData().getSkills(), mState));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Class> >
        (mDocument.getData().getClasses(), mState));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Faction> >
        (mDocument.getData().getFactions(), mState));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Race> >
        (mDocument.getData().getRaces(), mState));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Sound> >
        (mDocument.getData().getSounds(), mState));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Script> >
        (mDocument.getData().getScripts(), mState));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Region> >
        (mDocument.getData().getRegions(), mState));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::BirthSign> >
        (mDocument.getData().getBirthsigns(), mState));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Spell> >
        (mDocument.getData().getSpells(), mState));

    /// \todo deal with info records for topcis and journals
    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Dialogue> >
        (mDocument.getData().getTopics(), mState));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Dialogue> >
        (mDocument.getData().getJournals(), mState));

    appendStage (new WriteRefIdCollectionStage (mDocument, mState));


    appendStage (new CloseSaveStage (mState));

    appendStage (new FinalSavingStage (mDocument, mState));
}