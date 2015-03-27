
#include "searchoperation.hpp"

#include "../doc/state.hpp"
#include "../doc/document.hpp"

#include "../world/data.hpp"
#include "../world/idtablebase.hpp"

#include "searchstage.hpp"

CSMTools::SearchOperation::SearchOperation (CSMDoc::Document& document)
: CSMDoc::Operation (CSMDoc::State_Searching, false)
{
appendStage (new SearchStage (&dynamic_cast<CSMWorld::IdTableBase&> (*document.getData().getTableModel (CSMWorld::UniversalId::Type_Cells))));

}

void CSMTools::SearchOperation::configure (const Search& search)
{
    mSearch = search;
}

void CSMTools::SearchOperation::appendStage (SearchStage *stage)
{
    CSMDoc::Operation::appendStage (stage);
    stage->setOperation (this);
}

const CSMTools::Search& CSMTools::SearchOperation::getSearch() const
{
    return mSearch;
}
