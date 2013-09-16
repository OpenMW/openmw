
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


    appendStage (new CloseSaveStage (mState));

    appendStage (new FinalSavingStage (mDocument, mState));
}