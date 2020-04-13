#ifndef COMPILER_OPCODES_H
#define COMPILER_OPCODES_H

namespace Compiler
{
    namespace Ai
    {
        const int opcodeAiTravel = 0x20000;
        const int opcodeAiTravelExplicit = 0x20001;
        const int opcodeAiEscort = 0x20002;
        const int opcodeAiEscortExplicit = 0x20003;
        const int opcodeGetAiPackageDone = 0x200007c;
        const int opcodeGetAiPackageDoneExplicit = 0x200007d;
        const int opcodeGetCurrentAiPackage = 0x20001ef;
        const int opcodeGetCurrentAiPackageExplicit = 0x20001f0;
        const int opcodeGetDetected = 0x20001f1;
        const int opcodeGetDetectedExplicit = 0x20001f2;
        const int opcodeAiWander = 0x20010;
        const int opcodeAiWanderExplicit = 0x20011;
        const int opcodeAIActivate = 0x2001e;
        const int opcodeAIActivateExplicit = 0x2001f;
        const int opcodeAiEscortCell = 0x20020;
        const int opcodeAiEscortCellExplicit = 0x20021;
        const int opcodeAiFollow = 0x20022;
        const int opcodeAiFollowExplicit = 0x20023;
        const int opcodeAiFollowCell = 0x20024;
        const int opcodeAiFollowCellExplicit = 0x20025;
        const int opcodeSetHello = 0x200015c;
        const int opcodeSetHelloExplicit = 0x200015d;
        const int opcodeSetFight = 0x200015e;
        const int opcodeSetFightExplicit = 0x200015f;
        const int opcodeSetFlee = 0x2000160;
        const int opcodeSetFleeExplicit = 0x2000161;
        const int opcodeSetAlarm = 0x2000162;
        const int opcodeSetAlarmExplicit = 0x2000163;
        const int opcodeModHello = 0x20001b7;
        const int opcodeModHelloExplicit = 0x20001b8;
        const int opcodeModFight = 0x20001b9;
        const int opcodeModFightExplicit = 0x20001ba;
        const int opcodeModFlee = 0x20001bb;
        const int opcodeModFleeExplicit = 0x20001bc;
        const int opcodeModAlarm = 0x20001bd;
        const int opcodeModAlarmExplicit = 0x20001be;
        const int opcodeGetHello = 0x20001bf;
        const int opcodeGetHelloExplicit = 0x20001c0;
        const int opcodeGetFight = 0x20001c1;
        const int opcodeGetFightExplicit = 0x20001c2;
        const int opcodeGetFlee = 0x20001c3;
        const int opcodeGetFleeExplicit = 0x20001c4;
        const int opcodeGetAlarm = 0x20001c5;
        const int opcodeGetAlarmExplicit = 0x20001c6;
        const int opcodeGetLineOfSight = 0x2000222;
        const int opcodeGetLineOfSightExplicit = 0x2000223;
        const int opcodeToggleAI = 0x2000224;
        const int opcodeGetTarget = 0x2000238;
        const int opcodeGetTargetExplicit = 0x2000239;
        const int opcodeStartCombat = 0x200023a;
        const int opcodeStartCombatExplicit = 0x200023b;
        const int opcodeStopCombat = 0x200023c;
        const int opcodeStopCombatExplicit = 0x200023d;
        const int opcodeFace = 0x200024c;
        const int opcodeFaceExplicit = 0x200024d;
    }

    namespace Animation
    {
        const int opcodeSkipAnim = 0x2000138;
        const int opcodeSkipAnimExplicit = 0x2000139;
        const int opcodePlayAnim = 0x20006;
        const int opcodePlayAnimExplicit = 0x20007;
        const int opcodeLoopAnim = 0x20008;
        const int opcodeLoopAnimExplicit = 0x20009;
    }

    namespace Cell
    {
        const int opcodeCellChanged = 0x2000000;
        const int opcodeTestCells = 0x200030e;
        const int opcodeTestInteriorCells = 0x200030f;
        const int opcodeCOC = 0x2000026;
        const int opcodeCOE = 0x2000226;
        const int opcodeGetInterior = 0x2000131;
        const int opcodeGetPCCell = 0x2000136;
        const int opcodeGetWaterLevel = 0x2000141;
        const int opcodeSetWaterLevel = 0x2000142;
        const int opcodeModWaterLevel = 0x2000143;
    }

    namespace Console
    {

    }

    namespace Container
    {
        const int opcodeAddItem = 0x2000076;
        const int opcodeAddItemExplicit = 0x2000077;
        const int opcodeGetItemCount = 0x2000078;
        const int opcodeGetItemCountExplicit = 0x2000079;
        const int opcodeRemoveItem = 0x200007a;
        const int opcodeRemoveItemExplicit = 0x200007b;
        const int opcodeEquip = 0x20001b3;
        const int opcodeEquipExplicit = 0x20001b4;
        const int opcodeGetArmorType = 0x20001d1;
        const int opcodeGetArmorTypeExplicit = 0x20001d2;
        const int opcodeHasItemEquipped = 0x20001d5;
        const int opcodeHasItemEquippedExplicit = 0x20001d6;
        const int opcodeHasSoulGem = 0x20001de;
        const int opcodeHasSoulGemExplicit = 0x20001df;
        const int opcodeGetWeaponType = 0x20001e0;
        const int opcodeGetWeaponTypeExplicit = 0x20001e1;
    }

    namespace Control
    {
        const int numberOfControls = 7;

        extern const char *controls[numberOfControls];

        const int opcodeEnable = 0x200007e;
        const int opcodeDisable = 0x2000085;
        const int opcodeToggleCollision = 0x2000130;
        const int opcodeClearForceRun = 0x2000154;
        const int opcodeClearForceRunExplicit = 0x2000155;
        const int opcodeForceRun = 0x2000156;
        const int opcodeForceRunExplicit = 0x2000157;
        const int opcodeClearForceJump = 0x2000258;
        const int opcodeClearForceJumpExplicit = 0x2000259;
        const int opcodeForceJump = 0x200025a;
        const int opcodeForceJumpExplicit = 0x200025b;
        const int opcodeClearForceMoveJump = 0x200025c;
        const int opcodeClearForceMoveJumpExplicit = 0x200025d;
        const int opcodeForceMoveJump = 0x200025e;
        const int opcodeForceMoveJumpExplicit = 0x200025f;
        const int opcodeClearForceSneak = 0x2000158;
        const int opcodeClearForceSneakExplicit = 0x2000159;
        const int opcodeForceSneak = 0x200015a;
        const int opcodeForceSneakExplicit = 0x200015b;
        const int opcodeGetDisabled = 0x2000175;
        const int opcodeGetPcRunning = 0x20001c9;
        const int opcodeGetPcSneaking = 0x20001ca;
        const int opcodeGetForceRun = 0x20001cb;
        const int opcodeGetForceSneak = 0x20001cc;
        const int opcodeGetForceRunExplicit = 0x20001cd;
        const int opcodeGetForceSneakExplicit = 0x20001ce;
        const int opcodeGetForceJump = 0x2000260;
        const int opcodeGetForceMoveJump = 0x2000262;
        const int opcodeGetForceJumpExplicit = 0x2000261;
        const int opcodeGetForceMoveJumpExplicit = 0x2000263;
    }

    namespace Dialogue
    {
        const int opcodeJournal = 0x2000133;
        const int opcodeJournalExplicit = 0x200030b;
        const int opcodeSetJournalIndex = 0x2000134;
        const int opcodeGetJournalIndex = 0x2000135;
        const int opcodeAddTopic = 0x200013a;
        const int opcodeChoice = 0x2000a;
        const int opcodeForceGreeting = 0x200014f;
        const int opcodeForceGreetingExplicit = 0x2000150;
        const int opcodeGoodbye = 0x2000152;
        const int opcodeSetReputation = 0x20001ad;
        const int opcodeModReputation = 0x20001ae;
        const int opcodeSetReputationExplicit = 0x20001af;
        const int opcodeModReputationExplicit = 0x20001b0;
        const int opcodeGetReputation = 0x20001b1;
        const int opcodeGetReputationExplicit = 0x20001b2;
        const int opcodeSameFaction = 0x20001b5;
        const int opcodeSameFactionExplicit = 0x20001b6;
        const int opcodeModFactionReaction = 0x2000242;
        const int opcodeSetFactionReaction = 0x20002ff;
        const int opcodeGetFactionReaction = 0x2000243;
        const int opcodeClearInfoActor = 0x2000245;
        const int opcodeClearInfoActorExplicit = 0x2000246;
    }

    namespace Gui
    {
        const int opcodeEnableBirthMenu = 0x200000e;
        const int opcodeEnableClassMenu = 0x200000f;
        const int opcodeEnableNameMenu = 0x2000010;
        const int opcodeEnableRaceMenu = 0x2000011;
        const int opcodeEnableStatsReviewMenu = 0x2000012;
        const int opcodeEnableInventoryMenu = 0x2000013;
        const int opcodeEnableMagicMenu = 0x2000014;
        const int opcodeEnableMapMenu = 0x2000015;
        const int opcodeEnableStatsMenu = 0x2000016;
        const int opcodeEnableRest = 0x2000017;
        const int opcodeEnableLevelupMenu = 0x2000300;
        const int opcodeShowRestMenu = 0x2000018;
        const int opcodeShowRestMenuExplicit = 0x2000234;
        const int opcodeGetButtonPressed = 0x2000137;
        const int opcodeToggleFogOfWar = 0x2000145;
        const int opcodeToggleFullHelp = 0x2000151;
        const int opcodeShowMap = 0x20001a0;
        const int opcodeFillMap = 0x20001a1;
        const int opcodeMenuTest = 0x2002c;
        const int opcodeToggleMenus = 0x200024b;
    }

    namespace Misc
    {
        const int opcodeXBox = 0x200000c;
        const int opcodeOnActivate = 0x200000d;
        const int opcodeOnActivateExplicit = 0x2000306;
        const int opcodeActivate = 0x2000075;
        const int opcodeActivateExplicit = 0x2000244;
        const int opcodeLock = 0x20004;
        const int opcodeLockExplicit = 0x20005;
        const int opcodeUnlock = 0x200008c;
        const int opcodeUnlockExplicit = 0x200008d;
        const int opcodeToggleCollisionDebug = 0x2000132;
        const int opcodeToggleCollisionBoxes = 0x20001ac;
        const int opcodeToggleWireframe = 0x200013b;
        const int opcodeFadeIn = 0x200013c;
        const int opcodeFadeOut = 0x200013d;
        const int opcodeFadeTo = 0x200013e;
        const int opcodeToggleWater = 0x2000144;
        const int opcodeToggleWorld = 0x20002f5;
        const int opcodeTogglePathgrid = 0x2000146;
        const int opcodeDontSaveObject = 0x2000153;
        const int opcodePcForce1stPerson = 0x20002f6;
        const int opcodePcForce3rdPerson = 0x20002f7;
        const int opcodePcGet3rdPerson = 0x20002f8;
        const int opcodeToggleVanityMode = 0x2000174;
        const int opcodeGetPcSleep = 0x200019f;
        const int opcodeGetPcJumping = 0x2000233;
        const int opcodeWakeUpPc = 0x20001a2;
        const int opcodeGetLocked = 0x20001c7;
        const int opcodeGetLockedExplicit = 0x20001c8;
        const int opcodeGetEffect = 0x20001cf;
        const int opcodeGetEffectExplicit = 0x20001d0;
        const int opcodeBetaComment = 0x2002d;
        const int opcodeBetaCommentExplicit = 0x2002e;
        const int opcodeAddSoulGem = 0x20001f3;
        const int opcodeAddSoulGemExplicit = 0x20001f4;
        const int opcodeRemoveSoulGem = 0x20027;
        const int opcodeRemoveSoulGemExplicit = 0x20028;
        const int opcodeDrop = 0x20001f8;
        const int opcodeDropExplicit = 0x20001f9;
        const int opcodeDropSoulGem = 0x20001fa;
        const int opcodeDropSoulGemExplicit = 0x20001fb;
        const int opcodeGetAttacked = 0x20001d3;
        const int opcodeGetAttackedExplicit = 0x20001d4;
        const int opcodeGetWeaponDrawn = 0x20001d7;
        const int opcodeGetWeaponDrawnExplicit = 0x20001d8;
        const int opcodeGetSpellReadied = 0x2000231;
        const int opcodeGetSpellReadiedExplicit = 0x2000232;
        const int opcodeGetSpellEffects = 0x20001db;
        const int opcodeGetSpellEffectsExplicit = 0x20001dc;
        const int opcodeGetCurrentTime = 0x20001dd;
        const int opcodeSetDelete = 0x20001e5;
        const int opcodeSetDeleteExplicit = 0x20001e6;
        const int opcodeGetSquareRoot = 0x20001e7;
        const int opcodeFall = 0x200020a;
        const int opcodeFallExplicit = 0x200020b;
        const int opcodeGetStandingPc = 0x200020c;
        const int opcodeGetStandingPcExplicit = 0x200020d;
        const int opcodeGetStandingActor = 0x200020e;
        const int opcodeGetStandingActorExplicit = 0x200020f;
        const int opcodeGetCollidingPc = 0x2000250;
        const int opcodeGetCollidingPcExplicit = 0x2000251;
        const int opcodeGetCollidingActor = 0x2000252;
        const int opcodeGetCollidingActorExplicit = 0x2000253;
        const int opcodeHurtStandingActor = 0x2000254;
        const int opcodeHurtStandingActorExplicit = 0x2000255;
        const int opcodeHurtCollidingActor = 0x2000256;
        const int opcodeHurtCollidingActorExplicit = 0x2000257;
        const int opcodeGetWindSpeed = 0x2000212;
        const int opcodePlayBink = 0x20001f7;
        const int opcodeGoToJail = 0x2000235;
        const int opcodePayFine = 0x2000236;
        const int opcodePayFineThief = 0x2000237;
        const int opcodeHitOnMe = 0x2000213;
        const int opcodeHitOnMeExplicit = 0x2000214;
        const int opcodeHitAttemptOnMe = 0x20002f9;
        const int opcodeHitAttemptOnMeExplicit = 0x20002fa;
        const int opcodeDisableTeleporting = 0x2000215;
        const int opcodeEnableTeleporting = 0x2000216;
        const int opcodeShowVars = 0x200021d;
        const int opcodeShowVarsExplicit = 0x200021e;
        const int opcodeShow = 0x2000304;
        const int opcodeShowExplicit = 0x2000305;
        const int opcodeToggleGodMode = 0x200021f;
        const int opcodeToggleScripts = 0x2000301;
        const int opcodeDisableLevitation = 0x2000220;
        const int opcodeEnableLevitation = 0x2000221;
        const int opcodeCast = 0x2000227;
        const int opcodeCastExplicit = 0x2000228;
        const int opcodeExplodeSpell = 0x2000229;
        const int opcodeExplodeSpellExplicit = 0x200022a;
        const int opcodeGetPcInJail = 0x200023e;
        const int opcodeGetPcTraveling = 0x200023f;
        const int opcodeAddToLevCreature = 0x20002fb;
        const int opcodeRemoveFromLevCreature = 0x20002fc;
        const int opcodeAddToLevItem = 0x20002fd;
        const int opcodeRemoveFromLevItem = 0x20002fe;
        const int opcodeShowSceneGraph = 0x2002f;
        const int opcodeShowSceneGraphExplicit = 0x20030;
        const int opcodeToggleBorders = 0x2000307;
        const int opcodeToggleNavMesh = 0x2000308;
        const int opcodeToggleActorsPaths = 0x2000309;
        const int opcodeSetNavMeshNumberToRender = 0x200030a;
        const int opcodeRepairedOnMe = 0x200030c;
        const int opcodeRepairedOnMeExplicit = 0x200030d;
        const int opcodeToggleRecastMesh = 0x2000310;
    }

    namespace Sky
    {
        const int opcodeToggleSky = 0x2000021;
        const int opcodeTurnMoonWhite = 0x2000022;
        const int opcodeTurnMoonRed = 0x2000023;
        const int opcodeGetMasserPhase = 0x2000024;
        const int opcodeGetSecundaPhase = 0x2000025;
        const int opcodeGetCurrentWeather = 0x200013f;
        const int opcodeChangeWeather = 0x2000140;
        const int opcodeModRegion = 0x20026;
    }

    namespace Sound
    {
        const int opcodeSay = 0x2000001;
        const int opcodeSayDone = 0x2000002;
        const int opcodeStreamMusic = 0x2000003;
        const int opcodePlaySound = 0x2000004;
        const int opcodePlaySoundVP = 0x2000005;
        const int opcodePlaySound3D = 0x2000006;
        const int opcodePlaySound3DVP = 0x2000007;
        const int opcodePlayLoopSound3D = 0x2000008;
        const int opcodePlayLoopSound3DVP = 0x2000009;
        const int opcodeStopSound = 0x200000a;
        const int opcodeGetSoundPlaying = 0x200000b;

        const int opcodeSayExplicit = 0x2000019;
        const int opcodeSayDoneExplicit = 0x200001a;
        const int opcodePlaySound3DExplicit = 0x200001b;
        const int opcodePlaySound3DVPExplicit = 0x200001c;
        const int opcodePlayLoopSound3DExplicit = 0x200001d;
        const int opcodePlayLoopSound3DVPExplicit = 0x200001e;
        const int opcodeStopSoundExplicit = 0x200001f;
        const int opcodeGetSoundPlayingExplicit = 0x2000020;
    }

    namespace Stats
    {
        const int numberOfAttributes = 8;
        const int numberOfDynamics = 3;
        const int numberOfSkills = 27;

        const int numberOfMagicEffects = 24;

        const int opcodeGetAttribute = 0x2000027;
        const int opcodeGetAttributeExplicit = 0x200002f;
        const int opcodeSetAttribute = 0x2000037;
        const int opcodeSetAttributeExplicit = 0x200003f;
        const int opcodeModAttribute = 0x2000047;
        const int opcodeModAttributeExplicit = 0x200004f;

        const int opcodeGetDynamic = 0x2000057;
        const int opcodeGetDynamicExplicit = 0x200005a;
        const int opcodeSetDynamic = 0x200005d;
        const int opcodeSetDynamicExplicit = 0x2000060;
        const int opcodeModDynamic = 0x2000063;
        const int opcodeModDynamicExplicit = 0x2000066;
        const int opcodeModCurrentDynamic = 0x2000069;
        const int opcodeModCurrentDynamicExplicit = 0x200006c;
        const int opcodeGetDynamicGetRatio = 0x200006f;
        const int opcodeGetDynamicGetRatioExplicit = 0x2000072;

        const int opcodeGetSkill = 0x200008e;
        const int opcodeGetSkillExplicit = 0x20000a9;
        const int opcodeSetSkill = 0x20000c4;
        const int opcodeSetSkillExplicit = 0x20000df;
        const int opcodeModSkill = 0x20000fa;
        const int opcodeModSkillExplicit = 0x2000115;

        const int opcodeGetMagicEffect = 0x2000264;
        const int opcodeGetMagicEffectExplicit = 0x200027c;
        const int opcodeSetMagicEffect = 0x2000294;
        const int opcodeSetMagicEffectExplicit = 0x20002ac;
        const int opcodeModMagicEffect = 0x20002c4;
        const int opcodeModMagicEffectExplicit = 0x20002dc;

        const int opcodeGetPCCrimeLevel = 0x20001ec;
        const int opcodeSetPCCrimeLevel = 0x20001ed;
        const int opcodeModPCCrimeLevel = 0x20001ee;

        const int opcodeAddSpell = 0x2000147;
        const int opcodeAddSpellExplicit = 0x2000148;
        const int opcodeRemoveSpell = 0x2000149;
        const int opcodeRemoveSpellExplicit = 0x200014a;
        const int opcodeGetSpell = 0x200014b;
        const int opcodeGetSpellExplicit = 0x200014c;

        const int opcodePCRaiseRank = 0x2000b;
        const int opcodePCLowerRank = 0x2000c;
        const int opcodePCJoinFaction = 0x2000d;
        const int opcodePCRaiseRankExplicit = 0x20029;
        const int opcodePCLowerRankExplicit = 0x2002a;
        const int opcodePCJoinFactionExplicit = 0x2002b;

        const int opcodeGetPCRank = 0x2000e;
        const int opcodeGetPCRankExplicit = 0x2000f;
        const int opcodeModDisposition = 0x200014d;
        const int opcodeModDispositionExplicit = 0x200014e;
        const int opcodeSetDisposition = 0x20001a4;
        const int opcodeSetDispositionExplicit = 0x20001a5;
        const int opcodeGetDisposition = 0x20001a6;
        const int opcodeGetDispositionExplicit = 0x20001a7;

        const int opcodeGetLevel = 0x200018c;
        const int opcodeGetLevelExplicit = 0x200018d;
        const int opcodeSetLevel = 0x200018e;
        const int opcodeSetLevelExplicit = 0x200018f;

        const int opcodeGetDeadCount = 0x20001a3;

        const int opcodeGetPCFacRep = 0x20012;
        const int opcodeGetPCFacRepExplicit = 0x20013;
        const int opcodeSetPCFacRep = 0x20014;
        const int opcodeSetPCFacRepExplicit = 0x20015;
        const int opcodeModPCFacRep = 0x20016;
        const int opcodeModPCFacRepExplicit = 0x20017;

        const int opcodeGetCommonDisease = 0x20001a8;
        const int opcodeGetCommonDiseaseExplicit = 0x20001a9;
        const int opcodeGetBlightDisease = 0x20001aa;
        const int opcodeGetBlightDiseaseExplicit = 0x20001ab;

        const int opcodeGetRace = 0x20001d9;
        const int opcodeGetRaceExplicit = 0x20001da;

        const int opcodePcExpelled = 0x20018;
        const int opcodePcExpelledExplicit = 0x20019;
        const int opcodePcExpell = 0x2001a;
        const int opcodePcExpellExplicit = 0x2001b;
        const int opcodePcClearExpelled = 0x2001c;
        const int opcodePcClearExpelledExplicit = 0x2001d;
        const int opcodeRaiseRank = 0x20001e8;
        const int opcodeRaiseRankExplicit = 0x20001e9;
        const int opcodeLowerRank = 0x20001ea;
        const int opcodeLowerRankExplicit = 0x20001eb;
        const int opcodeOnDeath = 0x20001fc;
        const int opcodeOnDeathExplicit = 0x2000205;
        const int opcodeOnMurder = 0x2000249;
        const int opcodeOnMurderExplicit = 0x200024a;
        const int opcodeOnKnockout = 0x2000240;
        const int opcodeOnKnockoutExplicit = 0x2000241;

        const int opcodeBecomeWerewolf = 0x2000217;
        const int opcodeBecomeWerewolfExplicit = 0x2000218;
        const int opcodeUndoWerewolf = 0x2000219;
        const int opcodeUndoWerewolfExplicit = 0x200021a;
        const int opcodeSetWerewolfAcrobatics = 0x200021b;
        const int opcodeSetWerewolfAcrobaticsExplicit = 0x200021c;
        const int opcodeIsWerewolf = 0x20001fd;
        const int opcodeIsWerewolfExplicit = 0x20001fe;

        const int opcodeGetWerewolfKills = 0x20001e2;

        const int opcodeRemoveSpellEffects = 0x200022b;
        const int opcodeRemoveSpellEffectsExplicit = 0x200022c;
        const int opcodeRemoveEffects = 0x200022d;
        const int opcodeRemoveEffectsExplicit = 0x200022e;
        const int opcodeResurrect = 0x200022f;
        const int opcodeResurrectExplicit = 0x2000230;

        const int opcodeGetStat = 0x200024e;
        const int opcodeGetStatExplicit = 0x200024f;
    }

    namespace Transformation
    {
        const int opcodeSetScale = 0x2000164;
        const int opcodeSetScaleExplicit = 0x2000165;
        const int opcodeSetAngle = 0x2000166;
        const int opcodeSetAngleExplicit = 0x2000167;
        const int opcodeGetScale = 0x2000168;
        const int opcodeGetScaleExplicit = 0x2000169;
        const int opcodeGetAngle = 0x200016a;
        const int opcodeGetAngleExplicit = 0x200016b;
        const int opcodeGetPos = 0x2000190;
        const int opcodeGetPosExplicit = 0x2000191;
        const int opcodeSetPos = 0x2000192;
        const int opcodeSetPosExplicit = 0x2000193;
        const int opcodeGetStartingPos = 0x2000194;
        const int opcodeGetStartingPosExplicit = 0x2000195;
        const int opcodeGetStartingAngle = 0x2000210;
        const int opcodeGetStartingAngleExplicit = 0x2000211;
        const int opcodePosition = 0x2000196;
        const int opcodePositionExplicit = 0x2000197;
        const int opcodePositionCell = 0x2000198;
        const int opcodePositionCellExplicit = 0x2000199;

        const int opcodePlaceItemCell = 0x200019a;
        const int opcodePlaceItem = 0x200019b;
        const int opcodePlaceAtPc = 0x200019c;
        const int opcodePlaceAtMe = 0x200019d;
        const int opcodePlaceAtMeExplicit = 0x200019e;
        const int opcodeModScale = 0x20001e3;
        const int opcodeModScaleExplicit = 0x20001e4;
        const int opcodeRotate = 0x20001ff;
        const int opcodeRotateExplicit = 0x2000200;
        const int opcodeRotateWorld = 0x2000201;
        const int opcodeRotateWorldExplicit = 0x2000202;
        const int opcodeSetAtStart = 0x2000203;
        const int opcodeSetAtStartExplicit = 0x2000204;
        const int opcodeMove = 0x2000206;
        const int opcodeMoveExplicit = 0x2000207;
        const int opcodeMoveWorld = 0x2000208;
        const int opcodeMoveWorldExplicit = 0x2000209;
        const int opcodeResetActors = 0x20002f4;
        const int opcodeFixme = 0x2000302;
    }

    namespace User
    {
        const int opcodeUser1 = 0x200016c;
        const int opcodeUser2 = 0x200016d;
        const int opcodeUser3 = 0x200016e;
        const int opcodeUser3Explicit = 0x200016f;
        const int opcodeUser4 = 0x2000170;
        const int opcodeUser4Explicit = 0x2000171;
    }
}

#endif
