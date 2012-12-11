
#include "subviews.hpp"

#include "../doc/subviewfactoryimp.hpp"

#include "tablesubview.hpp"

void CSVWorld::addSubViewFactories (CSVDoc::SubViewFactoryManager& manager)
{
    manager.add (CSMWorld::UniversalId::Type_Globals,
        new CSVDoc::SubViewFactoryWithCreateFlag<TableSubView> (true));
}