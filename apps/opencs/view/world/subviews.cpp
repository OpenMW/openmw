#include "subviews.hpp"

#include "../doc/subviewfactoryimp.hpp"

#include "tablesubview.hpp"
#include "dialoguesubview.hpp"
#include "scriptsubview.hpp"
#include "regionmapsubview.hpp"
#include "genericcreator.hpp"
#include "globalcreator.hpp"
#include "cellcreator.hpp"
#include "referenceablecreator.hpp"
#include "referencecreator.hpp"
#include "startscriptcreator.hpp"
#include "scenesubview.hpp"
#include "dialoguecreator.hpp"
#include "infocreator.hpp"
#include "pathgridcreator.hpp"
#include "previewsubview.hpp"
#include "bodypartcreator.hpp"
#include "landcreator.hpp"
#include "landtexturecreator.hpp"

void CSVWorld::addSubViewFactories (CSVDoc::SubViewFactoryManager& manager)
{
    // Regular record tables (including references which are actually sub-records, but are promoted
    // to top-level records within the editor)
    manager.add (CSMWorld::UniversalId::Type_Gmsts,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, NullCreatorFactory>);

    manager.add (CSMWorld::UniversalId::Type_Skills,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, NullCreatorFactory>);

    manager.add (CSMWorld::UniversalId::Type_MagicEffects,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, NullCreatorFactory>);

    static const CSMWorld::UniversalId::Type sTableTypes[] =
    {
        CSMWorld::UniversalId::Type_Classes,
        CSMWorld::UniversalId::Type_Factions,
        CSMWorld::UniversalId::Type_Races,
        CSMWorld::UniversalId::Type_Sounds,
        CSMWorld::UniversalId::Type_Regions,
        CSMWorld::UniversalId::Type_Birthsigns,
        CSMWorld::UniversalId::Type_Spells,
        CSMWorld::UniversalId::Type_Enchantments,
        CSMWorld::UniversalId::Type_SoundGens,

        CSMWorld::UniversalId::Type_None // end marker
    };

    for (int i=0; sTableTypes[i]!=CSMWorld::UniversalId::Type_None; ++i)
        manager.add (sTableTypes[i],
            new CSVDoc::SubViewFactoryWithCreator<TableSubView, CreatorFactory<GenericCreator> >);

    manager.add (CSMWorld::UniversalId::Type_BodyParts,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, CreatorFactory<BodyPartCreator> >);

    manager.add (CSMWorld::UniversalId::Type_StartScripts,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, StartScriptCreatorFactory>);

    manager.add (CSMWorld::UniversalId::Type_Cells,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, CreatorFactory<CellCreator> >);

    manager.add (CSMWorld::UniversalId::Type_Referenceables,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, CreatorFactory<ReferenceableCreator> >);

    manager.add (CSMWorld::UniversalId::Type_References,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, ReferenceCreatorFactory>);

    manager.add (CSMWorld::UniversalId::Type_Topics,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, TopicCreatorFactory>);

    manager.add (CSMWorld::UniversalId::Type_Journals,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, JournalCreatorFactory>);

    manager.add (CSMWorld::UniversalId::Type_TopicInfos,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, InfoCreatorFactory>(false));

    manager.add (CSMWorld::UniversalId::Type_JournalInfos,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, InfoCreatorFactory>(false));

    manager.add (CSMWorld::UniversalId::Type_Pathgrids,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, PathgridCreatorFactory>);

    manager.add (CSMWorld::UniversalId::Type_Lands,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, CreatorFactory<LandCreator> >);

    manager.add (CSMWorld::UniversalId::Type_LandTextures,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, CreatorFactory<LandTextureCreator> >);

    manager.add (CSMWorld::UniversalId::Type_Globals,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, CreatorFactory<GlobalCreator> >);

    // Subviews for resources tables
    manager.add (CSMWorld::UniversalId::Type_Meshes,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, NullCreatorFactory>);
    manager.add (CSMWorld::UniversalId::Type_Icons,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, NullCreatorFactory>);
    manager.add (CSMWorld::UniversalId::Type_Musics,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, NullCreatorFactory>);
    manager.add (CSMWorld::UniversalId::Type_SoundsRes,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, NullCreatorFactory>);
    manager.add (CSMWorld::UniversalId::Type_Textures,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, NullCreatorFactory>);
    manager.add (CSMWorld::UniversalId::Type_Videos,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, NullCreatorFactory>);


    // Subviews for editing/viewing individual records
    manager.add (CSMWorld::UniversalId::Type_Script, new CSVDoc::SubViewFactory<ScriptSubView>);

    // Other stuff (combined record tables)
    manager.add (CSMWorld::UniversalId::Type_RegionMap, new CSVDoc::SubViewFactory<RegionMapSubView>);

    manager.add (CSMWorld::UniversalId::Type_Scene, new CSVDoc::SubViewFactory<SceneSubView>);

    // More other stuff
    manager.add (CSMWorld::UniversalId::Type_Filters,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView,
        CreatorFactory<GenericCreator, CSMWorld::Scope_Project | CSMWorld::Scope_Session> >);

    manager.add (CSMWorld::UniversalId::Type_DebugProfiles,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView,
        CreatorFactory<GenericCreator, CSMWorld::Scope_Project | CSMWorld::Scope_Session> >);

    manager.add (CSMWorld::UniversalId::Type_Scripts,
        new CSVDoc::SubViewFactoryWithCreator<TableSubView, CreatorFactory<GenericCreator,
            CSMWorld::Scope_Project | CSMWorld::Scope_Content> >);

    // Dialogue subviews
    static const CSMWorld::UniversalId::Type sTableTypes2[] =
    {
        CSMWorld::UniversalId::Type_Region,
        CSMWorld::UniversalId::Type_Spell,
        CSMWorld::UniversalId::Type_Birthsign,
        CSMWorld::UniversalId::Type_Global,
        CSMWorld::UniversalId::Type_Race,
        CSMWorld::UniversalId::Type_Class,
        CSMWorld::UniversalId::Type_Sound,
        CSMWorld::UniversalId::Type_Faction,
        CSMWorld::UniversalId::Type_Enchantment,
        CSMWorld::UniversalId::Type_SoundGen,

        CSMWorld::UniversalId::Type_None // end marker
    };

    for (int i=0; sTableTypes2[i]!=CSMWorld::UniversalId::Type_None; ++i)
        manager.add (sTableTypes2[i],
            new CSVDoc::SubViewFactoryWithCreator<DialogueSubView,
            CreatorFactory<GenericCreator> > (false));

    manager.add (CSMWorld::UniversalId::Type_BodyPart,
        new CSVDoc::SubViewFactoryWithCreator<DialogueSubView, CreatorFactory<BodyPartCreator> > (false));

    manager.add (CSMWorld::UniversalId::Type_StartScript,
        new CSVDoc::SubViewFactoryWithCreator<DialogueSubView, StartScriptCreatorFactory>(false));

    manager.add (CSMWorld::UniversalId::Type_Skill,
        new CSVDoc::SubViewFactoryWithCreator<DialogueSubView, NullCreatorFactory > (false));

    manager.add (CSMWorld::UniversalId::Type_MagicEffect,
        new CSVDoc::SubViewFactoryWithCreator<DialogueSubView, NullCreatorFactory > (false));

    manager.add (CSMWorld::UniversalId::Type_Gmst,
        new CSVDoc::SubViewFactoryWithCreator<DialogueSubView, NullCreatorFactory > (false));

    manager.add (CSMWorld::UniversalId::Type_Referenceable,
        new CSVDoc::SubViewFactoryWithCreator<DialogueSubView, CreatorFactory<ReferenceableCreator> > (false));

    manager.add (CSMWorld::UniversalId::Type_Reference,
        new CSVDoc::SubViewFactoryWithCreator<DialogueSubView, ReferenceCreatorFactory> (false));

    manager.add (CSMWorld::UniversalId::Type_Cell,
        new CSVDoc::SubViewFactoryWithCreator<DialogueSubView, CreatorFactory<CellCreator> > (false));

    manager.add (CSMWorld::UniversalId::Type_JournalInfo,
        new CSVDoc::SubViewFactoryWithCreator<DialogueSubView, InfoCreatorFactory> (false));

    manager.add (CSMWorld::UniversalId::Type_TopicInfo,
        new CSVDoc::SubViewFactoryWithCreator<DialogueSubView, InfoCreatorFactory>(false));

    manager.add (CSMWorld::UniversalId::Type_Topic,
        new CSVDoc::SubViewFactoryWithCreator<DialogueSubView, TopicCreatorFactory> (false));

    manager.add (CSMWorld::UniversalId::Type_Journal,
        new CSVDoc::SubViewFactoryWithCreator<DialogueSubView, JournalCreatorFactory> (false));

    manager.add (CSMWorld::UniversalId::Type_Pathgrid,
        new CSVDoc::SubViewFactoryWithCreator<DialogueSubView, PathgridCreatorFactory> (false));

    manager.add (CSMWorld::UniversalId::Type_Land,
        new CSVDoc::SubViewFactoryWithCreator<DialogueSubView, CreatorFactory<LandCreator> >(false));

    manager.add (CSMWorld::UniversalId::Type_LandTexture,
        new CSVDoc::SubViewFactoryWithCreator<DialogueSubView, CreatorFactory<LandTextureCreator> >(false));

    manager.add (CSMWorld::UniversalId::Type_DebugProfile,
        new CSVDoc::SubViewFactoryWithCreator<DialogueSubView, CreatorFactory<GenericCreator, CSMWorld::Scope_Project | CSMWorld::Scope_Session> > (false));

    manager.add (CSMWorld::UniversalId::Type_Filter,
        new CSVDoc::SubViewFactoryWithCreator<DialogueSubView, CreatorFactory<GenericCreator, CSMWorld::Scope_Project | CSMWorld::Scope_Session> > (false));

    manager.add (CSMWorld::UniversalId::Type_MetaData,
        new CSVDoc::SubViewFactory<SimpleDialogueSubView >);

    //preview
    manager.add (CSMWorld::UniversalId::Type_Preview, new CSVDoc::SubViewFactory<PreviewSubView>);
}
