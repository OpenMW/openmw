/*
  Copyright (C) 2020-2021 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

  Much of the information on the data structures are based on the information
  from Tes4Mod:Mod_File_Format and Tes5Mod:File_Formats but also refined by
  trial & error.  See http://en.uesp.net/wiki for details.

  Also see https://tes5edit.github.io/fopdoc/ for FO3/FONV specific details.

*/
#ifndef ESM4_SCRIPT_H
#define ESM4_SCRIPT_H

#include <cstdint>
#include <string>
#include <vector>

namespace ESM4
{
    enum EmotionType
    {
        EMO_Neutral  = 0,
        EMO_Anger    = 1,
        EMO_Disgust  = 2,
        EMO_Fear     = 3,
        EMO_Sad      = 4,
        EMO_Happy    = 5,
        EMO_Surprise = 6,
        EMO_Pained   = 7 // FO3/FONV
    };

    enum ConditionTypeAndFlag
    {
        // flag
        CTF_Combine     = 0x01,
        CTF_RunOnTarget = 0x02,
        CTF_UseGlobal   = 0x04,
        // condition
        CTF_EqualTo     = 0x00,
        CTF_NotEqualTo  = 0x20,
        CTF_GreaterThan = 0x40,
        CTF_GrThOrEqTo  = 0x60,
        CTF_LessThan    = 0x80,
        CTF_LeThOrEqTo  = 0xA0
    };

    enum FunctionIndices
    {
        FUN_GetDistance      = 1,
        FUN_GetLocked        = 5,
        FUN_GetPos           = 6,
        FUN_GetAngle         = 8,
        FUN_GetStartingPos   = 10,
        FUN_GetStartingAngle = 11,
        FUN_GetSecondsPassed = 12,
        FUN_GetActorValue    = 14,
        FUN_GetCurrentTime   = 18,
        FUN_GetScale         = 24,
        FUN_IsMoving         = 25,
        FUN_IsTurning        = 26,
        FUN_GetLineOfSight   = 27,
        FUN_GetIsInSameCell  = 32,
        FUN_GetDisabled      = 35,
        FUN_GetMenuMode      = 36,
        FUN_GetDisease       = 39,
        FUN_GetVampire       = 40,
        FUN_GetClothingValue = 41,
        FUN_SameFaction      = 42,
        FUN_SameRace         = 43,
        FUN_SameSex          = 44,
        FUN_GetDetected      = 45,
        FUN_GetDead          = 46,
        FUN_GetItemCount     = 47,
        FUN_GetGold          = 48,
        FUN_GetSleeping      = 49,
        FUN_GetTalkedToPC    = 50,
        FUN_GetScriptVariable = 53,
        FUN_GetQuestRunning  = 56,
        FUN_GetStage         = 58,
        FUN_GetStageDone     = 59,
        FUN_GetFactionRankDifference = 60,
        FUN_GetAlarmed       = 61,
        FUN_IsRaining        = 62,
        FUN_GetAttacked      = 63,
        FUN_GetIsCreature    = 64,
        FUN_GetLockLevel     = 65,
        FUN_GetShouldAttack  = 66,
        FUN_GetInCell        = 67,
        FUN_GetIsClass       = 68,
        FUN_GetIsRace        = 69,
        FUN_GetIsSex         = 70,
        FUN_GetInFaction     = 71,
        FUN_GetIsID          = 72,
        FUN_GetFactionRank   = 73,
        FUN_GetGlobalValue   = 74,
        FUN_IsSnowing        = 75,
        FUN_GetDisposition   = 76,
        FUN_GetRandomPercent = 77,
        FUN_GetQuestVariable = 79,
        FUN_GetLevel         = 80,
        FUN_GetArmorRating   = 81,
        FUN_GetDeadCount     = 84,
        FUN_GetIsAlerted     = 91,
        FUN_GetPlayerControlsDisabled = 98,
        FUN_GetHeadingAngle  = 99,
        FUN_IsWeaponOut      = 101,
        FUN_IsTorchOut       = 102,
        FUN_IsShieldOut      = 103,
        FUN_IsFacingUp       = 106,
        FUN_GetKnockedState  = 107,
        FUN_GetWeaponAnimType = 108,
        FUN_IsWeaponSkillType = 109,
        FUN_GetCurrentAIPackage = 110,
        FUN_IsWaiting        = 111,
        FUN_IsIdlePlaying    = 112,
        FUN_GetMinorCrimeCount = 116,
        FUN_GetMajorCrimeCount = 117,
        FUN_GetActorAggroRadiusViolated = 118,
        FUN_GetCrime         = 122,
        FUN_IsGreetingPlayer = 123,
        FUN_IsGuard          = 125,
        FUN_HasBeenEaten     = 127,
        FUN_GetFatiguePercentage = 128,
        FUN_GetPCIsClass     = 129,
        FUN_GetPCIsRace      = 130,
        FUN_GetPCIsSex       = 131,
        FUN_GetPCInFaction   = 132,
        FUN_SameFactionAsPC  = 133,
        FUN_SameRaceAsPC     = 134,
        FUN_SameSexAsPC      = 135,
        FUN_GetIsReference   = 136,
        FUN_IsTalking        = 141,
        FUN_GetWalkSpeed     = 142,
        FUN_GetCurrentAIProcedure = 143,
        FUN_GetTrespassWarningLevel = 144,
        FUN_IsTrespassing    = 145,
        FUN_IsInMyOwnedCell  = 146,
        FUN_GetWindSpeed     = 147,
        FUN_GetCurrentWeatherPercent = 148,
        FUN_GetIsCurrentWeather = 149,
        FUN_IsContinuingPackagePCNear = 150,
        FUN_CanHaveFlames    = 153,
        FUN_HasFlames        = 154,
        FUN_GetOpenState     = 157,
        FUN_GetSitting       = 159,
        FUN_GetFurnitureMarkerID = 160,
        FUN_GetIsCurrentPackage = 161,
        FUN_IsCurrentFurnitureRef = 162,
        FUN_IsCurrentFurnitureObj = 163,
        FUN_GetDayofWeek     = 170,
        FUN_GetTalkedToPCParam = 172,
        FUN_IsPCSleeping     = 175,
        FUN_IsPCAMurderer    = 176,
        FUN_GetDetectionLevel = 180,
        FUN_GetEquipped      = 182,
        FUN_IsSwimming       = 185,
        FUN_GetAmountSoldStolen = 190,
        FUN_GetIgnoreCrime   = 192,
        FUN_GetPCExpelled    = 193,
        FUN_GetPCFactionMurder = 195,
        FUN_GetPCEnemyofFaction = 197,
        FUN_GetPCFactionAttack = 199,
        FUN_GetDestroyed     = 203,
        FUN_HasMagicEffect   = 214,
        FUN_GetDefaultOpen   = 215,
        FUN_GetAnimAction    = 219,
        FUN_IsSpellTarget    = 223,
        FUN_GetVATSMode      = 224,
        FUN_GetPersuasionNumber = 225,
        FUN_GetSandman       = 226,
        FUN_GetCannibal      = 227,
        FUN_GetIsClassDefault = 228,
        FUN_GetClassDefaultMatch = 229,
        FUN_GetInCellParam   = 230,
        FUN_GetVatsTargetHeight = 235,
        FUN_GetIsGhost       = 237,
        FUN_GetUnconscious   = 242,
        FUN_GetRestrained    = 244,
        FUN_GetIsUsedItem    = 246,
        FUN_GetIsUsedItemType = 247,
        FUN_GetIsPlayableRace = 254,
        FUN_GetOffersServicesNow = 255,
        FUN_GetUsedItemLevel = 258,
        FUN_GetUsedItemActivate = 259,
        FUN_GetBarterGold    = 264,
        FUN_IsTimePassing    = 265,
        FUN_IsPleasant       = 266,
        FUN_IsCloudy         = 267,
        FUN_GetArmorRatingUpperBody = 274,
        FUN_GetBaseActorValue = 277,
        FUN_IsOwner          = 278,
        FUN_IsCellOwner      = 280,
        FUN_IsHorseStolen    = 282,
        FUN_IsLeftUp         = 285,
        FUN_IsSneaking       = 286,
        FUN_IsRunning        = 287,
        FUN_GetFriendHit     = 288,
        FUN_IsInCombat       = 289,
        FUN_IsInInterior     = 300,
        FUN_IsWaterObject    = 304,
        FUN_IsActorUsingATorch = 306,
        FUN_IsXBox           = 309,
        FUN_GetInWorldspace  = 310,
        FUN_GetPCMiscStat    = 312,
        FUN_IsActorEvil      = 313,
        FUN_IsActorAVictim   = 314,
        FUN_GetTotalPersuasionNumber = 315,
        FUN_GetIdleDoneOnce  = 318,
        FUN_GetNoRumors      = 320,
        FUN_WhichServiceMenu = 323,
        FUN_IsRidingHorse    = 327,
        FUN_IsInDangerousWater = 332,
        FUN_GetIgnoreFriendlyHits = 338,
        FUN_IsPlayersLastRiddenHorse = 339,
        FUN_IsActor          = 353,
        FUN_IsEssential      = 354,
        FUN_IsPlayerMovingIntoNewSpace = 358,
        FUN_GetTimeDead      = 361,
        FUN_GetPlayerHasLastRiddenHorse = 362,
        FUN_IsChild          = 365,
        FUN_GetLastPlayerAction = 367,
        FUN_IsPlayerActionActive = 368,
        FUN_IsTalkingActivatorActor = 370,
        FUN_IsInList         = 372,
        FUN_GetHasNote       = 382,
        FUN_GetHitLocation   = 391,
        FUN_IsPC1stPerson    = 392,
        FUN_GetCauseofDeath  = 397,
        FUN_IsLimbGone       = 398,
        FUN_IsWeaponInList   = 399,
        FUN_HasFriendDisposition = 403,
        FUN_GetVATSValue     = 408,
        FUN_IsKiller         = 409,
        FUN_IsKillerObject   = 410,
        FUN_GetFactionCombatReaction = 411,
        FUN_Exists           = 415,
        FUN_GetGroupMemberCount = 416,
        FUN_GetGroupTargetCount = 417,
        FUN_GetObjectiveCompleted = 420,
        FUN_GetObjectiveDisplayed = 421,
        FUN_GetIsVoiceType   = 427,
        FUN_GetPlantedExplosive = 428,
        FUN_IsActorTalkingThroughActivator = 430,
        FUN_GetHealthPercentage = 431,
        FUN_GetIsObjectType  = 433,
        FUN_GetDialogueEmotion = 435,
        FUN_GetDialogueEmotionValue = 436,
        FUN_GetIsCreatureType = 438,
        FUN_GetInZone        = 446,
        FUN_HasPerk          = 449,
        FUN_GetFactionRelation = 450,
        FUN_IsLastIdlePlayed = 451,
        FUN_GetPlayerTeammate = 454,
        FUN_GetPlayerTeammateCount = 455,
        FUN_GetActorCrimePlayerEnemy = 459,
        FUN_GetActorFactionPlayerEnemy = 460,
        FUN_IsPlayerTagSkill = 462,
        FUN_IsPlayerGrabbedRef = 464,
        FUN_GetDestructionStage = 471,
        FUN_GetIsAlignment   = 474,
        FUN_GetThreatRatio   = 478,
        FUN_GetIsUsedItemEquipType = 480,
        FUN_GetConcussed     = 489,
        FUN_GetMapMarkerVisible = 492,
        FUN_GetPermanentActorValue = 495,
        FUN_GetKillingBlowLimb = 496,
        FUN_GetWeaponHealthPerc = 500,
        FUN_GetRadiationLevel = 503,
        FUN_GetLastHitCritical = 510,
        FUN_IsCombatTarget   = 515,
        FUN_GetVATSRightAreaFree = 518,
        FUN_GetVATSLeftAreaFree = 519,
        FUN_GetVATSBackAreaFree = 520,
        FUN_GetVATSFrontAreaFree = 521,
        FUN_GetIsLockBroken  = 522,
        FUN_IsPS3            = 523,
        FUN_IsWin32          = 524,
        FUN_GetVATSRightTargetVisible = 525,
        FUN_GetVATSLeftTargetVisible = 526,
        FUN_GetVATSBackTargetVisible = 527,
        FUN_GetVATSFrontTargetVisible = 528,
        FUN_IsInCriticalStage = 531,
        FUN_GetXPForNextLevel = 533,
        FUN_GetQuestCompleted = 546,
        FUN_IsGoreDisabled   = 550,
        FUN_GetSpellUsageNum = 555,
        FUN_GetActorsInHigh  = 557,
        FUN_HasLoaded3D      = 558,
        FUN_GetReputation    = 573,
        FUN_GetReputationPct = 574,
        FUN_GetReputationThreshold = 575,
        FUN_IsHardcore       = 586,
        FUN_GetForceHitReaction = 601,
        FUN_ChallengeLocked  = 607,
        FUN_GetCasinoWinningStage = 610,
        FUN_PlayerInRegion   = 612,
        FUN_GetChallengeCompleted = 614,
        FUN_IsAlwaysHardcore = 619
    };

#pragma pack(push, 1)
    struct TargetResponseData
    {
        std::uint32_t emoType; // EmotionType
        std::int32_t  emoValue;
        std::uint32_t unknown1;
        std::uint32_t responseNo; // 1 byte + padding
        // below FO3/FONV
        FormId sound; // when 20 bytes usually 0 but there are exceptions (FO3 INFO FormId = 0x0002241f)
        std::uint32_t flags; // 1 byte + padding (0x01 = use emotion anim)
    };

    struct TargetCondition
    {
        std::uint32_t condition; // ConditionTypeAndFlag + padding
        float comparison; // WARN: can be GLOB FormId if flag set
        std::uint32_t functionIndex;
        std::uint32_t param1; // FIXME: if formid needs modindex adjustment or not?
        std::uint32_t param2;
        std::uint32_t runOn; // 0 subject, 1 target, 2 reference, 3 combat target, 4 linked reference
        // below FO3/FONV/TES5
        FormId reference;
    };

    struct ScriptHeader
    {
        std::uint32_t unused;
        std::uint32_t refCount;
        std::uint32_t compiledSize;
        std::uint32_t variableCount;
        std::uint16_t type; // 0 object, 1 quest, 0x100 effect
        std::uint16_t flag; // 0x01 enabled
    };
#pragma pack(pop)

    struct ScriptLocalVariableData
    {
        // SLSD
        std::uint32_t index;
        std::uint32_t unknown1;
        std::uint32_t unknown2;
        std::uint32_t unknown3;
        std::uint32_t type;
        std::uint32_t unknown4;
        // SCVR
        std::string variableName;

        void clear() {
            index = 0;
            type = 0;
            variableName.clear();
        }
    };

    struct ScriptDefinition
    {
        ScriptHeader scriptHeader;
        // SDCA compiled source
        std::string scriptSource;
        std::vector<ScriptLocalVariableData> localVarData;
        std::vector<std::uint32_t> localRefVarIndex;
        FormId globReference;
    };
}

#endif // ESM4_SCRIPT_H
