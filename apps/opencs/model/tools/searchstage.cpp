#include "searchstage.hpp"

#include "../world/idtablebase.hpp"

#include "searchoperation.hpp"

CSMTools::SearchStage::SearchStage (const CSMWorld::IdTableBase *model)
: mModel (model), mOperation (nullptr)
{}

int CSMTools::SearchStage::setup()
{
    if (mOperation)
        mSearch = mOperation->getSearch();

    mSearch.configure (mModel);
        
    return mModel->rowCount();
}

void CSMTools::SearchStage::perform (int stage, CSMDoc::Messages& messages)
{
    mSearch.searchRow (mModel, stage, messages);
}

void CSMTools::SearchStage::setOperation (const SearchOperation *operation)
{
    mOperation = operation;
}
