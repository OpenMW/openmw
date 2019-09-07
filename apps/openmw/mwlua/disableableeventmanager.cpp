#include "disableableeventmanager.hpp"

#include "luamanager.hpp"

#include "loadedgame.hpp"

/*
#include "LuaActivateEvent.h"
#include "LuaActivationTargetChangedEvent.h"
#include "LuaAddTopicEvent.h"
#include "LuaAttackEvent.h"
#include "LuaBodyPartsUpdatedEvent.h"
#include "LuaBookGetTextEvent.h"
#include "LuaButtonPressedEvent.h"
#include "LuaCalcArmorRatingEvent.h"
#include "LuaCalcBarterPriceEvent.h"
#include "LuaCalcHitArmorPieceEvent.h"
#include "LuaCalcHitChanceEvent.h"
#include "LuaCalcMovementSpeedEvent.h"
#include "LuaCalcRepairPriceEvent.h"
#include "LuaCalcRestInterruptEvent.h"
#include "LuaCalcSoulValueEvent.h"
#include "LuaCalcSpellPriceEvent.h"
#include "LuaCalcTrainingPriceEvent.h"
#include "LuaCalcTravelPriceEvent.h"
#include "LuaCellChangedEvent.h"
#include "LuaCombatStartEvent.h"
#include "LuaCombatStartedEvent.h"
#include "LuaCombatStopEvent.h"
#include "LuaCombatStoppedEvent.h"
#include "LuaConvertReferenceToItemEvent.h"
#include "LuaCrimeWitnessedEvent.h"
#include "LuaDamageEvent.h"
#include "LuaDamagedEvent.h"
#include "LuaDeathEvent.h"
#include "LuaDetermineActionEvent.h"
#include "LuaDeterminedActionEvent.h"
#include "LuaDisarmTrapEvent.h"
#include "LuaEquipEvent.h"
#include "LuaEquippedEvent.h"
#include "LuaFilterBarterMenuEvent.h"
#include "LuaFilterContentsMenuEvent.h"
#include "LuaFilterInventoryEvent.h"
#include "LuaFilterInventorySelectEvent.h"
#include "LuaFilterSoulGemTargetEvent.h"
#include "LuaFrameEvent.h"
#include "LuaGenericUiActivatedEvent.h"
#include "LuaGenericUiPostEvent.h"
#include "LuaGenericUiPreEvent.h"
#include "LuaInfoFilterEvent.h"
#include "LuaInfoGetTextEvent.h"
#include "LuaInfoResponseEvent.h"
#include "LuaItemDroppedEvent.h"
#include "LuaItemTileUpdatedEvent.h"
#include "LuaJournalEvent.h"
#include "LuaKeyDownEvent.h"
#include "LuaKeyEvent.h"
#include "LuaKeyUpEvent.h"
#include "LuaLevelUpEvent.h"
#include "LuaLeveledCreaturePickedEvent.h"
#include "LuaLeveledItemPickedEvent.h"
#include "LuaLoadGameEvent.h"
#include "LuaLoadedGameEvent.h"
#include "LuaMagicCastedEvent.h"
#include "LuaMenuStateEvent.h"
#include "LuaMeshLoadedEvent.h"
#include "LuaMobileActorActivatedEvent.h"
#include "LuaMobileActorDeactivatedEvent.h"
#include "LuaMobileObjectCollisionEvent.h"
#include "LuaMobileObjectWaterImpactEvent.h"
#include "LuaMobileProjectileActorCollisionEvent.h"
#include "LuaMobileProjectileObjectCollisionEvent.h"
#include "LuaMobileProjectileTerrainCollisionEvent.h"
#include "LuaMouseAxisEvent.h"
#include "LuaMouseButtonDownEvent.h"
#include "LuaMouseButtonUpEvent.h"
#include "LuaMouseWheelEvent.h"
#include "LuaMusicSelectTrackEvent.h"
#include "LuaPickLockEvent.h"
#include "LuaPotionBrewedEvent.h"
#include "LuaProjectileExpireEvent.h"
#include "LuaReferenceSceneNodeCreatedEvent.h"
#include "LuaRestInterruptEvent.h"
#include "LuaSaveGameEvent.h"
#include "LuaSavedGameEvent.h"
#include "LuaShowRestWaitMenuEvent.h"
#include "LuaSimulateEvent.h"
#include "LuaSkillExerciseEvent.h"
#include "LuaSkillRaisedEvent.h"
#include "LuaSpellCastEvent.h"
#include "LuaSpellCastedEvent.h"
#include "LuaSpellResistEvent.h"
#include "LuaSpellTickEvent.h"
#include "LuaUiObjectTooltipEvent.h"
#include "LuaUiRefreshedEvent.h"
#include "LuaUiSpellTooltipEvent.h"
#include "LuaUnequippedEvent.h"
#include "LuaWeaponReadiedEvent.h"
#include "LuaWeaponUnreadiedEvent.h"
#include "LuaWeatherChangedImmediateEvent.h"
#include "LuaWeatherCycledEvent.h"
#include "LuaWeatherTransitionFinishedEvent.h"
#include "LuaWeatherTransitionStartedEvent.h"
*/

namespace mwse {
	namespace lua {
		namespace event {
			void DisableableEventManager::bindToLua() {
				// Get our lua state.
				auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
				sol::state& state = stateHandle.state;

				// Start our usertype. We must finish this with state.set_usertype.
				auto usertypeDefinition = state.create_simple_usertype<DisableableEventManager>();
				usertypeDefinition.set("new", sol::no_constructor);

                //usertypeDefinition.set("loaded", true);
                usertypeDefinition.set("loaded", sol::property(&LoadedGameEvent::getEventEnabled, &LoadedGameEvent::setEventEnabled));

                /*
				// Give access to the enabled state.
				usertypeDefinition.set("activate", sol::property(&ActivateEvent::getEventEnabled, &ActivateEvent::setEventEnabled));
				usertypeDefinition.set("activationTargetChanged", sol::property(&ActivationTargetChangedEvent::getEventEnabled, &ActivationTargetChangedEvent::setEventEnabled));
				usertypeDefinition.set("attack", sol::property(&AttackEvent::getEventEnabled, &AttackEvent::setEventEnabled));
				usertypeDefinition.set("bodyPartsUpdated", sol::property(&BodyPartsUpdatedEvent::getEventEnabled, &BodyPartsUpdatedEvent::setEventEnabled));
				usertypeDefinition.set("bookGetText", sol::property(&BookGetTextEvent::getEventEnabled, &BookGetTextEvent::setEventEnabled));
				usertypeDefinition.set("buttonPressed", sol::property(&ButtonPressedEvent::getEventEnabled, &ButtonPressedEvent::setEventEnabled));
				usertypeDefinition.set("calcArmorPieceHit", sol::property(&CalcHitArmorPiece::getEventEnabled, &CalcHitArmorPiece::setEventEnabled));
				usertypeDefinition.set("calcArmorRating", sol::property(&CalculateArmorRatingEvent::getEventEnabled, &CalculateArmorRatingEvent::setEventEnabled));
				usertypeDefinition.set("calcBarterPrice", sol::property(&CalculateBarterPriceEvent::getEventEnabled, &CalculateBarterPriceEvent::setEventEnabled));
				usertypeDefinition.set("calcFlySpeed", sol::property(&CalculateMovementSpeed::getEventEnabled, &CalculateMovementSpeed::setEventEnabled));
				usertypeDefinition.set("calcHitChance", sol::property(&CalcHitChanceEvent::getEventEnabled, &CalcHitChanceEvent::setEventEnabled));
				usertypeDefinition.set("calcMoveSpeed", sol::property(&CalculateMovementSpeed::getEventEnabled, &CalculateMovementSpeed::setEventEnabled));
				usertypeDefinition.set("calcRepairPrice", sol::property(&CalculateRepairPriceEvent::getEventEnabled, &CalculateRepairPriceEvent::setEventEnabled));
				usertypeDefinition.set("calcRestInterrupt", sol::property(&CalcRestInterruptEvent::getEventEnabled, &CalcRestInterruptEvent::setEventEnabled));
				usertypeDefinition.set("calcRunSpeed", sol::property(&CalculateMovementSpeed::getEventEnabled, &CalculateMovementSpeed::setEventEnabled));
				usertypeDefinition.set("calcSoulValue", sol::property(&CalculateSoulValueEvent::getEventEnabled, &CalculateSoulValueEvent::setEventEnabled));
				usertypeDefinition.set("calcSpellPrice", sol::property(&CalculateSpellPriceEvent::getEventEnabled, &CalculateSpellPriceEvent::setEventEnabled));
				usertypeDefinition.set("calcSwimRunSpeed", sol::property(&CalculateMovementSpeed::getEventEnabled, &CalculateMovementSpeed::setEventEnabled));
				usertypeDefinition.set("calcSwimSpeed", sol::property(&CalculateMovementSpeed::getEventEnabled, &CalculateMovementSpeed::setEventEnabled));
				usertypeDefinition.set("calcTrainingPrice", sol::property(&CalculateTrainingPriceEvent::getEventEnabled, &CalculateTrainingPriceEvent::setEventEnabled));
				usertypeDefinition.set("calcTravelPrice", sol::property(&CalculateTravelPriceEvent::getEventEnabled, &CalculateTravelPriceEvent::setEventEnabled));
				usertypeDefinition.set("calcWalkSpeed", sol::property(&CalculateMovementSpeed::getEventEnabled, &CalculateMovementSpeed::setEventEnabled));
				usertypeDefinition.set("cellChanged", sol::property(&CellChangedEvent::getEventEnabled, &CellChangedEvent::setEventEnabled));
				usertypeDefinition.set("collideWater", sol::property(&MobileObjectWaterImpactEvent::getEventEnabled, &MobileObjectWaterImpactEvent::setEventEnabled));
				usertypeDefinition.set("collision", sol::property(&MobileObjectCollisionEvent::getEventEnabled, &MobileObjectCollisionEvent::setEventEnabled));
				usertypeDefinition.set("combatStart", sol::property(&CombatStartEvent::getEventEnabled, &CombatStartEvent::setEventEnabled));
				usertypeDefinition.set("combatStarted", sol::property(&CombatStartedEvent::getEventEnabled, &CombatStartedEvent::setEventEnabled));
				usertypeDefinition.set("combatStop", sol::property(&CombatStopEvent::getEventEnabled, &CombatStopEvent::setEventEnabled));
				usertypeDefinition.set("combatStopped", sol::property(&CombatStoppedEvent::getEventEnabled, &CombatStoppedEvent::setEventEnabled));
				usertypeDefinition.set("convertReferenceToItem", sol::property(&ConvertReferenceToItemEvent::getEventEnabled, &ConvertReferenceToItemEvent::setEventEnabled));
				usertypeDefinition.set("crimeWitnessed", sol::property(&CrimeWitnessedEvent::getEventEnabled, &CrimeWitnessedEvent::setEventEnabled));
				usertypeDefinition.set("damage", sol::property(&DamageEvent::getEventEnabled, &DamageEvent::setEventEnabled));
				usertypeDefinition.set("damaged", sol::property(&DamagedEvent::getEventEnabled, &DamagedEvent::setEventEnabled));
				usertypeDefinition.set("death", sol::property(&DeathEvent::getEventEnabled, &DeathEvent::setEventEnabled));
				usertypeDefinition.set("determineAction", sol::property(&DetermineActionEvent::getEventEnabled, &DetermineActionEvent::setEventEnabled));
				usertypeDefinition.set("determinedAction", sol::property(&DeterminedActionEvent::getEventEnabled, &DeterminedActionEvent::setEventEnabled));
				usertypeDefinition.set("enterFrame", sol::property(&FrameEvent::getEventEnabled, &FrameEvent::setEventEnabled));
				usertypeDefinition.set("equip", sol::property(&EquipEvent::getEventEnabled, &EquipEvent::setEventEnabled));
				usertypeDefinition.set("equipped", sol::property(&EquippedEvent::getEventEnabled, &EquippedEvent::setEventEnabled));
				usertypeDefinition.set("exerciseSkill", sol::property(&SkillExerciseEvent::getEventEnabled, &SkillExerciseEvent::setEventEnabled));
				usertypeDefinition.set("filterBarterMenu", sol::property(&FilterBarterMenuEvent::getEventEnabled, &FilterBarterMenuEvent::setEventEnabled));
				usertypeDefinition.set("filterContentsMenu", sol::property(&FilterContentsMenuEvent::getEventEnabled, &FilterContentsMenuEvent::setEventEnabled));
				usertypeDefinition.set("filterInventory", sol::property(&FilterInventoryEvent::getEventEnabled, &FilterInventoryEvent::setEventEnabled));
				usertypeDefinition.set("filterInventorySelect", sol::property(&FilterInventorySelectEvent::getEventEnabled, &FilterInventorySelectEvent::setEventEnabled));
				usertypeDefinition.set("filterSoulGemTarget", sol::property(&FilterSoulGemTargetEvent::getEventEnabled, &FilterSoulGemTargetEvent::setEventEnabled));
				usertypeDefinition.set("infoFilter", sol::property(&InfoFilterEvent::getEventEnabled, &InfoFilterEvent::setEventEnabled));
				usertypeDefinition.set("infoGetText", sol::property(&InfoGetTextEvent::getEventEnabled, &InfoGetTextEvent::setEventEnabled));
				usertypeDefinition.set("infoResponse", sol::property(&InfoResponseEvent::getEventEnabled, &InfoResponseEvent::setEventEnabled));
				usertypeDefinition.set("itemDropped", sol::property(&ItemDroppedEvent::getEventEnabled, &ItemDroppedEvent::setEventEnabled));
				usertypeDefinition.set("itemTileUpdated", sol::property(&ItemTileUpdatedEvent::getEventEnabled, &ItemTileUpdatedEvent::setEventEnabled));
				usertypeDefinition.set("journal", sol::property(&JournalEvent::getEventEnabled, &JournalEvent::setEventEnabled));
				usertypeDefinition.set("keyDown", sol::property(&KeyDownEvent::getEventEnabled, &KeyDownEvent::setEventEnabled));
				usertypeDefinition.set("keyUp", sol::property(&KeyUpEvent::getEventEnabled, &KeyUpEvent::setEventEnabled));
				usertypeDefinition.set("leveledCreaturePicked", sol::property(&LeveledCreaturePickedEvent::getEventEnabled, &LeveledCreaturePickedEvent::setEventEnabled));
				usertypeDefinition.set("leveledItemPicked", sol::property(&LeveledItemPickedEvent::getEventEnabled, &LeveledItemPickedEvent::setEventEnabled));
				usertypeDefinition.set("levelUp", sol::property(&LevelUpEvent::getEventEnabled, &LevelUpEvent::setEventEnabled));
				usertypeDefinition.set("load", sol::property(&LoadGameEvent::getEventEnabled, &LoadGameEvent::setEventEnabled));
				usertypeDefinition.set("lockPick", sol::property(&PickLockEvent::getEventEnabled, &PickLockEvent::setEventEnabled));
				usertypeDefinition.set("magicCasted", sol::property(&MagicCastedEvent::getEventEnabled, &MagicCastedEvent::setEventEnabled));
				usertypeDefinition.set("menuEnter", sol::property(&MenuStateEvent::getEventEnabled, &MenuStateEvent::setEventEnabled));
				usertypeDefinition.set("menuExit", sol::property(&MenuStateEvent::getEventEnabled, &MenuStateEvent::setEventEnabled));
				usertypeDefinition.set("meshLoaded", sol::property(&MeshLoadedEvent::getEventEnabled, &MeshLoadedEvent::setEventEnabled));
				usertypeDefinition.set("mobileActivated", sol::property(&MobileActorActivatedEvent::getEventEnabled, &MobileActorActivatedEvent::setEventEnabled));
				usertypeDefinition.set("mobileDeactivated", sol::property(&MobileActorDeactivatedEvent::getEventEnabled, &MobileActorDeactivatedEvent::setEventEnabled));
				usertypeDefinition.set("mouseAxis", sol::property(&MouseAxisEvent::getEventEnabled, &MouseAxisEvent::setEventEnabled));
				usertypeDefinition.set("mouseButtonDown", sol::property(&MouseButtonDownEvent::getEventEnabled, &MouseButtonDownEvent::setEventEnabled));
				usertypeDefinition.set("mouseButtonUp", sol::property(&MouseButtonUpEvent::getEventEnabled, &MouseButtonUpEvent::setEventEnabled));
				usertypeDefinition.set("mouseWheel", sol::property(&MouseWheelEvent::getEventEnabled, &MouseWheelEvent::setEventEnabled));
				usertypeDefinition.set("musicSelectTrack", sol::property(&MusicSelectTrackEvent::getEventEnabled, &MusicSelectTrackEvent::setEventEnabled));
				usertypeDefinition.set("potionBrewed", sol::property(&PotionBrewedEvent::getEventEnabled, &PotionBrewedEvent::setEventEnabled));
				usertypeDefinition.set("projectileExpire", sol::property(&ProjectileExpireEvent::getEventEnabled, &ProjectileExpireEvent::setEventEnabled));
				usertypeDefinition.set("projectileHitActor", sol::property(&MobileProjectileActorCollisionEvent::getEventEnabled, &MobileProjectileActorCollisionEvent::setEventEnabled));
				usertypeDefinition.set("projectileHitObject", sol::property(&MobileProjectileObjectCollisionEvent::getEventEnabled, &MobileProjectileObjectCollisionEvent::setEventEnabled));
				usertypeDefinition.set("projectileHitTerrain", sol::property(&MobileProjectileTerrainCollisionEvent::getEventEnabled, &MobileProjectileTerrainCollisionEvent::setEventEnabled));
				usertypeDefinition.set("referenceSceneNodeCreated", sol::property(&ReferenceSceneNodeCreatedEvent::getEventEnabled, &ReferenceSceneNodeCreatedEvent::setEventEnabled));
				usertypeDefinition.set("restInterrupt", sol::property(&RestInterruptEvent::getEventEnabled, &RestInterruptEvent::setEventEnabled));
				usertypeDefinition.set("save", sol::property(&SaveGameEvent::getEventEnabled, &SaveGameEvent::setEventEnabled));
				usertypeDefinition.set("saved", sol::property(&SavedGameEvent::getEventEnabled, &SavedGameEvent::setEventEnabled));
				usertypeDefinition.set("simulate", sol::property(&SimulateEvent::getEventEnabled, &SimulateEvent::setEventEnabled));
				usertypeDefinition.set("skillRaised", sol::property(&SkillRaisedEvent::getEventEnabled, &SkillRaisedEvent::setEventEnabled));
				usertypeDefinition.set("spellCast", sol::property(&SpellCastEvent::getEventEnabled, &SpellCastEvent::setEventEnabled));
				usertypeDefinition.set("spellCasted", sol::property(&SpellCastedEvent::getEventEnabled, &SpellCastedEvent::setEventEnabled));
				usertypeDefinition.set("spellCastedFailure", sol::property(&SpellCastedEvent::getEventEnabled, &SpellCastedEvent::setEventEnabled));
				usertypeDefinition.set("spellResist", sol::property(&SpellResistEvent::getEventEnabled, &SpellResistEvent::setEventEnabled));
				usertypeDefinition.set("spellTick", sol::property(&SpellTickEvent::getEventEnabled, &SpellTickEvent::setEventEnabled));
				usertypeDefinition.set("topicAdded", sol::property(&AddTopicEvent::getEventEnabled, &AddTopicEvent::setEventEnabled));
				usertypeDefinition.set("trapDisarm", sol::property(&DisarmTrapEvent::getEventEnabled, &DisarmTrapEvent::setEventEnabled));
				usertypeDefinition.set("uiActivated", sol::property(&GenericUiActivatedEvent::getEventEnabled, &GenericUiActivatedEvent::setEventEnabled));
				usertypeDefinition.set("uiEvent", sol::property(&GenericUiPostEvent::getEventEnabled, &GenericUiPostEvent::setEventEnabled));
				usertypeDefinition.set("uiObjectTooltip", sol::property(&UiObjectTooltipEvent::getEventEnabled, &UiObjectTooltipEvent::setEventEnabled));
				usertypeDefinition.set("uiPreEvent", sol::property(&GenericUiPreEvent::getEventEnabled, &GenericUiPreEvent::setEventEnabled));
				usertypeDefinition.set("uiRefreshed", sol::property(&UiRefreshedEvent::getEventEnabled, &UiRefreshedEvent::setEventEnabled));
				usertypeDefinition.set("uiShowRestMenu", sol::property(&ShowRestWaitMenuEvent::getEventEnabled, &ShowRestWaitMenuEvent::setEventEnabled));
				usertypeDefinition.set("uiSpellTooltip", sol::property(&UiSpellTooltipEvent::getEventEnabled, &UiSpellTooltipEvent::setEventEnabled));
				usertypeDefinition.set("unequipped", sol::property(&UnequippedEvent::getEventEnabled, &UnequippedEvent::setEventEnabled));
				usertypeDefinition.set("weaponReadied", sol::property(&WeaponReadiedEvent::getEventEnabled, &WeaponReadiedEvent::setEventEnabled));
				usertypeDefinition.set("weaponUnreadied", sol::property(&WeaponUnreadiedEvent::getEventEnabled, &WeaponUnreadiedEvent::setEventEnabled));
				usertypeDefinition.set("weatherChangedImmediate", sol::property(&WeatherChangedImmediateEvent::getEventEnabled, &WeatherChangedImmediateEvent::setEventEnabled));
				usertypeDefinition.set("weatherCycled", sol::property(&WeatherCycledEvent::getEventEnabled, &WeatherCycledEvent::setEventEnabled));
				usertypeDefinition.set("weatherTransitionFinished", sol::property(&WeatherTransitionFinishedEvent::getEventEnabled, &WeatherTransitionFinishedEvent::setEventEnabled));
				usertypeDefinition.set("weatherTransitionStarted", sol::property(&WeatherTransitionStartedEvent::getEventEnabled, &WeatherTransitionStartedEvent::setEventEnabled));
				*/

				// Finish up our usertype.
				state.set_usertype("disableableEventManager", usertypeDefinition);
			}
		}
	}
}
