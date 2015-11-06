#include "subviews.hpp"

#include "../doc/subviewfactoryimp.hpp"

#include "reportsubview.hpp"
#include "searchsubview.hpp"

void CSVTools::addSubViewFactories (CSVDoc::SubViewFactoryManager& manager)
{
    manager.add (CSMWorld::UniversalId::Type_VerificationResults,
        new CSVDoc::SubViewFactory<ReportSubView>);
    manager.add (CSMWorld::UniversalId::Type_LoadErrorLog,
        new CSVDoc::SubViewFactory<ReportSubView>);
    manager.add (CSMWorld::UniversalId::Type_Search,
        new CSVDoc::SubViewFactory<SearchSubView>);
}
