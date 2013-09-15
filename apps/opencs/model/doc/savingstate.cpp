
#include "savingstate.hpp"

#include "operation.hpp"

CSMDoc::SavingState::SavingState (Operation& operation)
: mOperation (operation)
{}

bool CSMDoc::SavingState::hasError() const
{
    return mOperation.hasError();
}