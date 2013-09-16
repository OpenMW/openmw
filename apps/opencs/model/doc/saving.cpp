
#include "saving.hpp"

#include <components/esm/defs.hpp>

#include "../world/data.hpp"
#include "../world/idcollection.hpp"

#include "state.hpp"
#include "savingstages.hpp"
#include "document.hpp"

CSMDoc::Saving::Saving (Document& document)
: Operation (State_Saving, true, true), mDocument (document), mState (*this)
{
    appendStage (new OpenSaveStage (mDocument, mState));

    appendStage (new WriteHeaderStage (mDocument, mState));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Global> >
        (mDocument.getData().getGlobals(), mState, ESM::REC_GLOB));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::GameSetting> >
        (mDocument.getData().getGmsts(), mState, ESM::REC_GMST));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Skill> >
        (mDocument.getData().getSkills(), mState, ESM::REC_SKIL));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Class> >
        (mDocument.getData().getClasses(), mState, ESM::REC_CLAS));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Faction> >
        (mDocument.getData().getFactions(), mState, ESM::REC_FACT));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Race> >
        (mDocument.getData().getRaces(), mState, ESM::REC_RACE));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Sound> >
        (mDocument.getData().getSounds(), mState, ESM::REC_SOUN));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Script> >
        (mDocument.getData().getScripts(), mState, ESM::REC_SCPT));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Region> >
        (mDocument.getData().getRegions(), mState, ESM::REC_REGN));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::BirthSign> >
        (mDocument.getData().getBirthsigns(), mState, ESM::REC_BSGN));

    appendStage (new WriteCollectionStage<CSMWorld::IdCollection<ESM::Spell> >
        (mDocument.getData().getSpells(), mState, ESM::REC_SPEL));


    appendStage (new CloseSaveStage (mState));

    appendStage (new FinalSavingStage (mDocument, mState));
}