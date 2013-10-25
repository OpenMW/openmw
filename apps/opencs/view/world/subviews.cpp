
#include "subviews.hpp"

#include "../doc/subviewfactoryimp.hpp"

#include "../filter/filtercreator.hpp"

#include "tablesubview.hpp"
#include "dialoguesubview.hpp"
#include "scriptsubview.hpp"
#include "regionmapsubview.hpp"
#include "genericcreator.hpp"
#include "cellcreator.hpp"
#include "referenceablecreator.hpp"
#include "referencecreator.hpp"
#include "scenesubview.hpp"

void CSVWorld::addSubViewFactories (CSVDoc::SubViewFactoryManager& manager)
{
    // Regular record tables (including references which are actually sub-records, but are promoted
    // to top-level records within the editor)
    manager.add (CSMWorld::UniversalId::Type_Gmsts,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, NullCreatorFactory>);

    manager.add (CSMWorld::UniversalId::Type_Skills,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, NullCreatorFactory>);

    static const CSMWorld::UniversalId::Type sTableTypes[] =
    {
        CSMWorld::UniversalId::Type_Globals,
        CSMWorld::UniversalId::Type_Classes,
        CSMWorld::UniversalId::Type_Factions,
        CSMWorld::UniversalId::Type_Races,
        CSMWorld::UniversalId::Type_Sounds,
        CSMWorld::UniversalId::Type_Scripts,
        CSMWorld::UniversalId::Type_Regions,
        CSMWorld::UniversalId::Type_Birthsigns,
        CSMWorld::UniversalId::Type_Spells,

        CSMWorld::UniversalId::Type_None // end marker
    };

    for (int i=0; sTableTypes[i]!=CSMWorld::UniversalId::Type_None; ++i)
        manager.add (sTableTypes[i],
            new CSVDoc::SubViewFactoryWithCreator<TableSubView, CreatorFactory<GenericCreator> >);

    manager.add (CSMWorld::UniversalId::Type_Cells,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, CreatorFactory<CellCreator> >);

    manager.add (CSMWorld::UniversalId::Type_Referenceables,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, CreatorFactory<ReferenceableCreator> >);

    manager.add (CSMWorld::UniversalId::Type_References,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, CreatorFactory<ReferenceCreator> >);

    // Subviews for editing/viewing individual records
    manager.add (CSMWorld::UniversalId::Type_Script, new CSVDoc::SubViewFactory<ScriptSubView>);

    // Other stuff (combined record tables)
    manager.add (CSMWorld::UniversalId::Type_RegionMap, new CSVDoc::SubViewFactory<RegionMapSubView>);

    manager.add (CSMWorld::UniversalId::Type_Filters,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView,
        CreatorFactory<CSVFilter::FilterCreator> >);

    manager.add (CSMWorld::UniversalId::Type_Scene, new CSVDoc::SubViewFactory<SceneSubView>);
}