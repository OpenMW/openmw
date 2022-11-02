#include "mergeoperation.hpp"

#include <utility>
#include <variant>

#include <apps/opencs/model/doc/operation.hpp>
#include <apps/opencs/model/tools/mergestate.hpp>
#include <apps/opencs/model/world/cell.hpp>
#include <apps/opencs/model/world/data.hpp>
#include <apps/opencs/model/world/info.hpp>
#include <apps/opencs/model/world/infocollection.hpp>
#include <apps/opencs/model/world/pathgrid.hpp>
#include <apps/opencs/model/world/record.hpp>
#include <apps/opencs/model/world/subcellcollection.hpp>

#include <components/esm3/debugprofile.hpp>
#include <components/esm3/filter.hpp>
#include <components/esm3/loadbody.hpp>
#include <components/esm3/loadbsgn.hpp>
#include <components/esm3/loadclas.hpp>
#include <components/esm3/loaddial.hpp>
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

#include "../doc/document.hpp"
#include "../doc/state.hpp"

#include "mergestages.hpp"

CSMTools::MergeOperation::MergeOperation(CSMDoc::Document& document, ToUTF8::FromType encoding)
    : CSMDoc::Operation(CSMDoc::State_Merging, true)
    , mState(document)
{
    appendStage(new StartMergeStage(mState));

    appendStage(new MergeIdCollectionStage<ESM::Global>(mState, &CSMWorld::Data::getGlobals));
    appendStage(new MergeIdCollectionStage<ESM::GameSetting>(mState, &CSMWorld::Data::getGmsts));
    appendStage(new MergeIdCollectionStage<ESM::Skill>(mState, &CSMWorld::Data::getSkills));
    appendStage(new MergeIdCollectionStage<ESM::Class>(mState, &CSMWorld::Data::getClasses));
    appendStage(new MergeIdCollectionStage<ESM::Faction>(mState, &CSMWorld::Data::getFactions));
    appendStage(new MergeIdCollectionStage<ESM::Race>(mState, &CSMWorld::Data::getRaces));
    appendStage(new MergeIdCollectionStage<ESM::Sound>(mState, &CSMWorld::Data::getSounds));
    appendStage(new MergeIdCollectionStage<ESM::Script>(mState, &CSMWorld::Data::getScripts));
    appendStage(new MergeIdCollectionStage<ESM::Region>(mState, &CSMWorld::Data::getRegions));
    appendStage(new MergeIdCollectionStage<ESM::BirthSign>(mState, &CSMWorld::Data::getBirthsigns));
    appendStage(new MergeIdCollectionStage<ESM::Spell>(mState, &CSMWorld::Data::getSpells));
    appendStage(new MergeIdCollectionStage<ESM::Dialogue>(mState, &CSMWorld::Data::getTopics));
    appendStage(new MergeIdCollectionStage<ESM::Dialogue>(mState, &CSMWorld::Data::getJournals));
    appendStage(new MergeIdCollectionStage<CSMWorld::Cell>(mState, &CSMWorld::Data::getCells));
    appendStage(new MergeIdCollectionStage<ESM::Filter>(mState, &CSMWorld::Data::getFilters));
    appendStage(new MergeIdCollectionStage<ESM::Enchantment>(mState, &CSMWorld::Data::getEnchantments));
    appendStage(new MergeIdCollectionStage<ESM::BodyPart>(mState, &CSMWorld::Data::getBodyParts));
    appendStage(new MergeIdCollectionStage<ESM::DebugProfile>(mState, &CSMWorld::Data::getDebugProfiles));
    appendStage(new MergeIdCollectionStage<ESM::SoundGenerator>(mState, &CSMWorld::Data::getSoundGens));
    appendStage(new MergeIdCollectionStage<ESM::MagicEffect>(mState, &CSMWorld::Data::getMagicEffects));
    appendStage(new MergeIdCollectionStage<ESM::StartScript>(mState, &CSMWorld::Data::getStartScripts));
    appendStage(new MergeIdCollectionStage<CSMWorld::Pathgrid, CSMWorld::SubCellCollection<CSMWorld::Pathgrid>>(
        mState, &CSMWorld::Data::getPathgrids));
    appendStage(
        new MergeIdCollectionStage<CSMWorld::Info, CSMWorld::InfoCollection>(mState, &CSMWorld::Data::getTopicInfos));
    appendStage(
        new MergeIdCollectionStage<CSMWorld::Info, CSMWorld::InfoCollection>(mState, &CSMWorld::Data::getJournalInfos));
    appendStage(new MergeRefIdsStage(mState));
    appendStage(new MergeReferencesStage(mState));
    appendStage(new MergeReferencesStage(mState));
    appendStage(new PopulateLandTexturesMergeStage(mState));
    appendStage(new MergeLandStage(mState));
    appendStage(new FixLandsAndLandTexturesMergeStage(mState));
    appendStage(new CleanupLandTexturesMergeStage(mState));

    appendStage(new FinishMergedDocumentStage(mState, encoding));
}

void CSMTools::MergeOperation::setTarget(std::unique_ptr<CSMDoc::Document> document)
{
    mState.mTarget = std::move(document);
}

void CSMTools::MergeOperation::operationDone()
{
    CSMDoc::Operation::operationDone();

    if (mState.mCompleted)
        emit mergeDone(mState.mTarget.release());
}
