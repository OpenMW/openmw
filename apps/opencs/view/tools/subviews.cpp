
#include "subviews.hpp"

#include "../doc/subviewfactoryimp.hpp"

#include "reportsubview.hpp"

void CSVTools::addSubViewFactories (CSVDoc::SubViewFactoryManager& manager)
{
    manager.add (CSMWorld::UniversalId::Type_VerificationResults,
        new CSVDoc::SubViewFactory<ReportSubView>);
}