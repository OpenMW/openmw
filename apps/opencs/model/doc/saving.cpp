
#include "saving.hpp"

#include "state.hpp"

#include "savingstages.hpp"

CSMDoc::Saving::Saving (Document& document)
: Operation (State_Saving, true, true), mDocument (document), mState (*this)
{


    appendStage (new FinalSavingStage (mDocument, mState));
}