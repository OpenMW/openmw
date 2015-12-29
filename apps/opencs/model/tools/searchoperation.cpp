#include "searchoperation.hpp"

#include "../doc/state.hpp"
#include "../doc/document.hpp"

#include "../world/data.hpp"
#include "../world/idtablebase.hpp"

#include "searchstage.hpp"

CSMTools::SearchOperation::SearchOperation (CSMDoc::Document& document)
: CSMDoc::Operation (CSMDoc::State_Searching, false)
{
    std::vector<CSMWorld::UniversalId::Type> types = CSMWorld::UniversalId::listTypes (
        CSMWorld::UniversalId::Class_RecordList |
        CSMWorld::UniversalId::Class_ResourceList
        );

    for (std::vector<CSMWorld::UniversalId::Type>::const_iterator iter (types.begin());
        iter!=types.end(); ++iter)
        appendStage (new SearchStage (&dynamic_cast<CSMWorld::IdTableBase&> (
            *document.getData().getTableModel (*iter))));

    setDefaultSeverity (CSMDoc::Message::Severity_Info);
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
