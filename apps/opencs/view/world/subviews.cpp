
#include "subviews.hpp"

#include "../doc/subviewfactoryimp.hpp"

#include "tablesubview.hpp"
#include "dialoguesubview.hpp"

void CSVWorld::addSubViewFactories (CSVDoc::SubViewFactoryManager& manager)
{
    manager.add (CSMWorld::UniversalId::Type_Globals,
        new CSVDoc::SubViewFactoryWithCreateFlag<TableSubView> (true));

    manager.add (CSMWorld::UniversalId::Type_Global,
        new CSVDoc::SubViewFactoryWithCreateFlag<DialogueSubView> (true));
}