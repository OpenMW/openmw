#include "saving.hpp"

#include <algorithm>
#include <variant>

#include <apps/opencs/model/doc/operation.hpp>
#include <apps/opencs/model/world/record.hpp>
#include <apps/opencs/model/world/scope.hpp>

#include <components/esm3/debugprofile.hpp>
#include <components/esm3/filter.hpp>
#include <components/esm3/loadbody.hpp>
#include <components/esm3/loadbsgn.hpp>
#include <components/esm3/loadclas.hpp>
#include <components/esm3/loadench.hpp>
#include <components/esm3/loadfact.hpp>
#include <components/esm3/loadglob.hpp>
#include <components/esm3/loadgmst.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadrace.hpp>
#include <components/esm3/loadregn.hpp>
#include <components/esm3/loadscpt.hpp>
#include <components/esm3/loadskil.hpp>
#include <components/esm3/loadsndg.hpp>
#include <components/esm3/loadsoun.hpp>
#include <components/esm3/loadspel.hpp>
#include <components/esm3/loadsscr.hpp>
#include <components/esm3/selectiongroup.hpp>

#include "../world/data.hpp"
#include "../world/idcollection.hpp"

#include "document.hpp"
#include "savingstages.hpp"
#include "state.hpp"

CSMDoc::Saving::Saving(Document& document, const std::filesystem::path& projectPath, ToUTF8::FromType encoding)
    : Operation(State_Saving, true, true)
    , mDocument(document)
    , mState(*this, projectPath, encoding)
{
    // save project file
    appendStage(new OpenSaveStage(mDocument, mState, true));

    appendStage(new WriteHeaderStage(mDocument, mState, true));

    appendStage(new WriteCollectionStage<CSMWorld::IdCollection<ESM::Filter>>(
        mDocument.getData().getFilters(), mState, CSMWorld::Scope_Project));

    appendStage(new WriteCollectionStage<CSMWorld::IdCollection<ESM::DebugProfile>>(
        mDocument.getData().getDebugProfiles(), mState, CSMWorld::Scope_Project));

    appendStage(new WriteCollectionStage<CSMWorld::IdCollection<ESM::Script>>(
        mDocument.getData().getScripts(), mState, CSMWorld::Scope_Project));

    appendStage(new WriteCollectionStage<CSMWorld::IdCollection<ESM::SelectionGroup>>(
        mDocument.getData().getSelectionGroups(), mState, CSMWorld::Scope_Project));

    appendStage(new CloseSaveStage(mState));

    // save content file
    appendStage(new OpenSaveStage(mDocument, mState, false));

    appendStage(new WriteHeaderStage(mDocument, mState, false));

    appendStage(
        new WriteCollectionStage<CSMWorld::IdCollection<ESM::Global>>(mDocument.getData().getGlobals(), mState));

    appendStage(
        new WriteCollectionStage<CSMWorld::IdCollection<ESM::GameSetting>>(mDocument.getData().getGmsts(), mState));

    appendStage(new WriteCollectionStage<CSMWorld::IdCollection<ESM::Skill>>(mDocument.getData().getSkills(), mState));

    appendStage(new WriteCollectionStage<CSMWorld::IdCollection<ESM::Class>>(mDocument.getData().getClasses(), mState));

    appendStage(
        new WriteCollectionStage<CSMWorld::IdCollection<ESM::Faction>>(mDocument.getData().getFactions(), mState));

    appendStage(new WriteCollectionStage<CSMWorld::IdCollection<ESM::Race>>(mDocument.getData().getRaces(), mState));

    appendStage(new WriteCollectionStage<CSMWorld::IdCollection<ESM::Sound>>(mDocument.getData().getSounds(), mState));

    appendStage(
        new WriteCollectionStage<CSMWorld::IdCollection<ESM::Script>>(mDocument.getData().getScripts(), mState));

    appendStage(
        new WriteCollectionStage<CSMWorld::IdCollection<ESM::Region>>(mDocument.getData().getRegions(), mState));

    appendStage(
        new WriteCollectionStage<CSMWorld::IdCollection<ESM::BirthSign>>(mDocument.getData().getBirthsigns(), mState));

    appendStage(new WriteCollectionStage<CSMWorld::IdCollection<ESM::Spell>>(mDocument.getData().getSpells(), mState));

    appendStage(new WriteCollectionStage<CSMWorld::IdCollection<ESM::Enchantment>>(
        mDocument.getData().getEnchantments(), mState));

    appendStage(
        new WriteCollectionStage<CSMWorld::IdCollection<ESM::BodyPart>>(mDocument.getData().getBodyParts(), mState));

    appendStage(new WriteCollectionStage<CSMWorld::IdCollection<ESM::MagicEffect>>(
        mDocument.getData().getMagicEffects(), mState));

    appendStage(new WriteCollectionStage<CSMWorld::IdCollection<ESM::StartScript>>(
        mDocument.getData().getStartScripts(), mState));

    appendStage(new WriteRefIdCollectionStage(mDocument, mState));

    // Can reference creatures so needs to load after them for TESCS compatibility
    appendStage(new WriteCollectionStage<CSMWorld::IdCollection<ESM::SoundGenerator>>(
        mDocument.getData().getSoundGens(), mState));

    appendStage(new CollectionReferencesStage(mDocument, mState));

    appendStage(new WriteCellCollectionStage(mDocument, mState));

    // Dialogue can reference objects, cells, and journals so must be written after these records for vanilla-compatible
    // files

    appendStage(new WriteDialogueCollectionStage(mDocument, mState, true));

    appendStage(new WriteDialogueCollectionStage(mDocument, mState, false));

    appendStage(new WritePathgridCollectionStage(mDocument, mState));

    appendStage(new WriteLandTextureCollectionStage(mDocument, mState));

    // references Land Textures
    appendStage(new WriteLandCollectionStage(mDocument, mState));

    // close file and clean up
    appendStage(new CloseSaveStage(mState));

    appendStage(new FinalSavingStage(mDocument, mState));
}
