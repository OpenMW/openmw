#include "tes3util.hpp"

#include "functions/ai.hpp"
#include "functions/gui.hpp"
#include "functions/mechanics.hpp"
#include "functions/sound.hpp"
#include "functions/world.hpp"

/*
#include "TES3GameFile.h"

#include "TES3Util.h"
#include "UIUtil.h"
#include "LuaUtil.h"
#include "Stack.h"
#include "Log.h"
#include "ScriptUtil.h"
#include "CodePatchUtil.h"

#include "NICamera.h"
#include "NINode.h"
#include "NIPick.h"
#include "NIRTTI.h"
#include "NIStream.h"
#include "NITriShape.h"

#include "TES3AnimationData.h"
#include "TES3Actor.h"
#include "TES3AIData.h"
#include "TES3AIPackage.h"
#include "TES3Alchemy.h"
#include "TES3Armor.h"
#include "TES3AudioController.h"
#include "TES3Cell.h"
#include "TES3Class.h"
#include "TES3Container.h"
#include "TES3Creature.h"
#include "TES3CrimeEventList.h"
#include "TES3DataHandler.h"
#include "TES3Dialogue.h"
#include "TES3DialogueInfo.h"
#include "TES3Door.h"
#include "TES3Enchantment.h"
#include "TES3Faction.h"
#include "TES3Fader.h"
#include "TES3Game.h"
#include "TES3GameSetting.h"
#include "TES3GlobalVariable.h"
#include "TES3InputController.h"
#include "TES3ItemData.h"
#include "TES3LeveledList.h"
#include "TES3MagicEffectController.h"
#include "TES3Misc.h"
#include "TES3MobController.h"
#include "TES3MobileCreature.h"
#include "TES3MobilePlayer.h"
#include "TES3NPC.h"
#include "TES3PlayerAnimationData.h" 
#include "TES3Reference.h"
#include "TES3Region.h"
#include "TES3Script.h"
#include "TES3ScriptCompiler.h"
#include "TES3Sound.h"
#include "TES3SoundGenerator.h"
#include "TES3Spell.h"
#include "TES3SpellInstanceController.h"
#include "TES3UIElement.h"
#include "TES3UIManager.h"
#include "TES3UIMenuController.h"
#include "TES3Weather.h"
#include "TES3WeatherController.h"
#include "TES3WorldController.h"
*/

namespace MWLua
{
    void bindTES3Util()
    {
        //auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        //sol::state& state = stateHandle.state;

        bindTES3AIFunctions();
        bindTES3MechanicsFunctions();
        bindTES3SoundFunctions();
        bindTES3GUIFunctions();
        bindTES3WorldFunctions();

        /*
        // Bind function: tes3.getMobilePlayer
        state["tes3"]["getMobilePlayer"] = []() -> sol::object {
            TES3::WorldController* worldController = TES3::WorldController::get();
            if (worldController) {
                return makeLuaObject(worldController->getMobilePlayer());
            }
            return sol::nil;
        };

        // Bind function: tes3.getPlayerCell()
        state["tes3"]["getPlayerCell"] = []() -> sol::object {
            TES3::DataHandler* dataHandler = TES3::DataHandler::get();
            if (dataHandler) {
                return makeLuaObject(dataHandler->currentCell);
            }
            return sol::nil;
        };

        // Bind function: tes3.getGame
        state["tes3"]["getGame"] = []() {
            return TES3::Game::get();
        };

        // Bind function: tes3.getDataHandler
        state["tes3"]["getDataHandler"] = []() {
            return TES3::DataHandler::get();
        };

        // Bind function: tes3.getGame
        state["tes3"]["getWorldController"] = []() {
            return TES3::WorldController::get();
        };

        // Bind function: tes3.getObject
        state["tes3"]["getObject"] = [](const char* id) -> sol::object {
            TES3::DataHandler * dataHandler = TES3::DataHandler::get();
            if (dataHandler) {
                return makeLuaObject(dataHandler->nonDynamicData->resolveObject(id));
            }
            return sol::nil;
        };

        state["tes3"]["deleteObject"] = [](sol::object maybe) {
            TES3::BaseObject* object = maybe.as<TES3::BaseObject*>();
            if (object) {
                TES3::DataHandler::get()->nonDynamicData->deleteObject(object);
                object->vTable.base->destructor(object, true);
                return true;
            }
            return false;
        };

        // Bind function: tes3.getScript
        state["tes3"]["getScript"] = [](const char* id) -> sol::object {
            TES3::DataHandler * dataHandler = TES3::DataHandler::get();
            if (dataHandler) {
                return makeLuaObject(dataHandler->nonDynamicData->findScriptByName(id));
            }
            return sol::nil;
        };

        // Bind function: tes3.findGlobal
        state["tes3"]["findGlobal"] = [](const char* id) -> sol::object {
            TES3::DataHandler * dataHandler = TES3::DataHandler::get();
            if (dataHandler) {
                return makeLuaObject(dataHandler->nonDynamicData->findGlobalVariable(id));
            }
            return sol::nil;
        };

        // Bind function: tes3.findGMST
        state["tes3"]["findGMST"] = [](sol::object key) -> sol::object {
            TES3::DataHandler * dataHandler = TES3::DataHandler::get();
            if (dataHandler == nullptr) {
                return sol::nil;
            }

            if (key.is<double>()) {
                int index = key.as<double>();
                if (index >= TES3::GMST::sMonthMorningstar && index <= TES3::GMST::sWitchhunter) {
                    return makeLuaObject(dataHandler->nonDynamicData->GMSTs[index]);
                }
            }
            else if (key.is<const char*>()) {
                int index = -1;
                const char* keyStr = key.as<const char*>();
                for (int i = 0; i <= TES3::GMST::sWitchhunter; i++) {
                    if (strcmp(TES3::GameSettingInfo::get(i)->name, keyStr) == 0) {
                        return makeLuaObject(dataHandler->nonDynamicData->GMSTs[i]);
                    }
                }
            }

            return sol::nil;
        };

        // DEPRECATED: To be eventually redone after mods have transitioned away from it.
        state["tes3"]["getGMST"] = [](sol::object key) -> sol::object {
            auto& luaManager = mwse::lua::LuaManager::getInstance();
            auto stateHandle = luaManager.getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;

            // Display deprecation warning and traceback.
            logStackTrace("WARNING: Use of deprecated function tes3.getGMST. Use tes3.findGMST instead.");

            TES3::DataHandler * dataHandler = TES3::DataHandler::get();
            if (dataHandler == nullptr) {
                return sol::nil;
            }

            if (key.is<double>()) {
                int index = key.as<double>();
                if (index >= TES3::GMST::sMonthMorningstar && index <= TES3::GMST::sWitchhunter) {
                    return makeLuaObject(dataHandler->nonDynamicData->GMSTs[index]);
                }
            }
            else if (key.is<std::string>()) {
                int index = -1;
                std::string keyStr = key.as<std::string>();
                for (int i = 0; i <= TES3::GMST::sWitchhunter; i++) {
                    TES3::GameSettingInfo* info = TES3::GameSettingInfo::get(i);
                    if (strcmp(info->name, keyStr.c_str()) == 0) {
                        index = i;
                        break;
                    }
                }

                if (index != -1) {
                    return makeLuaObject(dataHandler->nonDynamicData->GMSTs[index]);
                }
            }

            return sol::nil;
        };

        // Bind function: tes3.adjustSoundVolume
        state["tes3"]["adjustSoundVolume"] = [](sol::optional<sol::table> params) {
            // Get parameters.
            TES3::Sound* sound = getOptionalParamSound(params, "sound");
            TES3::Reference* reference = getOptionalParamReference(params, "reference");
            int mix = getOptionalParam<int>(params, "mixChannel", int(TES3::AudioMixType::Effects));
            double volume = getOptionalParam<double>(params, "volume", 1.0);

            if (!sound || !reference) {
                log::getLog() << "tes3.adjustSoundVolume: Valid sound and reference required." << std::endl;
                return;
            }

            // Clamp volume.
            volume = std::max(0.0, volume);
            volume = std::min(volume, 1.0);

            // Apply mix and rescale to 0-250
            volume *= 250.0 * TES3::WorldController::get()->audioController->getMixVolume(TES3::AudioMixType(mix));

            TES3::DataHandler::get()->adjustSoundVolume(sound, reference, volume);
        };

        // Bind function: tes3.removeSound
        state["tes3"]["removeSound"] = [](sol::optional<sol::table> params) {
            // Get parameters.
            TES3::Sound* sound = getOptionalParamSound(params, "sound");
            TES3::Reference* reference = getOptionalParamReference(params, "reference");

            TES3::DataHandler::get()->removeSound(sound, reference);
        };

        // Bind function: tes3.isModActive
        state["tes3"]["isModActive"] = [](const char* modName) {
            TES3::DataHandler* dataHandler = TES3::DataHandler::get();
            if (dataHandler == nullptr) {
                return false;
            }

            for (int i = 0; i < 256; i++) {
                TES3::GameFile* gameFile = dataHandler->nonDynamicData->activeMods[i];
                if (gameFile == nullptr) {
                    return false;
                }

                // Compare mod name with this active mod.
                if (_stricmp(gameFile->filename, modName) == 0) {
                    return true;
                }
            }

            return false;
        };

        // Bind function: tes3.getModList
        state["tes3"]["getModList"] = []() -> sol::object {
            auto& luaManager = mwse::lua::LuaManager::getInstance();
            auto stateHandle = luaManager.getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;

            TES3::DataHandler* dataHandler = TES3::DataHandler::get();
            if (dataHandler == nullptr) {
                return sol::nil;
            }

            sol::table mods = state.create_table();
            for (int i = 0; i < 256; i++) {
                TES3::GameFile* gameFile = dataHandler->nonDynamicData->activeMods[i];
                if (gameFile == nullptr) {
                    break;
                }
                mods[i + 1] = static_cast<const char*>(gameFile->filename);
            }

            return mods;
        };

        // Bind function: tes3.iterateList
        state["tes3"]["iterateObjects"] = sol::overload(&iterateObjects, &iterateObjectsFiltered);

        // Bind function: tes3.getSound
        state["tes3"]["getSound"] = [](const char* id) -> sol::object {
            TES3::DataHandler * dataHandler = TES3::DataHandler::get();
            if (dataHandler) {
                return makeLuaObject(TES3::DataHandler::get()->nonDynamicData->findSound(id));
            }
            else {
                throw std::exception("Function called before Data Handler was initialized.");
            }
        };

        // Bind function: tes3.getSoundGenerator
        state["tes3"]["getSoundGenerator"] = [](std::string creatureId, unsigned int type) -> sol::object {
            auto nonDynamicData = TES3::DataHandler::get()->nonDynamicData;
            auto creature = nonDynamicData->resolveObjectByType<TES3::Creature>(creatureId, TES3::ObjectType::Creature);
            if (creature == nullptr) {
                return sol::nil;
            }

            while (creature->soundGenerator) {
                creature = creature->soundGenerator;
            }

            auto soundGenerators = nonDynamicData->soundGenerators;
            const char* id = creature->getObjectID();
            size_t idLength = strnlen_s(id, 32);
            for (auto itt = soundGenerators->head; itt != nullptr; itt = itt->next) {
                if (itt->data->soundType != static_cast<TES3::SoundType>(type)) {
                    continue;
                }

                if (_strnicmp(id, itt->data->name, idLength) == 0) {
                    return makeLuaObject(itt->data);
                }
            }

            return sol::nil;
        };

        state["tes3"]["getFaction"] = [](const char* id) -> sol::object {
            TES3::DataHandler * dataHandler = TES3::DataHandler::get();
            if (dataHandler) {
                return makeLuaObject(dataHandler->nonDynamicData->findFaction(id));
            }
            return sol::nil;
        };

        // Bind function: tes3.getCameraVector
        // This function currently calls out to MGE, which should be changed at some point.
        state["tes3"]["getCameraVector"] = []() {
            Stack& stack = Stack::getInstance();
            mwscript::RunOriginalOpCode(NULL, NULL, OpCode::MGEGetEyeVec);

            // Get the results from the MWSE stack.
            float x = stack.popFloat();
            float y = stack.popFloat();
            float z = stack.popFloat();

            return std::make_shared<TES3::Vector3>(x, y, z);
        };

        // Bind function: tes3.getCameraPosition
        state["tes3"]["getCameraPosition"] = []() -> sol::optional<TES3::Vector3> {
            TES3::WorldController * worldController = TES3::WorldController::get();
            if (worldController) {
                return worldController->worldCamera.camera->worldBoundOrigin;
            }
            return sol::optional<TES3::Vector3>();
        };

        // Bind function: tes3.getPlayerEyePosition
        state["tes3"]["getPlayerEyePosition"] = []() -> sol::optional<TES3::Vector3> {
            auto worldController = TES3::WorldController::get();
            if (worldController) {
                auto mobilePlayer = worldController->getMobilePlayer();
                if (mobilePlayer) {
                    return mobilePlayer->animationData.asPlayer->firstPersonHeadCameraNode->worldTransform.translation;
                }
            }
            return sol::optional<TES3::Vector3>();
        };

        // Bind function: tes3.getPlayerEyeVector
        state["tes3"]["getPlayerEyeVector"] = []() -> sol::optional<TES3::Vector3> {
            auto worldController = TES3::WorldController::get();
            if (worldController) {
                auto rotation = worldController->armCamera.cameraRoot->localRotation;
                return TES3::Vector3(rotation->m0.y, rotation->m1.y, rotation->m2.y);
            }
            return sol::optional<TES3::Vector3>();
        };

        // Bind function: tes3.rayTest
        static NI::Pick* rayTestCache = nullptr;
        static std::vector<NI::AVObject*> rayTestIgnoreRoots;
        state["tes3"]["rayTest"] = [](sol::table params) -> sol::object {
            auto& luaManager = mwse::lua::LuaManager::getInstance();
            auto stateHandle = luaManager.getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;

            // Make sure we got our required position.
            sol::optional<TES3::Vector3> position = getOptionalParamVector3(params, "position");
            if (!position) {
                return false;
            }

            // Make sure we got our required direction.
            sol::optional<TES3::Vector3> direction = getOptionalParamVector3(params, "direction");
            if (!direction) {
                return false;
            }

            // Get optional maximum search distance.
            double maxDistance = getOptionalParam<double>(params, "maxDistance", 0.0);

            // Create our pick if it doesn't exist.
            if (rayTestCache == nullptr) {
                rayTestCache = NI::Pick::malloc();
            }

            // Or clean up results otherwise.
            else {
                rayTestCache->clearResults();
            }

            // TODO: Allow specifying the root?
            rayTestCache->root = TES3::Game::get()->worldRoot;

            // Are we finding all or the first?
            if (getOptionalParam<bool>(params, "findAll", false)) {
                rayTestCache->pickType = NI::PickType::FIND_ALL;
            }
            else {
                rayTestCache->pickType = NI::PickType::FIND_FIRST;
            }

            // Sort results by distance?
            if (getOptionalParam<bool>(params, "sort", true)) {
                rayTestCache->sortType = NI::PickSortType::SORT;
            }
            else {
                rayTestCache->sortType = NI::PickSortType::NO_SORT;
            }

            // Use triangle or model bounds for intersection?
            if (getOptionalParam<bool>(params, "useModelBounds", false)) {
                rayTestCache->intersectType = NI::PickIntersectType::BOUND_INTERSECT;
            }
            else {
                rayTestCache->intersectType = NI::PickIntersectType::TRIANGLE_INTERSECT;
            }

            // Use model coordinates or world coordinates?
            if (getOptionalParam<bool>(params, "useModelCoordinates", false)) {
                rayTestCache->coordinateType = NI::PickCoordinateType::MODEL_COORDINATES;
            }
            else {
                rayTestCache->coordinateType = NI::PickCoordinateType::WORLD_COORDINATES;
            }

            // Use the back side of a triangle? Note: Parameter name flipped!
            rayTestCache->frontOnly = !getOptionalParam<bool>(params, "useBackTriangles", false);

            // Observe app cull flag?
            rayTestCache->observeAppCullFlag = getOptionalParam<bool>(params, "observeAppCullFlag", true);

            // Determine what returned values we care about.
            rayTestCache->returnColor = getOptionalParam<bool>(params, "returnColor", false);
            rayTestCache->returnNormal = getOptionalParam<bool>(params, "returnNormal", true);
            rayTestCache->returnSmoothNormal = getOptionalParam<bool>(params, "returnSmoothNormal", false);
            rayTestCache->returnTexture = getOptionalParam<bool>(params, "returnTexture", false);

            // Default root nodes to ignore.
            std::vector<NI::AVObject*> ignoreRestoreList;
            if (rayTestIgnoreRoots.empty()) {
                auto weather = TES3::WorldController::get()->weatherController;
                auto world = TES3::Game::get()->worldRoot;
                rayTestIgnoreRoots.push_back(weather->sgRainRoot);
                rayTestIgnoreRoots.push_back(weather->sgSnowRoot);
                rayTestIgnoreRoots.push_back(weather->sgStormRoot);
                rayTestIgnoreRoots.push_back(world->getObjectByName("WorldProjectileRoot"));
                rayTestIgnoreRoots.push_back(world->getObjectByName("WorldSpellRoot"));
                rayTestIgnoreRoots.push_back(world->getObjectByName("WorldVFXRoot"));
            }
            for (const auto node : rayTestIgnoreRoots) {
                if (!node->getAppCulled()) {
                    node->setAppCulled(true);
                    ignoreRestoreList.push_back(node);
                }
            }

            // Allow defining references/nodes to ignore from the raytest.
            sol::optional<sol::table> ignoreTable = params["ignore"];
            if (ignoreTable) {
                for (const auto& kvPair : ignoreTable.value()) {
                    sol::object value = kvPair.second;
                    if (value.is<NI::Node>()) {
                        auto node = value.as<NI::Node*>();
                        if (!node->getAppCulled()) {
                            // Cull the node, and add it to a list to set as unculled later.
                            node->setAppCulled(true);
                            ignoreRestoreList.push_back(node);
                        }
                    }
                    else if (value.is<TES3::Reference>()) {
                        auto reference = value.as<TES3::Reference*>();
                        if (reference->sceneNode) {
                            if (!reference->sceneNode->getAppCulled()) {
                                // Cull the node, and add it to a list to set as unculled later.
                                reference->sceneNode->setAppCulled(true);
                                ignoreRestoreList.push_back(reference->sceneNode);
                            }
                        }
                    }
                    else {
                        // Restore previous cull states.
                        for (const auto node : ignoreRestoreList) {
                            node->setAppCulled(false);
                        }

                        throw std::exception("tes3.rayTest: Invalid item in ignore list. Must contain only scene graph nodes or references.");
                    }
                }
            }

            // Our pick is configured. Let's run it! (Use normalized direction for skinned mesh fix later.)
            rayTestCache->pickObjects(&position.value(), &direction.value().normalized(), false, maxDistance);

            // Restore previous cull states.
            for (auto itt = ignoreRestoreList.begin(); itt != ignoreRestoreList.end(); itt++) {
                (*itt)->setAppCulled(false);
            }

            // Did we get any results?
            if (rayTestCache->results.filledCount == 0) {
                return sol::nil;
            }

            // Fix distances and missing intersection data for skinned nodes.
            auto distanceScale = 1.0 / direction.value().length();
            auto skinnedCorrection = (maxDistance != 0.0) ? maxDistance : 1.0;

            for (int i = 0; i < rayTestCache->results.filledCount; i++) {
                // Adjust distance as if direction was not normalized.
                auto r = rayTestCache->results.storage[i];
                r->distance *= distanceScale;

                // Skinned nodes only have usable scaled distance data.
                if ((uintptr_t)r->object->getRunTimeTypeInformation() == NI::RTTIStaticPtr::NiTriShape) {
                    auto node = static_cast<const NI::TriShape*>(r->object);
                    if (node->skinInstance) {
                        r->distance *= skinnedCorrection;
                        r->intersection = position.value() + direction.value() * r->distance;
                        r->normal = (r->intersection - node->worldBoundOrigin).normalized();
                    }
                }
            }

            // Are we looking for a single result?
            if (rayTestCache->pickType == NI::PickType::FIND_FIRST) {
                return sol::make_object(state, rayTestCache->results.storage[0]);
            }

            // We're now in multi-result mode. We'll store these in a table.
            sol::table results = state.create_table();

            // Go through and clone the results in a way that will play nice.
            for (int i = 0; i < rayTestCache->results.filledCount; i++) {
                results[i + 1] = rayTestCache->results.storage[i];
            }

            return results;
        };

        // Bind function: tes3.tapKey
        state["tes3"]["tapKey"] = [](double key) {
            Stack::getInstance().pushLong(key);
            mwscript::RunOriginalOpCode(NULL, NULL, OpCode::MGETapKey);
        };

        // Bind function: tes3.pushKey
        state["tes3"]["pushKey"] = [](double key) {
            Stack::getInstance().pushLong(key);
            mwscript::RunOriginalOpCode(NULL, NULL, OpCode::MGEPushKey);
        };

        // Bind function: tes3.releaseKey
        state["tes3"]["releaseKey"] = [](double key) {
            Stack::getInstance().pushLong(key);
            mwscript::RunOriginalOpCode(NULL, NULL, OpCode::MGEReleaseKey);
        };

        // Bind function: tes3.hammerKey
        state["tes3"]["hammerKey"] = [](double key) {
            Stack::getInstance().pushLong(key);
            mwscript::RunOriginalOpCode(NULL, NULL, OpCode::MGEHammerKey);
        };

        // Bind function: tes3.unhammerKey
        state["tes3"]["unhammerKey"] = [](double key) {
            Stack::getInstance().pushLong(key);
            mwscript::RunOriginalOpCode(NULL, NULL, OpCode::MGEUnhammerKey);
        };

        // Bind function: tes3.enableKey
        state["tes3"]["enableKey"] = [](double key) {
            Stack::getInstance().pushLong(key);
            mwscript::RunOriginalOpCode(NULL, NULL, OpCode::MGEAllowKey);
        };

        // Bind function: tes3.disableKey
        state["tes3"]["disableKey"] = [](double key) {
            Stack::getInstance().pushLong(key);
            mwscript::RunOriginalOpCode(NULL, NULL, OpCode::MGEDisallowKey);
        };

        // TODO: tes3.getTopMenu is deprecated.
        state["tes3"]["getTopMenu"] = []() {
            return tes3::ui::getTopMenu();
        };

        state["tes3"]["getCumulativeDaysForMonth"] = [](int month) -> sol::optional<int> {
            TES3::WorldController * worldController = TES3::WorldController::get();
            if (worldController) {
                return worldController->getCumulativeDaysForMonth(month);
            }
            return sol::optional<int>();
        };

        state["tes3"]["getEquippedItem"] = [](sol::table params) -> TES3::EquipmentStack* {
            // Find our equipment based on the object given.
            TES3::Iterator<TES3::EquipmentStack> * equipment = NULL;
            sol::object actor = params["actor"];
            if (actor.valid()) {
                if (actor.is<TES3::Reference*>()) {
                    equipment = actor.as<TES3::Reference*>()->getEquipment();
                }
                else if (actor.is<TES3::MobileActor*>()) {
                    equipment = actor.as<TES3::MobileActor*>()->reference->getEquipment();
                }
                else if (actor.is<TES3::Actor*>()) {
                    equipment = &actor.as<TES3::Actor*>()->equipment;
                }
            }
            else {
                throw std::exception("No actor provided.");
            }

            // Make sure we got the equipment.
            if (equipment == nullptr) {
                throw std::exception("The provided actor's equipment could not be resolved.");
            }

            // Get filter: Item Type
            sol::optional<int> filterObjectType;
            sol::optional<int> filterSlot;
            if (params["objectType"].valid()) {
                filterObjectType = (int)params["objectType"];

                // Get filter: Item Slot/Type
                if (params["slot"].valid()) {
                    filterSlot = (int)params["slot"];
                }
                else if (params["type"].valid()) {
                    filterSlot = (int)params["type"];
                }
            }

            // Get filter: Item Enchanted
            sol::optional<bool> filterEnchanted;
            if (params["enchanted"].valid()) {
                filterEnchanted = (bool)params["enchanted"];
            }

            // Loop through the items and pick the first one that matches our filters.
            for (auto itt = equipment->head; itt != nullptr; itt = itt->next) {
                TES3::Object * object = itt->data->object;

                // Filter object type.
                if (filterObjectType && filterObjectType.value() != object->objectType) {
                    continue;
                }

                // Filter slot/type.
                if (filterSlot && filterSlot.value() != object->getType()) {
                    continue;
                }

                // Filter enchanted.
                if (filterEnchanted) {
                    TES3::Enchantment * enchantment = object->getEnchantment();
                    if (filterEnchanted.value() == true && enchantment == nullptr) {
                        continue;
                    }
                    else if (filterEnchanted.value() == false && enchantment != nullptr) {
                        continue;
                    }
                }

                // If we got this far we match all filters. Return the object.
                return itt->data;
            }

            return nullptr;
        };

        state["tes3"]["getInputBinding"] = [](int code) -> TES3::InputConfig* {
            if (code < TES3::KeyBind::FirstKey || code > TES3::KeyBind::LastKey) {
                return nullptr;
            }

            TES3::WorldController * worldController = TES3::WorldController::get();
            if (worldController) {
                TES3::InputController * inputController = worldController->inputController;
                if (inputController) {
                    return &inputController->inputMaps[code];
                }
            }
            return nullptr;
        };

        state["tes3"]["getRegion"] = []() -> sol::object {
            TES3::DataHandler * dataHandler = TES3::DataHandler::get();
            if (dataHandler) {
                // Try to get the current cell's region first.
                if (dataHandler->currentCell) {
                    TES3::Region * region = dataHandler->currentCell->getRegion();
                    if (region) {
                        return makeLuaObject(region);
                    }
                }

                // Otherwise fall back to the last exterior cell's region.
                if (dataHandler->lastExteriorCell) {
                    return makeLuaObject(dataHandler->lastExteriorCell->getRegion());
                }
            }

            return sol::nil;
        };

        state["tes3"]["getCurrentWeather"] = []() -> sol::object {
            TES3::WorldController * worldController = TES3::WorldController::get();
            if (worldController) {
                return makeLuaObject(worldController->weatherController->currentWeather);
            }

            return sol::nil;
        };

        state["tes3"]["getSkill"] = [](int skillID) -> sol::object {
            TES3::DataHandler * dataHandler = TES3::DataHandler::get();
            if (dataHandler) {
                return makeLuaObject(&dataHandler->nonDynamicData->skills[skillID]);
            }

            return sol::nil;
        };

        state["tes3"]["removeEffects"] = [](sol::table params) {
            auto& luaManager = mwse::lua::LuaManager::getInstance();
            auto stateHandle = luaManager.getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;

            TES3::Reference * reference = getOptionalParamExecutionReference(params);
            if (reference == nullptr) {
                throw std::exception("tes3.removeEffects: No reference parameter provided.");
            }

            int effect = getOptionalParam<int>(params, "effect", -1);
            int castType = getOptionalParam<int>(params, "castType", -1);
            int chance = getOptionalParam<int>(params, "chance", 100);
            if (chance > 100) {
                chance = 100;
            }
            else if (chance < 0) {
                chance = 0;
            }

            if (effect != -1) {
                TES3::WorldController::get()->spellInstanceController->removeSpellsByEffect(reference, effect, chance);
            }
            else if (castType != -1) {
                bool removeSpell = getOptionalParam<bool>(params, "removeSpell", castType != int(TES3::SpellCastType::Spell));
                TES3::WorldController::get()->spellInstanceController->clearSpellEffect(reference, castType, chance, removeSpell);
            }
            else {
                throw std::exception("tes3.removeEffects: Must pass either 'effect' or 'castType' parameter!");
            }
        };

        state["tes3"]["loadMesh"] = [](const char* relativePath) -> sol::object {
            std::string path = "Meshes\\";
            path += relativePath;

            return makeLuaNiPointer(TES3::DataHandler::get()->nonDynamicData->meshData->loadMesh(path.c_str()));
        };

        state["tes3"]["playVoiceover"] = [](sol::table params) -> bool {
            auto& luaManager = mwse::lua::LuaManager::getInstance();
            auto stateHandle = luaManager.getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;

            // Get the actor that we're going to make say something.
            auto actor = getOptionalParamMobileActor(params, "actor");
            if (actor == nullptr) {
                return false;
            }

            // Accept either a string or a number as the voiceover id.
            int voiceover = -1;
            sol::object voiceoverObject = params["voiceover"];
            if (voiceoverObject.is<const char*>()) {
                sol::object result = state["tes3"]["voiceover"][voiceoverObject.as<const char*>()];
                if (result.is<int>()) {
                    voiceover = result.as<int>();
                }
            }
            else if (voiceoverObject.is<int>()) {
                voiceover = voiceoverObject.as<int>();
            }

            // Validate the input.
            if (voiceover < TES3::Voiceover::First || voiceover > TES3::Voiceover::Last) {
                return false;
            }

            actor->playVoiceover(voiceover);
            return true;
        };

        state["tes3"]["getFileExists"] = [](const char* path) {
            return tes3::resolveAssetPath(path) != 0;
        };

        state["tes3"]["getFileSource"] = [](const char* path) -> sol::optional<std::tuple<std::string, std::string>> {
            char buffer[512];
            int result = tes3::resolveAssetPath(path, buffer);

            if (result == 1) {
                return std::make_tuple("file", buffer);
            }
            else if (result == 2) {
                return std::make_tuple("bsa", buffer);
            }

            return sol::optional<std::tuple<std::string, std::string>>();
        };

        // Very slow method to get an INFO record by its ID.
        state["tes3"]["getDialogueInfo"] = [](sol::table params) -> sol::object {
            TES3::Dialogue * dialogue = getOptionalParamDialogue(params, "dialogue");
            const char * id = getOptionalParam<const char*>(params, "id", nullptr);
            if (dialogue == nullptr || id == nullptr) {
                return sol::nil;
            }

            for (auto itt = dialogue->info.head; itt; itt = itt->next) {
                auto dialogueInfo = itt->data;
                if (!dialogueInfo->loadId()) {
                    continue;
                }

                if (_strcmpi(id, dialogueInfo->loadLinkNode->name) == 0) {
                    dialogueInfo->unloadId();
                    return makeLuaObject(dialogueInfo);
                }

                dialogueInfo->unloadId();
            }

            return sol::nil;
        };

        // Very slow method to get an INFO record by its ID.
        state["tes3"]["getCell"] = [](sol::table params) -> sol::object {
            // If we were given a name, try that.
            sol::optional<const char*> cellId = params["id"];
            if (cellId) {
                return makeLuaObject(TES3::DataHandler::get()->nonDynamicData->getCellByName(cellId.value()));
            }

            // Otherwise try to use X/Y.
            return makeLuaObject(TES3::DataHandler::get()->nonDynamicData->getCellByGrid(params["x"], params["y"]));
        };

        state["tes3"]["setStatistic"] = [](sol::table params) {
            auto& luaManager = mwse::lua::LuaManager::getInstance();
            auto stateHandle = luaManager.getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;

            // Figure out our mobile object, in case someone gives us a reference instead.
            sol::userdata maybeMobile = params["reference"];
            if (maybeMobile.is<TES3::Reference>()) {
                maybeMobile = maybeMobile["mobile"];
            }

            // Make sure our object is of the right type.
            if (!maybeMobile.is<TES3::MobileActor>()) {
                logStackTrace("tes3.setStatistic: Could not resolve parameter 'reference'.");
                return;
            }

            // Try to get our statistic.
            TES3::MobileActor * mobile = maybeMobile.as<TES3::MobileActor*>();
            TES3::Statistic * statistic = nullptr;
            sol::optional<const char*> statisticName = params["name"];
            sol::optional<int> statisticSkill = params["skill"];
            sol::optional<int> statisticAttribute = params["attribute"];
            if (statisticSkill) {
                if (mobile->actorType == TES3::MobileActorType::Creature) {
                    if (statisticSkill.value() >= TES3::CreatureSkillID::FirstSkill && statisticSkill.value() <= TES3::CreatureSkillID::LastSkill) {
                        statistic = &static_cast<TES3::MobileCreature*>(mobile)->skills[statisticSkill.value()];
                    }
                    else {
                        mwse::log::getLog() << "tes3.setStatistic: Invalid skill index " << std::dec << statisticSkill.value() << " for creature." << std::endl;
                        logStackTrace();
                        return;
                    }
                }
                else {
                    if (statisticSkill.value() >= TES3::SkillID::FirstSkill && statisticSkill.value() <= TES3::SkillID::LastSkill) {
                        statistic = &static_cast<TES3::MobileNPC*>(mobile)->skills[statisticSkill.value()];
                    }
                    else {
                        mwse::log::getLog() << "tes3.setStatistic: Invalid skill index " << std::dec << statisticSkill.value() << " for NPC." << std::endl;
                        logStackTrace();
                        return;
                    }
                }
            }
            else if (statisticAttribute) {
                if (statisticAttribute.value() >= TES3::Attribute::FirstAttribute && statisticAttribute.value() <= TES3::Attribute::LastAttribute) {
                    statistic = &mobile->attributes[statisticAttribute.value()];
                }
                else {
                    mwse::log::getLog() << "tes3.setStatistic: Invalid attribute index " << std::dec << statisticSkill.value() << "." << std::endl;
                    logStackTrace();
                    return;
                }
            }
            else if (statisticName) {
                sol::optional<TES3::Statistic*> maybeStatistic = maybeMobile[statisticName.value()];
                if (maybeStatistic) {
                    statistic = maybeStatistic.value();
                }
                else {
                    logStackTrace("tes3.setStatistic: No statistic with the given criteria could be found.");
                    return;
                }
            }

            // This case shouldn't be hit.
            if (statistic == nullptr) {
                logStackTrace("tes3.setStatistic: No statistic resolved.");
                return;
            }

            // Retype our variables to something more friendly, and get additional params.
            sol::optional<bool> limit = params["limit"];

            sol::optional<float> current = params["current"];
            sol::optional<float> base = params["base"];
            sol::optional<float> value = params["value"];

            // Edit both.
            if (value) {
                statistic->setBaseAndCurrent(value.value());
            }
            // If we're given a current value, modify it.
            else if (current) {
                statistic->setCurrentCapped(current.value(), limit.value_or(false));
            }
            // If we're given a base value, modify it.
            else if (base) {
                statistic->setBase(base.value());
            }
            else {
                logStackTrace("tes3.setStatistic: No edit mode provided, missing parameter 'current' or 'base' or 'value'.");
                return;
            }

            // Update any derived statistics.
            mobile->updateDerivedStatistics(statistic);

            // Ensure the reference is flagged as modified.
            if (mobile->reference) {
                mobile->reference->setObjectModified(true);
            }

            // If this was on the player update any associated GUI widgets.
            if (mobile->actorType == TES3::MobileActorType::Player) {
                // Manually update health/magicka/fatigue UI elements.
                if (statistic == &mobile->health) {
                    TES3::UI::updateHealthFillBar(statistic->current, statistic->base);
                }
                else if (statistic == &mobile->magicka) {
                    TES3::UI::updateMagickaFillBar(statistic->current, statistic->base);
                }
                else if (statistic == &mobile->fatigue) {
                    TES3::UI::updateFatigueFillBar(statistic->current, statistic->base);
                }
                else {
                    // Check to see if an attribute was edited.
                    for (size_t i = TES3::Attribute::FirstAttribute; i <= TES3::Attribute::LastAttribute; i++) {
                        if (statistic == &mobile->attributes[i]) {
                            TES3::UI::updateEncumbranceBar();
                            TES3::UI::updatePlayerAttribute(statistic->getCurrent(), i);
                            break;
                        }
                    }
                }

                TES3::UI::updateStatsPane();
            }
        };

        state["tes3"]["modStatistic"] = [](sol::table params) {
            auto& luaManager = mwse::lua::LuaManager::getInstance();
            auto stateHandle = luaManager.getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;

            // Figure out our mobile object, in case someone gives us a reference instead.
            sol::userdata maybeMobile = params["reference"];
            if (maybeMobile.is<TES3::Reference>()) {
                maybeMobile = maybeMobile["mobile"];
            }

            // Make sure our object is of the right type.
            if (!maybeMobile.is<TES3::MobileActor>()) {
                logStackTrace("tes3.modStatistic: Could not resolve parameter 'reference'.");
                return;
            }

            // Try to get our statistic.
            TES3::MobileActor * mobile = maybeMobile.as<TES3::MobileActor*>();
            TES3::Statistic * statistic = nullptr;
            sol::optional<const char*> statisticName = params["name"];
            sol::optional<int> statisticSkill = params["skill"];
            sol::optional<int> statisticAttribute = params["attribute"];
            if (statisticSkill) {
                if (mobile->actorType == TES3::MobileActorType::Creature) {
                    if (statisticSkill.value() >= TES3::CreatureSkillID::FirstSkill && statisticSkill.value() <= TES3::CreatureSkillID::LastSkill) {
                        statistic = &static_cast<TES3::MobileCreature*>(mobile)->skills[statisticSkill.value()];
                    }
                    else {
                        mwse::log::getLog() << "tes3.modStatistic: Invalid skill index " << std::dec << statisticSkill.value() << " for creature." << std::endl;
                        logStackTrace();
                        return;
                    }
                }
                else {
                    if (statisticSkill.value() >= TES3::SkillID::FirstSkill && statisticSkill.value() <= TES3::SkillID::LastSkill) {
                        statistic = &static_cast<TES3::MobileNPC*>(mobile)->skills[statisticSkill.value()];
                    }
                    else {
                        mwse::log::getLog() << "tes3.modStatistic: Invalid skill index " << std::dec << statisticSkill.value() << " for NPC." << std::endl;
                        logStackTrace();
                        return;
                    }
                }
            }
            else if (statisticAttribute) {
                if (statisticAttribute.value() >= TES3::Attribute::FirstAttribute && statisticAttribute.value() <= TES3::Attribute::LastAttribute) {
                    statistic = &mobile->attributes[statisticAttribute.value()];
                }
                else {
                    mwse::log::getLog() << "tes3.modStatistic: Invalid attribute index " << std::dec << statisticSkill.value() << "." << std::endl;
                    logStackTrace();
                    return;
                }
            }
            else if (statisticName) {
                sol::optional<TES3::Statistic*> maybeStatistic = maybeMobile[statisticName.value()];
                if (maybeStatistic) {
                    statistic = maybeStatistic.value();
                }
                else {
                    logStackTrace("tes3.modStatistic: No statistic with the given criteria could be found.");
                    return;
                }
            }

            // This case shouldn't be hit.
            if (statistic == nullptr) {
                logStackTrace("tes3.modStatistic: No statistic resolved.");
                return;
            }

            // Retype our variables to something more friendly, and get additional params.
            sol::optional<bool> limit = params["limit"];

            sol::optional<float> current = params["current"];
            sol::optional<float> base = params["base"];
            sol::optional<float> value = params["value"];

            // Edit both.
            if (value) {
                statistic->modBaseCapped(value.value(), limit.value_or(false), limit.value_or(false));
                statistic->modCurrentCapped(value.value(), limit.value_or(false), limit.value_or(false), limit.value_or(false));
            }
            // If we're given a current value, modify it.
            else if (current) {
                statistic->modCurrentCapped(current.value(), limit.value_or(false), limit.value_or(false), limit.value_or(false));
            }
            // If we're given a base value, modify it.
            else if (base) {
                statistic->modBaseCapped(base.value(), limit.value_or(false), limit.value_or(false));
            }
            else {
                logStackTrace("tes3.modStatistic: No edit mode provided, missing parameter 'current' or 'base' or 'value'.");
                return;
            }

            // Update any derived statistics.
            mobile->updateDerivedStatistics(statistic);

            // Ensure the reference is flagged as modified.
            if (mobile->reference) {
                mobile->reference->setObjectModified(true);
            }

            // If this was on the player update any associated GUI widgets.
            if (mobile->actorType == TES3::MobileActorType::Player) {
                // Manually update health/magicka/fatigue UI elements.
                if (statistic == &mobile->health) {
                    TES3::UI::updateHealthFillBar(statistic->current, statistic->base);
                }
                else if (statistic == &mobile->magicka) {
                    TES3::UI::updateMagickaFillBar(statistic->current, statistic->base);
                }
                else if (statistic == &mobile->fatigue) {
                    TES3::UI::updateFatigueFillBar(statistic->current, statistic->base);
                }
                else {
                    // Check to see if an attribute was edited.
                    for (size_t i = TES3::Attribute::FirstAttribute; i <= TES3::Attribute::LastAttribute; i++) {
                        if (statistic == &mobile->attributes[i]) {
                            TES3::UI::updateEncumbranceBar();
                            TES3::UI::updatePlayerAttribute(statistic->getCurrent(), i);
                            break;
                        }
                    }
                }

                TES3::UI::updateStatsPane();
            }
        };

        state["tes3"]["runLegacyScript"] = [](sol::table params) {
            TES3::Script * script = getOptionalParamScript(params, "script");
            if (script == nullptr) {
                script = TES3::WorldController::get()->scriptGlobals;
            }

            TES3::ScriptCompiler * compiler = TES3::WorldController::get()->menuController->scriptCompiler;
            int source = getOptionalParam<int>(params, "source", TES3::CompilerSource::Default);
            if (source < TES3::CompilerSource::FirstSource || source > TES3::CompilerSource::LastSource) {
                return false;
            }

            const char* command = getOptionalParam<const char*>(params, "command", nullptr);
            if (command == nullptr) {
                return false;
            }

            TES3::ScriptVariables * variables = getOptionalParam<TES3::ScriptVariables*>(params, "variables", nullptr);

            TES3::Reference * reference = getOptionalParamExecutionReference(params);
            if (reference && variables == nullptr) {
                variables = reference->getScriptVariables();
            }

            TES3::Dialogue * dialogue = getOptionalParamDialogue(params, "dialogue");
            TES3::DialogueInfo * dialogueInfo = getOptionalParam< TES3::DialogueInfo*>(params, "info", nullptr);
            if (dialogue == nullptr || dialogueInfo == nullptr) {
                dialogue = nullptr;
                dialogueInfo = nullptr;
                source = TES3::CompilerSource::Default;
            }
            else {
                source = TES3::CompilerSource::Dialogue;
            }

            script->doCommand(compiler, command, source, reference, variables, dialogueInfo, dialogue);
            return true;
        };

        state["tes3"]["getActiveCells"] = []() -> sol::object {
            auto dataHandler = TES3::DataHandler::get();
            if (dataHandler == nullptr) {
                return sol::nil;
            }

            auto& luaManager = mwse::lua::LuaManager::getInstance();
            auto stateHandle = luaManager.getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;
            sol::table result = state.create_table();

            if (dataHandler->currentInteriorCell) {
                result[1] = makeLuaObject(dataHandler->currentInteriorCell);
            }
            else {
                int exteriorCount = 0;
                for (size_t i = 0; i < 9; i++) {
                    auto cellDataPointer = dataHandler->exteriorCellData[i];
                    if (cellDataPointer && cellDataPointer->loadingFlags >= 1) {
                        exteriorCount++;
                        result[exteriorCount] = makeLuaObject(cellDataPointer->cell);
                    }
                }
            }

            return result;
        };

        state["tes3"]["positionCell"] = [](sol::table params) {
            auto worldController = TES3::WorldController::get();
            auto macp = worldController->getMobilePlayer();

            // Get the target that we're working with.
            TES3::MobileActor * mobile = getOptionalParamMobileActor(params, "reference");
            if (mobile == nullptr) {
                mobile = macp;
            }

            // Get the position.
            sol::optional<TES3::Vector3> position = getOptionalParamVector3(params, "position");
            if (!position) {
                return false;
            }

            // Get the orientation.
            sol::optional<TES3::Vector3> orientation = getOptionalParamVector3(params, "orientation");
            if (!orientation) {
                orientation = mobile->reference->orientation;
            }

            // Get the cell.
            TES3::Cell * cell = getOptionalParamCell(params, "cell");
            if (cell == nullptr) {
                // Try to find an exterior cell from the position.
                int gridX = TES3::Cell::toGridCoord(position.value().x);
                int gridY = TES3::Cell::toGridCoord(position.value().y);
                cell = TES3::DataHandler::get()->nonDynamicData->getCellByGrid(gridX, gridY);

                if (cell == nullptr) {
                    return false;
                }
            }

            // Are we dealing with the player? If so, use the special functions.
            if (mobile == macp) {
                sol::optional<bool> suppressFaderOpt = params["suppressFader"];
                bool suppressFader = suppressFaderOpt.value_or(false);
                bool faderInitialState = TES3::DataHandler::get()->useCellTransitionFader;

                if (suppressFader) {
                    TES3::DataHandler::get()->useCellTransitionFader = false;
                }

                sol::optional<bool> teleportCompanions = params["teleportCompanions"];
                if (teleportCompanions.value_or(true) && macp->listFriendlyActors.size > 0) {
                    const auto TES3_cellChangeWithCompanions = reinterpret_cast<void(__cdecl*)(TES3::Vector3, TES3::Vector3, TES3::Cell*)>(0x45C9B0);
                    TES3_cellChangeWithCompanions(position.value(), orientation.value(), cell);
                }
                else {
                    const auto TES3_cellChange = reinterpret_cast<void(__cdecl*)(TES3::Vector3, TES3::Vector3, TES3::Cell*, int)>(0x45CEF0);
                    sol::optional<bool> flag = params["flag"];
                    TES3_cellChange(position.value(), orientation.value(), cell, flag.value_or(true));
                }

                if (suppressFader) {
                    TES3::DataHandler::get()->useCellTransitionFader = faderInitialState;
                }
            }
            else {
                const auto TES3_relocateReference = reinterpret_cast<void(__cdecl*)(TES3::Reference*, TES3::Cell*, TES3::Vector3*, float)>(0x50EDD0);
                TES3_relocateReference(mobile->reference, cell, &position.value(), orientation.value().z);
            }

            // Ensure the reference is flagged as modified.
            if (mobile->reference) {
                mobile->reference->setObjectModified(true);
            }

            return true;
        };

        state["tes3"]["addSoulGem"] = [](sol::table params) {
            TES3::Misc * item = getOptionalParamObject<TES3::Misc>(params, "item");
            if (item == nullptr) {
                return false;
            }

            auto data = mwse::tes3::addCustomSoulGem(item);
            if (data == nullptr) {
                return false;
            }

            return true;
        };

        state["tes3"]["addArmorSlot"] = [](sol::table params) {
            sol::optional<int> slot = params["slot"];
            if (!slot || (slot.value() >= TES3::ArmorSlot::First && slot.value() <= TES3::ArmorSlot::Last) || mwse::tes3::getArmorSlotData(slot.value())) {
                throw std::exception("tes3.addArmorSlot: Invalid slot. An unusued slot must be provided.");
            }

            sol::optional<const char*> name = params["name"];
            if (!name || name.value() == nullptr) {
                throw std::exception("tes3.addArmorSlot: No name provided for slot.");
            }

            sol::optional<float> weight = params["weight"];

            auto slotData = new TES3::ArmorSlotData();
            slotData->slot = slot.value();
            slotData->name = name.value();
            slotData->weight = weight.value_or(0.0f);

            mwse::tes3::setArmorSlotData(slotData);
        };

        state["tes3"]["createCell"] = [](sol::table params) -> sol::object {
            auto nonDynamicData = TES3::DataHandler::get()->nonDynamicData;

            TES3::Cell * cell = nullptr;

            sol::optional<int> gridX = params["gridX"];
            sol::optional<int> gridY = params["gridY"];
            if (gridX && gridY) {
                auto existingCell = nonDynamicData->getCellByGrid(gridX.value(), gridY.value());
                if (existingCell) {
                    mwse::log::getLog() << "Could not create cell at coordinates <" << gridX.value() << ", " << gridY.value() << ">. Cell already exists at that location." << std::endl;
                    return sol::nil;
                }

                cell = TES3::Cell::create();
                cell->setCellFlag(TES3::CellFlag::Interior, false);
                cell->setGridX(gridX.value());
                cell->setGridY(gridY.value());
                cell->setName("");
            }
            else {
                sol::optional<const char*> name = params["name"];
                if (!name) {
                    mwse::log::getLog() << "Could not create cell. Interior cells must have a name." << std::endl;
                    return sol::nil;
                }

                auto existingCell = nonDynamicData->getCellByName(name.value());
                if (existingCell) {
                    mwse::log::getLog() << "Could not create cell \"" << name.value() << "\". Cell already exists with the given name." << std::endl;
                    return sol::nil;
                }

                cell = TES3::Cell::create();
                cell->setCellFlag(TES3::CellFlag::Interior, true);
                cell->setName(name.value());
            }

            cell->setObjectModified(true);

            nonDynamicData->cells->insertAtFront(cell);

            return makeLuaObject(cell);
        };

        state["tes3"]["createReference"] = [](sol::table params) -> sol::object {
            auto dataHandler = TES3::DataHandler::get();

            // Get the object we are going to create a reference for.
            auto object = getOptionalParamObject<TES3::PhysicalObject>(params, "object");
            if (object == nullptr) {
                throw std::invalid_argument("Invalid 'object' parameter provided.");
            }

            // Get the position.
            auto maybePosition = getOptionalParamVector3(params, "position");
            if (!maybePosition) {
                throw std::invalid_argument("Invalid 'position' parameter provided.");
            }

            // Get the orientation.
            TES3::Vector3 orientation(0.0f, 0.0f, 0.0f);
            auto maybeOrientation = getOptionalParamVector3(params, "orientation");
            if (maybeOrientation) {
                orientation = maybeOrientation.value();
            }

            // Try to resolve the sell, either by what we were given, or a valid cell based on the given position.
            TES3::Cell * cell = getOptionalParamCell(params, "cell");
            if (cell == nullptr || (!cell->isInterior() && !cell->isPointInCell(maybePosition.value().x, maybePosition.value().y))) {
                int cellX = TES3::Cell::toGridCoord(maybePosition.value().x);
                int cellY = TES3::Cell::toGridCoord(maybePosition.value().y);
                cell = dataHandler->nonDynamicData->getCellByGrid(cellX, cellY);
            }

            // Make sure we actually got a cell.
            if (cell == nullptr) {
                throw std::invalid_argument("Invalid 'cell' parameter provided. Please provide a cell, or give a valid exterior position.");
            }

            // Create reference.
            TES3::Reference * reference = new TES3::Reference();
            reference->baseObject = object;

            // Scale as needed.
            float scale = getOptionalParam<float>(params, "scale", 1.0f);
            if (scale != 1.0f) {
                reference->setScale(scale);
            }

            // Add it to the cell lists/data handler.
            bool cellWasCreated = false;
            dataHandler->nonDynamicData->createReference(object, &maybePosition.value(), &orientation, cellWasCreated, reference, cell);
            reference->ensureScriptDataIsInstanced();
            cell->insertReference(reference);

            // Did we just make an actor? If so we need to add it to the mob manager.
            if (object->objectType == TES3::ObjectType::Creature || object->objectType == TES3::ObjectType::NPC) {
                TES3::WorldController::get()->mobController->addMob(reference);
                auto mact = reference->getAttachedMobileActor();
                if (mact && mact->isActor()) {
                    mact->enterLeaveSimulation(true);
                }
            }
            // Lights need to be configured.
            else if (object->objectType == TES3::ObjectType::Light) {
                dataHandler->updateLightingForReference(reference);
                dataHandler->setDynamicLightingForReference(reference);

                // They also need collision.
                dataHandler->updateCollisionGroupsForActiveCells();
            }
            // For all other things, just reset collision.
            else {
                dataHandler->updateCollisionGroupsForActiveCells();
            }

            // Make sure everything is set as modified.
            reference->setObjectModified(true);
            cell->setObjectModified(true);

            return makeLuaObject(reference);
        };

        state["tes3"]["setDestination"] = [](sol::table params) {
            TES3::Reference * reference = getOptionalParamExecutionReference(params);
            if (reference == nullptr) {
                throw std::invalid_argument("Invalid reference parameter provided.");
            }

            if (reference->baseObject->objectType != TES3::ObjectType::Door) {
                throw std::invalid_argument("Provided reference is not a door.");
            }

            // Get the position.
            sol::optional<TES3::Vector3> position = getOptionalParamVector3(params, "position");
            if (!position) {
                throw std::invalid_argument("Invalid position parameter provided.");
            }

            // Get the orientation.
            sol::optional<TES3::Vector3> orientation = getOptionalParamVector3(params, "orientation");
            if (!orientation) {
                throw std::invalid_argument("Invalid orientation parameter provided.");
            }

            // Get the cell.
            TES3::Cell * cell = getOptionalParamCell(params, "cell");

            reference->setTravelDestination(&position.value(), &orientation.value(), cell);
            reference->setObjectModified(true);
            return true;
        };

        state["tes3"]["addItem"] = [](sol::table params) -> int {
            // Get the reference we are manipulating.
            TES3::Reference * reference = getOptionalParamReference(params, "reference");
            if (reference == nullptr) {
                throw std::invalid_argument("Invalid 'reference' parameter provided.");
            }

            // Get the item we are going to add.
            TES3::Item * item = getOptionalParamObject<TES3::Item>(params, "item");
            if (item == nullptr) {
                throw std::invalid_argument("Invalid 'item' parameter provided.");
            }

            TES3::ItemData * itemData = getOptionalParam<TES3::ItemData*>(params, "itemData", nullptr);

            // Make sure we're dealing with actors.
            TES3::Actor * actor = static_cast<TES3::Actor*>(reference->baseObject);
            if (!actor->isActor()) {
                throw std::invalid_argument("The 'reference' reference does not point to an actor.");
            }

            // Clone the object if needed.
            if (reference->clone()) {
                actor = static_cast<TES3::Actor*>(reference->baseObject);
            }

            // Get how many items we are adding.
            int fulfilledCount = 0;
            int desiredCount = std::max(std::abs(getOptionalParam(params, "count", 1)), 1);
            if (itemData != nullptr) {
                desiredCount = 1;
            }

            if (getOptionalParam<bool>(params, "limit", false)) {
                // Prevent placing items into organic containers.
                if (actor->actorFlags.test(TES3::ActorFlagContainer::OrganicBit)) {
                    return 0;
                }

                // Figure out how many more of the item can fit in the container.
                auto maxCapacity = static_cast<TES3::Container*>(reference->getBaseObject())->capacity;
                auto currentWeight = actor->inventory.calculateContainedWeight();
                int fulfilledCount = std::min((int)((maxCapacity - currentWeight) / item->getWeight()), desiredCount);
            }
            else {
                fulfilledCount = desiredCount;
            }

            // No items to add? Great, let's get out of here.
            if (fulfilledCount == 0) {
                return 0;
            }

            if (itemData) {
                // Clear the owner, if any.
                itemData->owner = nullptr;

                // Delete the item data if it's fully repaired.
                if (TES3::ItemData::isFullyRepaired(itemData, item)) {
                    delete itemData;
                    itemData = nullptr;
                }
            }

            // Add the item and return the added count, since we do no inventory checking.
            auto mobile = reference->getAttachedMobileActor();
            actor->inventory.addItem(mobile, item, fulfilledCount, false, &itemData);

            // Play the relevant sound.
            auto worldController = TES3::WorldController::get();
            auto playerMobile = worldController->getMobilePlayer();
            if (getOptionalParam<bool>(params, "playSound", true)) {
                if (mobile == playerMobile) {
                    worldController->playItemUpDownSound(item, true);
                }
            }

            // Update body parts for creatures/NPCs that may have items unequipped.
            if (mobile) {
                reference->updateBipedParts();

                if (mobile == playerMobile) {
                    playerMobile->firstPersonReference->updateBipedParts();
                }
            }

            // If either of them are the player, we need to update the GUI.
            if (getOptionalParam<bool>(params, "updateGUI", true)) {
                // Update inventory menu if necessary.
                if (mobile == playerMobile) {
                    worldController->inventoryData->clearIcons(2);
                    worldController->inventoryData->addInventoryItems(&playerMobile->npcInstance->inventory, 2);
                    mwse::tes3::ui::inventoryUpdateIcons();
                }

                // Update contents menu if necessary.
                auto contentsMenu = TES3::UI::findMenu(*reinterpret_cast<TES3::UI::UI_ID*>(0x7D3098));
                if (contentsMenu) {
                    TES3::Reference * contentsReference = static_cast<TES3::Reference*>(contentsMenu->getProperty(TES3::UI::PropertyType::Pointer, *reinterpret_cast<TES3::UI::Property*>(0x7D3048)).ptrValue);
                    if (reference == contentsReference) {
                        TES3::UI::updateContentsMenuTiles();
                    }
                }
            }

            reference->setObjectModified(true);
            return fulfilledCount;
        };

        state["tes3"]["removeItem"] = [](sol::table params) -> int {
            // Get the reference we are manipulating.
            TES3::Reference * reference = getOptionalParamReference(params, "reference");
            if (reference == nullptr) {
                throw std::invalid_argument("Invalid 'reference' parameter provided.");
            }

            // Get the item we are going to remove.
            TES3::Item * item = getOptionalParamObject<TES3::Item>(params, "item");
            if (item == nullptr) {
                throw std::invalid_argument("Invalid 'item' parameter provided.");
            }

            TES3::ItemData * itemData = getOptionalParam<TES3::ItemData*>(params, "itemData", nullptr);
            auto deleteItemData = getOptionalParam<bool>(params, "deleteItemData", itemData != nullptr);

            // Make sure we're dealing with actors.
            TES3::Actor * actor = static_cast<TES3::Actor*>(reference->baseObject);
            if (!actor->isActor()) {
                throw std::invalid_argument("The 'reference' reference does not point to an actor.");
            }

            // Clone the object if needed.
            if (reference->clone()) {
                actor = static_cast<TES3::Actor*>(reference->baseObject);
            }

            // Get how many items we are removing. Force to 1 if we supply an itemData.
            int desiredCount = std::max(std::abs(getOptionalParam(params, "count", 1)), 1);
            if (itemData != nullptr) {
                desiredCount = 1;
            }

            // Make sure that the inventory contains the item.
            TES3::ItemStack * stack = actor->inventory.findItemStack(item);
            if (stack == nullptr) {
                return 0;
            }

            // If we were given an itemData, make sure that it's here.
            if (itemData != nullptr && stack->variables != nullptr && !stack->variables->contains(itemData)) {
                return 0;
            }

            // Limit removal by stack count.
            int fulfilledCount = std::min(desiredCount, stack->count);

            // No items to remove? Great, let's get out of here.
            if (fulfilledCount == 0) {
                return 0;
            }

            // Try to unequip the item if it's equipped.
            auto mobile = reference->getAttachedMobileActor();
            if (itemData != nullptr) {
                actor->unequipItem(item, true, mobile, false, itemData);
            }

            // Add the item and return the added count, since we do no inventory checking.
            actor->inventory.removeItemWithData(mobile, item, itemData, fulfilledCount, deleteItemData);

            // Play the relevant sound.
            auto worldController = TES3::WorldController::get();
            auto playerMobile = worldController->getMobilePlayer();
            if (getOptionalParam<bool>(params, "playSound", true)) {
                if (mobile == playerMobile) {
                    worldController->playItemUpDownSound(item, false);
                }
            }

            // Update body parts for creatures/NPCs that may have items unequipped.
            if (mobile) {
                reference->updateBipedParts();

                if (mobile == playerMobile) {
                    playerMobile->firstPersonReference->updateBipedParts();
                }
            }

            // If either of them are the player, we need to update the GUI.
            if (getOptionalParam<bool>(params, "updateGUI", true)) {
                // Update inventory menu if necessary.
                if (mobile == playerMobile) {
                    worldController->inventoryData->clearIcons(2);
                    worldController->inventoryData->addInventoryItems(&playerMobile->npcInstance->inventory, 2);
                    mwse::tes3::ui::inventoryUpdateIcons();
                }

                // Update contents menu if necessary.
                auto contentsMenu = TES3::UI::findMenu(*reinterpret_cast<TES3::UI::UI_ID*>(0x7D3098));
                if (contentsMenu) {
                    TES3::Reference * contentsReference = static_cast<TES3::Reference*>(contentsMenu->getProperty(TES3::UI::PropertyType::Pointer, *reinterpret_cast<TES3::UI::Property*>(0x7D3048)).ptrValue);
                    if (reference == contentsReference) {
                        TES3::UI::updateContentsMenuTiles();
                    }
                }
            }

            reference->setObjectModified(true);
            return fulfilledCount;
        };

        state["tes3"]["transferItem"] = [](sol::table params) -> int {
            // Get the reference we are transferring from.
            TES3::Reference * fromReference = getOptionalParamReference(params, "from");
            if (fromReference == nullptr) {
                throw std::invalid_argument("Invalid 'from' parameter provided.");
            }

            // Get the reference we are transferring to.
            TES3::Reference * toReference = getOptionalParamReference(params, "to");
            if (toReference == nullptr) {
                throw std::invalid_argument("Invalid 'to' parameter provided.");
            }

            // Get the item we are going to transfer.
            TES3::Item * item = getOptionalParamObject<TES3::Item>(params, "item");
            if (item == nullptr) {
                throw std::invalid_argument("Invalid 'item' parameter provided.");
            }

            // Make sure we're dealing with actors.
            TES3::Actor * fromActor = static_cast<TES3::Actor*>(fromReference->baseObject);
            if (!fromActor->isActor()) {
                throw std::invalid_argument("The 'from' reference does not point to an actor.");
            }
            TES3::Actor * toActor = static_cast<TES3::Actor*>(toReference->baseObject);
            if (!toActor->isActor()) {
                throw std::invalid_argument("The 'to' reference does not point to an actor.");
            }

            // Do either of the references need to be cloned?
            if (fromReference->clone()) {
                fromActor = static_cast<TES3::Actor*>(fromReference->baseObject);
            }
            if (toReference->clone()) {
                toActor = static_cast<TES3::Actor*>(toReference->baseObject);
            }

            // Get any associated item data.
            TES3::ItemData * itemData = getOptionalParam<TES3::ItemData*>(params, "itemData", nullptr);

            // Get how many items we are transferring.
            int desiredCount = std::max(std::abs(getOptionalParam(params, "count", 1)), 1);
            int fulfilledCount = 0;

            // Get the mobile objects for the references, if applicable.
            auto toMobile = toReference->getAttachedMobileActor();
            auto fromMobile = fromReference->getAttachedMobileActor();

            // Are we looking at a non-container?
            auto fromIsContainer = (fromActor->objectType == TES3::ObjectType::Container);

            // Manage anything we need to regarding containers.
            float maxCapacity = -1.0f;
            float currentWeight = 0.0f;
            float itemWeight = item->getWeight();
            if (toActor->objectType == TES3::ObjectType::Container && getOptionalParam<bool>(params, "limitCapacity", true)) {
                // Prevent placing items into organic containers.
                if (toActor->actorFlags.test(TES3::ActorFlagContainer::OrganicBit)) {
                    return 0;
                }

                // Figure out the max capacity and currently stored weight of the container.
                maxCapacity = static_cast<TES3::Container*>(toReference->getBaseObject())->capacity;
                currentWeight = toActor->inventory.calculateContainedWeight();
                if (currentWeight > maxCapacity) {
                    return 0;
                }
            }

            // Were we given an ItemData? If so, we only need to transfer one item.
            if (itemData) {
                if ((maxCapacity == -1.0f || currentWeight + itemWeight <= maxCapacity) && fromActor->inventory.containsItem(item, itemData)) {
                    toActor->inventory.addItem(toMobile, item, 1, false, &itemData);
                    fromActor->inventory.removeItemWithData(fromMobile, item, itemData, 1, false);

                    if (!fromIsContainer) {
                        fromActor->unequipItem(item, true, fromMobile, false, itemData);
                    }

                    fulfilledCount = 1;
                    currentWeight += itemWeight;
                }
            }
            // No ItemData? We have to go through and transfer items over one by one.
            else {
                TES3::ItemStack * fromStack = fromActor->inventory.findItemStack(item);
                if (fromStack) {
                    int stackCount = std::abs(fromStack->count);
                    int itemsLeftToTransfer = std::min(desiredCount, stackCount);

                    // If we're limited by capacity, find out how many items we really want to transfer.
                    if (maxCapacity != -1.0f) {
                        itemsLeftToTransfer = std::min(itemsLeftToTransfer, (int)std::floorf((maxCapacity - currentWeight) / itemWeight));
                    }

                    if (itemsLeftToTransfer <= 0) {
                        return 0;
                    }

                    // Remove transfer items without data first.
                    int countWithoutVariables = stackCount - (fromStack->variables ? fromStack->variables->endIndex : 0);
                    if (countWithoutVariables > 0) {
                        int amountToTransfer = std::min(countWithoutVariables, itemsLeftToTransfer);
                        toActor->inventory.addItem(toMobile, item, amountToTransfer, false, nullptr);
                        fromActor->inventory.removeItemWithData(fromMobile, item, nullptr, amountToTransfer, false);

                        // Check for ammunition, as unlike other equipment, it does not generate itemData when equipped.
                        if (!fromIsContainer && item->objectType == TES3::ObjectType::Ammo) {
                            fromActor->unequipItem(item, true, fromMobile, false, nullptr);
                        }

                        fulfilledCount += amountToTransfer;
                        itemsLeftToTransfer -= amountToTransfer;
                    }

                    // Then transfer over items with data.
                    if (fromStack->variables) {
                        while (itemsLeftToTransfer > 0) {
                            auto itemData = fromStack->variables->storage[0];
                            toActor->inventory.addItem(toMobile, item, 1, false, &fromStack->variables->storage[0]);
                            fromActor->inventory.removeItemWithData(fromMobile, item, itemData, 1, false);

                            if (!fromIsContainer) {
                                fromActor->unequipItem(item, true, fromMobile, false, itemData);
                            }

                            fulfilledCount++;
                            itemsLeftToTransfer--;
                        }
                    }
                }
            }

            // No items to transfer? Great, let's get out of here.
            if (fulfilledCount == 0) {
                return 0;
            }

            // Play the relevant sound.
            auto worldController = TES3::WorldController::get();
            auto playerMobile = worldController->getMobilePlayer();
            if (getOptionalParam<bool>(params, "playSound", true)) {
                if (toMobile == playerMobile) {
                    worldController->playItemUpDownSound(item, true);
                }
                else if (fromMobile == playerMobile) {
                    worldController->playItemUpDownSound(item, false);
                }
            }

            // Update body parts for creatures/NPCs that may have items unequipped.
            if (fromMobile) {
                fromReference->updateBipedParts();

                if (fromMobile == playerMobile) {
                    playerMobile->firstPersonReference->updateBipedParts();
                }
            }

            // If either of them are the player, we need to update the GUI.
            if (getOptionalParam<bool>(params, "updateGUI", true)) {
                // Update inventory menu if necessary.
                if (fromMobile == playerMobile || toMobile == playerMobile) {
                    worldController->inventoryData->clearIcons(2);
                    worldController->inventoryData->addInventoryItems(&playerMobile->npcInstance->inventory, 2);
                    mwse::tes3::ui::inventoryUpdateIcons();
                }

                // Update contents menu if necessary.
                auto contentsMenu = TES3::UI::findMenu(*reinterpret_cast<TES3::UI::UI_ID*>(0x7D3098));
                if (contentsMenu) {
                    // Make sure that the contents reference is one of the ones we care about.
                    TES3::Reference * contentsReference = static_cast<TES3::Reference*>(contentsMenu->getProperty(TES3::UI::PropertyType::Pointer, *reinterpret_cast<TES3::UI::Property*>(0x7D3048)).ptrValue);
                    if (fromReference == contentsReference || toReference == contentsReference) {
                        // If we're looking at a companion, we need to update the profit value and trigger the GUI updates.
                        float isCompanion = *reinterpret_cast<float*>(0x7D3184);
                        if (isCompanion != 0.0f) {
                            float& companionProfit = *reinterpret_cast<float*>(0x7D3188);
                            if (toReference == contentsReference) {
                                companionProfit += fulfilledCount * item->getValue();
                            }
                            else {
                                companionProfit -= fulfilledCount * item->getValue();
                            }
                            TES3::UI::updateContentsCompanionElements();
                        }

                        // We also need to update the menu tiles.
                        TES3::UI::updateContentsMenuTiles();
                    }
                }
            }

            fromReference->setObjectModified(true);
            toReference->setObjectModified(true);
            return fulfilledCount;
        };

        state["tes3"]["addItemData"] = [](sol::table params) -> TES3::ItemData* {
            auto luaState = LuaManager::getInstance().getThreadSafeStateHandle();

            // Get the reference of the item or item container.
            TES3::Reference * toReference = getOptionalParamReference(params, "to");
            if (toReference == nullptr) {
                throw std::invalid_argument("Invalid 'to' parameter provided.");
            }
            // Check if it's setting a world reference or an item in a container.
            TES3::Actor * toActor = static_cast<TES3::Actor*>(toReference->baseObject);
            if (!toActor->isActor()) {
                // It's a reference. This is the easy part.
                // Return nil if there is already itemData, or return the newly-created itemData.
                if (toReference->getAttachedItemData()) {
                    return nullptr;
                }
                toReference->setObjectModified(true);
                return toReference->getOrCreateAttachedItemData();
            }

            // Get the item we are going to transfer.
            TES3::Item * item = getOptionalParamObject<TES3::Item>(params, "item");
            if (item == nullptr) {
                throw std::invalid_argument("Invalid 'item' parameter provided.");
            }

            // Does the reference need to be cloned?
            if (toReference->clone()) {
                toActor = static_cast<TES3::Actor*>(toReference->baseObject);
            }

            // Try to find an ItemData-less item and add ItemData for it.
            TES3::ItemStack * stack = toActor->inventory.findItemStack(item);
            if (!stack || stack->count < 1) {
                throw std::runtime_error("The actor does not possess any of 'item'.");
            }

            if (!stack->variables) {
                // Create array required to hold ItemData.
                stack->variables = TES3::TArray<TES3::ItemData>::create();
            }
            else if (stack->count <= stack->variables->filledCount) {
                // All items already have ItemData.
                return nullptr;
            }

            // Finally add ItemData to stack.
            TES3::ItemData * itemData = TES3::ItemData::createForObject(item);
            stack->variables->add(itemData);

            // If either of them are the player, we need to update the GUI.
            auto worldController = TES3::WorldController::get();
            auto toMobile = toReference->getAttachedMobileActor();
            auto playerMobile = worldController->getMobilePlayer();

            if (getOptionalParam<bool>(params, "updateGUI", true)) {
                // Update inventory menu if necessary.
                if (toMobile == playerMobile) {
                    worldController->inventoryData->clearIcons(2);
                    worldController->inventoryData->addInventoryItems(&playerMobile->npcInstance->inventory, 2);
                    mwse::tes3::ui::inventoryUpdateIcons();
                }

                // Update contents menu if necessary.
                auto contentsMenu = TES3::UI::findMenu(*reinterpret_cast<TES3::UI::UI_ID*>(0x7D3098));
                if (contentsMenu) {
                    // Make sure that the contents reference is one of the ones we care about.
                    TES3::Reference * contentsReference = static_cast<TES3::Reference*>(contentsMenu->getProperty(TES3::UI::PropertyType::Pointer, *reinterpret_cast<TES3::UI::Property*>(0x7D3048)).ptrValue);
                    if (toReference == contentsReference) {
                        TES3::UI::updateContentsMenuTiles();
                    }
                }
            }

            toReference->setObjectModified(true);
            return itemData;
        };

        state["tes3"]["dropItem"] = [](sol::table params) {
            // Who is dropping?
            TES3::MobileActor * mobile = getOptionalParamMobileActor(params, "reference");
            if (mobile == nullptr) {
                throw std::invalid_argument("Invalid reference parameter provided.");
            }

            // What are they dropping?
            TES3::Item * item = getOptionalParamObject<TES3::Item>(params, "item");
            if (item == nullptr) {
                throw std::invalid_argument("Invalid item parameter provided.");
            }

            // Get data about what is being dropped.
            TES3::ItemData * itemData = getOptionalParam<TES3::ItemData*>(params, "itemData", nullptr);
            int count = getOptionalParam<int>(params, "count", 1);
            bool matchExact = getOptionalParam<bool>(params, "matchExact", true);

            // Drop the item.
            mobile->dropItem(item, itemData, count, matchExact);
            auto droppedReference = mobile->getCell()->temporaryRefs.tail;

            // Update inventory tiles if needed.
            if (getOptionalParam<bool>(params, "updateGUI", true)) {
                auto worldController = TES3::WorldController::get();
                auto macp = worldController->getMobilePlayer();
                if (mobile == macp) {
                    worldController->inventoryData->clearIcons(2);
                    worldController->inventoryData->addInventoryItems(&macp->npcInstance->inventory, 2);
                    mwse::tes3::ui::inventoryUpdateIcons();
                }
            }

            return makeLuaObject(droppedReference);
        };

        state["tes3"]["persuade"] = [](sol::table params) {
            auto actor = getOptionalParamMobileActor(params, "actor");
            if (actor == nullptr) {
                throw std::invalid_argument("Invalid actor parameter provided.");
            }

            auto rng = rand() % 100;

            int index = getOptionalParam<int>(params, "index", -1);

            if (index >= 0 && index <= 6) {
                actor->reference->setObjectModified(true);
                return actor->persuade(rng, index);
            }
            else {
                auto fBribe100Mod = TES3::DataHandler::get()->nonDynamicData->GMSTs[TES3::GMST::fBribe100Mod];
                float modifier = getOptionalParam<float>(params, "modifier", 0.0f);
                if (modifier <= 0.0f) {
                    throw std::invalid_argument("Invalid modifier parameter provided.");
                }

                float oldModifier = fBribe100Mod->value.asFloat;
                fBribe100Mod->value.asFloat = modifier;

                bool result = actor->persuade(rng, 4);
                fBribe100Mod->value.asFloat = oldModifier;

                actor->reference->setObjectModified(true);

                return result;
            }
        };

        state["tes3"]["findDialogue"] = [](sol::table params) {
            int type = getOptionalParam<int>(params, "type", -1);
            int page = getOptionalParam<int>(params, "page", -1);
            return makeLuaObject(TES3::Dialogue::getDialogue(type, page));
        };

        state["tes3"]["isAffectedBy"] = [](sol::table params) {
            TES3::Reference * reference = getOptionalParamExecutionReference(params);
            if (reference == nullptr) {
                throw std::invalid_argument("Invalid 'reference' parameter provided.");
            }

            auto mact = reference->getAttachedMobileActor();
            if (mact == nullptr) {
                throw std::invalid_argument("Invalid 'reference' parameter provided. No mobile actor found.");
            }

            // Are we checking for being affected by an object?
            int effectId = getOptionalParam<int>(params, "effect", -1);
            TES3::BaseObject * object = getOptionalParamObject<TES3::BaseObject>(params, "object");
            if (object != nullptr) {
                if (object->objectType == TES3::ObjectType::Alchemy) {
                    return mact->isAffectedByAlchemy(static_cast<TES3::Alchemy*>(object));
                }
                else if (object->objectType == TES3::ObjectType::Enchantment) {
                    return mact->isAffectedByEnchantment(static_cast<TES3::Enchantment*>(object));
                }
                else if (object->objectType == TES3::ObjectType::Spell) {
                    return mact->isAffectedBySpell(static_cast<TES3::Spell*>(object));
                }
                else if (object->objectType == TES3::ObjectType::MagicEffect) {
                    effectId = static_cast<TES3::MagicEffect*>(object)->id;
                }
                else {
                    throw std::invalid_argument("Invalid 'object' parameter provided.");
                }
            }

            // Check based on effect ID.
            if (effectId > -1) {
                auto firstEffect = mact->activeMagicEffects.firstEffect;
                auto itt = firstEffect->next;
                while (itt != firstEffect) {
                    if (itt->magicEffectID == effectId) {
                        return true;
                    }
                    itt = itt->next;
                }
            }
            else {
                throw std::invalid_argument("Invalid 'effect' parameter provided.");
            }

            return false;
        };

        state["omw"]["addMagicEffect"] = [](sol::table params) {
            MWWorld::Store<ESM::MagicEffect> *effectsStore = const_cast<MWWorld::Store<ESM::MagicEffect>*>(&MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>());

            ESM::MagicEffect effect;

            // Get the new ID.
            int id = getOptionalParam<int>(params, "id", -1);
            if (id <= ESM::MagicEffect::SummonCreature05) {
                throw std::invalid_argument("Invalid 'id' parameter provided. It must be an integer greater than 142.");
            }
            else if (effectsStore->search(id)) {
                throw std::invalid_argument("Invalid 'id' parameter provided. An effect with this id already exists.");
            }

            // Create the effect and assign basic properties.
            //effect.mId = id;
            // FIXME: generate Id
            effect.mId = "custom_magic_effect_"+id;
            effect.mData.mBaseCost = getOptionalParam<float>(params, "baseCost", 1.0f);
            effect.mData.mSchool = getOptionalParam<int>(params, "school", 0);
            effect.mData.mUnknown1 = getOptionalParam<float>(params, "size", 1.0f);
            effect.mData.mUnknown2 = getOptionalParam<float>(params, "sizeCap", 1.0f);
            effect.mData.mSpeed = getOptionalParam<float>(params, "speed", 1.0f);

            // Description.
            sol::optional<std::string> description = params["description"];
            if (description) {
                effect.mDescription = description.value();
            }
            else {
                effect.mDescription = "No description available.";
            }

            effect.mData.mRed = std::min(std::max(getOptionalParam<int>(params, "lightingRed", 255), 0), 255);
            effect.mData.mGreen = std::min(std::max(getOptionalParam<int>(params, "lightingGreen", 255), 0), 255);
            effect.mData.mBlue = std::min(std::max(getOptionalParam<int>(params, "lightingBlue", 255), 0), 255);

            sol::optional<std::string> icon = params["icon"];
            if (!icon || icon.value().length() >= 31) {
                throw std::invalid_argument("Invalid 'icon' parameter provided. Must be a string no longer than 31 characters long.");
            }
            else {
                effect.mIcon = icon.value();
            }

            sol::optional<std::string> particleTexture = params["particleTexture"];
            if (!particleTexture || particleTexture.value().length() >= 31) {
                throw std::invalid_argument("Invalid 'particleTexture' parameter provided. Must be a string no longer than 31 characters long.");
            }
            else {
                effect.mParticle = particleTexture.value();
            }

            sol::optional<std::string> castSound = params["castSound"];
            if (!castSound || castSound.value().length() >= 31) {
                throw std::invalid_argument("Invalid 'castSound' parameter provided. Must be a string no longer than 31 characters long.");
            }
            else {
                effect.mCastSound = castSound.value();
            }

            sol::optional<std::string> boltSound = params["boltSound"];
            if (!boltSound || boltSound.value().length() >= 31) {
                throw std::invalid_argument("Invalid 'boltSound' parameter provided. Must be a string no longer than 31 characters long.");
            }
            else {
                effect.mBoltSound = castSound.value();
            }

            sol::optional<std::string> hitSound = params["hitSound"];
            if (!hitSound || hitSound.value().length() >= 31) {
                throw std::invalid_argument("Invalid 'hitSound' parameter provided. Must be a string no longer than 31 characters long.");
            }
            else {
                effect.mHitSound = hitSound.value();
            }

            sol::optional<std::string> areaSound = params["areaSound"];
            if (!areaSound || areaSound.value().length() >= 31) {
                throw std::invalid_argument("Invalid 'areaSound' parameter provided. Must be a string no longer than 31 characters long.");
            }
            else {
                effect.mAreaSound = areaSound.value();
            }

            sol::optional<std::string> castVfx = params["castVisualEffect"];
            if (!castVfx || castVfx.value().length() >= 31) {
                throw std::invalid_argument("Invalid 'castVisualEffect' parameter provided. Must be a string no longer than 31 characters long.");
            }
            else {
                effect.mArea = castVfx.value();
            }

            sol::optional<std::string> boltVfx = params["boltVisualEffect"];
            if (!boltVfx || boltVfx.value().length() >= 31) {
                throw std::invalid_argument("Invalid 'boltVisualEffect' parameter provided. Must be a string no longer than 31 characters long.");
            }
            else {
                effect.mBolt = boltVfx.value();
            }

            sol::optional<std::string> hitVfx = params["hitVisualEffect"];
            if (!hitVfx || hitVfx.value().length() >= 31) {
                throw std::invalid_argument("Invalid 'hitVisualEffect' parameter provided. Must be a string no longer than 31 characters long.");
            }
            else {
                effect.mHit = hitVfx.value();
            }

            sol::optional<std::string> areaVfx = params["areaVisualEffect"];
            if (!areaVfx || areaVfx.value().length() >= 31) {
                throw std::invalid_argument("Invalid 'areaVfx' parameter provided. Must be a string no longer than 31 characters long.");
            }
            else {
                effect.mAreaSound = areaVfx.value();
            }

            // FIXME: support for effect names

            sol::optional<std::string> name = params["name"];
            if (name) {
                magicEffectController->effectCustomNames[id] = name.value();
            }
            else {
                magicEffectController->effectCustomNames[id] = "Unnamed Effect";
            }

            // Set individual flags.
            effect.mData.mFlags = 0;
            effect.mData.mFlags |= getOptionalParam<bool>(params, "targetsSkills", true) ? ESM::MagicEffect::TargetSkill : 0;
            effect.mData.mFlags |= getOptionalParam<bool>(params, "targetsAttributes", true) ? ESM::MagicEffect::TargetAttribute : 0;
            effect.mData.mFlags |= getOptionalParam<bool>(params, "hasNoDuration", true) ? ESM::MagicEffect::NoDuration : 0;
            effect.mData.mFlags |= getOptionalParam<bool>(params, "hasNoMagnitude", true) ? ESM::MagicEffect::NoMagnitude : 0;
            effect.mData.mFlags |= getOptionalParam<bool>(params, "isHarmful", true) ? ESM::MagicEffect::TargetSkill : 0;
            effect.mData.mFlags |= getOptionalParam<bool>(params, "hasContinuousVFX", true) ? ESM::MagicEffect::ContinuousVfx : 0;
            effect.mData.mFlags |= getOptionalParam<bool>(params, "canCastSelf", true) ? ESM::MagicEffect::CastSelf : 0;
            effect.mData.mFlags |= getOptionalParam<bool>(params, "canCastTouch", true) ? ESM::MagicEffect::CastTouch : 0;
            effect.mData.mFlags |= getOptionalParam<bool>(params, "canCastTarget", true) ? ESM::MagicEffect::CastTarget : 0;
            effect.mData.mFlags |= getOptionalParam<bool>(params, "allowEnchanting", true) ? ESM::MagicEffect::AllowEnchanting : 0;
            effect.mData.mFlags |= getOptionalParam<bool>(params, "allowSpellmaking", true) ? ESM::MagicEffect::AllowSpellmaking : 0;
            effect.mData.mFlags |= getOptionalParam<bool>(params, "usesNegativeLighting", true) ? ESM::MagicEffect::NegativeLight : 0;
            effect.mData.mFlags |= getOptionalParam<bool>(params, "appliesOnce", true) ? ESM::MagicEffect::UncappedDamage : 0;
            effect.mData.mFlags |= getOptionalParam<bool>(params, "nonRecastable", true) ? ESM::MagicEffect::NonRecastable : 0;
            effect.mData.mFlags |= getOptionalParam<bool>(params, "illegalDaedra", true) ? ESM::MagicEffect::IllegalDaedra : 0;
            effect.mData.mFlags |= getOptionalParam<bool>(params, "unreflectable", true) ? ESM::MagicEffect::Unreflectable : 0;
            effect.mData.mFlags |= getOptionalParam<bool>(params, "casterLinked", true) ? ESM::MagicEffect::CasterLinked : 0;

            // Actually add the effect.
            // FIXME: implement actual adding
            effectsStore->insertStatic(effect);

            ESM::GameSetting newGmst;
            MWWorld::Store<ESM::GameSetting> *gmst = const_cast<MWWorld::Store<ESM::GameSetting>*>(&MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>());
            MWWorld::Store<ESM::MagicEffect> *effectsStore = const_cast<MWWorld::Store<ESM::MagicEffect>*>(&MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>());
            gmst->insertStatic(newGmst);

            // FIXME: support for effects handlers
            // Get the tick function.
            sol::optional<sol::protected_function> onTick = params["onTick"];
            if (onTick) {
                magicEffectController->effectLuaTickFunctions[id] = onTick.value();
            }
            sol::optional<sol::protected_function> onCollision = params["onCollision"];
            if (onCollision) {
                magicEffectController->effectLuaCollisionFunctions[id] = onCollision.value();
            }

            // Set the GMST as the negative effect ID for custom hooks later.
            magicEffectController->effectNameGMSTs[id] = -id;

            ESM::MagicEffect *addedEffect = const_cast<ESM::MagicEffect*>(effectsStore->find(id));

            return makeLuaObject(addedEffect);
        };
        */
    }
}
