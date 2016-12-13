#include "defaultgmsts.hpp"

#include <limits>

const float FInf = std::numeric_limits<float>::infinity();
const float FEps = std::numeric_limits<float>::epsilon();

const int IMax = std::numeric_limits<int>::max();
const int IMin = std::numeric_limits<int>::min();

const char* CSMWorld::DefaultGmsts::Floats[CSMWorld::DefaultGmsts::FloatCount] =
{
    "fAIFleeFleeMult",
    "fAIFleeHealthMult",
    "fAIMagicSpellMult",
    "fAIMeleeArmorMult",
    "fAIMeleeSummWeaponMult",
    "fAIMeleeWeaponMult",
    "fAIRangeMagicSpellMult",
    "fAIRangeMeleeWeaponMult",
    "fAlarmRadius",
    "fAthleticsRunBonus",
    "fAudioDefaultMaxDistance",
    "fAudioDefaultMinDistance",
    "fAudioMaxDistanceMult",
    "fAudioMinDistanceMult",
    "fAudioVoiceDefaultMaxDistance",
    "fAudioVoiceDefaultMinDistance",
    "fAutoPCSpellChance",
    "fAutoSpellChance",
    "fBargainOfferBase",
    "fBargainOfferMulti",
    "fBarterGoldResetDelay",
    "fBaseRunMultiplier",
    "fBlockStillBonus",
    "fBribe1000Mod",
    "fBribe100Mod",
    "fBribe10Mod",
    "fCombatAngleXY",
    "fCombatAngleZ",
    "fCombatArmorMinMult",
    "fCombatBlockLeftAngle",
    "fCombatBlockRightAngle",
    "fCombatCriticalStrikeMult",
    "fCombatDelayCreature",
    "fCombatDelayNPC",
    "fCombatDistance",
    "fCombatDistanceWerewolfMod",
    "fCombatForceSideAngle",
    "fCombatInvisoMult",
    "fCombatKODamageMult",
    "fCombatTorsoSideAngle",
    "fCombatTorsoStartPercent",
    "fCombatTorsoStopPercent",
    "fConstantEffectMult",
    "fCorpseClearDelay",
    "fCorpseRespawnDelay",
    "fCrimeGoldDiscountMult",
    "fCrimeGoldTurnInMult",
    "fCrimeStealing",
    "fDamageStrengthBase",
    "fDamageStrengthMult",
    "fDifficultyMult",
    "fDiseaseXferChance",
    "fDispAttacking",
    "fDispBargainFailMod",
    "fDispBargainSuccessMod",
    "fDispCrimeMod",
    "fDispDiseaseMod",
    "fDispFactionMod",
    "fDispFactionRankBase",
    "fDispFactionRankMult",
    "fDispositionMod",
    "fDispPersonalityBase",
    "fDispPersonalityMult",
    "fDispPickPocketMod",
    "fDispRaceMod",
    "fDispStealing",
    "fDispWeaponDrawn",
    "fEffectCostMult",
    "fElementalShieldMult",
    "fEnchantmentChanceMult",
    "fEnchantmentConstantChanceMult",
    "fEnchantmentConstantDurationMult",
    "fEnchantmentMult",
    "fEnchantmentValueMult",
    "fEncumberedMoveEffect",
    "fEncumbranceStrMult",
    "fEndFatigueMult",
    "fFallAcroBase",
    "fFallAcroMult",
    "fFallDamageDistanceMin",
    "fFallDistanceBase",
    "fFallDistanceMult",
    "fFatigueAttackBase",
    "fFatigueAttackMult",
    "fFatigueBase",
    "fFatigueBlockBase",
    "fFatigueBlockMult",
    "fFatigueJumpBase",
    "fFatigueJumpMult",
    "fFatigueMult",
    "fFatigueReturnBase",
    "fFatigueReturnMult",
    "fFatigueRunBase",
    "fFatigueRunMult",
    "fFatigueSneakBase",
    "fFatigueSneakMult",
    "fFatigueSpellBase",
    "fFatigueSpellCostMult",
    "fFatigueSpellMult",
    "fFatigueSwimRunBase",
    "fFatigueSwimRunMult",
    "fFatigueSwimWalkBase",
    "fFatigueSwimWalkMult",
    "fFightDispMult",
    "fFightDistanceMultiplier",
    "fFightStealing",
    "fFleeDistance",
    "fGreetDistanceReset",
    "fHandtoHandHealthPer",
    "fHandToHandReach",
    "fHoldBreathEndMult",
    "fHoldBreathTime",
    "fIdleChanceMultiplier",
    "fIngredientMult",
    "fInteriorHeadTrackMult",
    "fJumpAcrobaticsBase",
    "fJumpAcroMultiplier",
    "fJumpEncumbranceBase",
    "fJumpEncumbranceMultiplier",
    "fJumpMoveBase",
    "fJumpMoveMult",
    "fJumpRunMultiplier",
    "fKnockDownMult",
    "fLevelMod",
    "fLevelUpHealthEndMult",
    "fLightMaxMod",
    "fLuckMod",
    "fMagesGuildTravel",
    "fMagicCreatureCastDelay",
    "fMagicDetectRefreshRate",
    "fMagicItemConstantMult",
    "fMagicItemCostMult",
    "fMagicItemOnceMult",
    "fMagicItemPriceMult",
    "fMagicItemRechargePerSecond",
    "fMagicItemStrikeMult",
    "fMagicItemUsedMult",
    "fMagicStartIconBlink",
    "fMagicSunBlockedMult",
    "fMajorSkillBonus",
    "fMaxFlySpeed",
    "fMaxHandToHandMult",
    "fMaxHeadTrackDistance",
    "fMaxWalkSpeed",
    "fMaxWalkSpeedCreature",
    "fMedMaxMod",
    "fMessageTimePerChar",
    "fMinFlySpeed",
    "fMinHandToHandMult",
    "fMinorSkillBonus",
    "fMinWalkSpeed",
    "fMinWalkSpeedCreature",
    "fMiscSkillBonus",
    "fNPCbaseMagickaMult",
    "fNPCHealthBarFade",
    "fNPCHealthBarTime",
    "fPCbaseMagickaMult",
    "fPerDieRollMult",
    "fPersonalityMod",
    "fPerTempMult",
    "fPickLockMult",
    "fPickPocketMod",
    "fPotionMinUsefulDuration",
    "fPotionStrengthMult",
    "fPotionT1DurMult",
    "fPotionT1MagMult",
    "fPotionT4BaseStrengthMult",
    "fPotionT4EquipStrengthMult",
    "fProjectileMaxSpeed",
    "fProjectileMinSpeed",
    "fProjectileThrownStoreChance",
    "fRepairAmountMult",
    "fRepairMult",
    "fReputationMod",
    "fRestMagicMult",
    "fSeriousWoundMult",
    "fSleepRandMod",
    "fSleepRestMod",
    "fSneakBootMult",
    "fSneakDistanceBase",
    "fSneakDistanceMultiplier",
    "fSneakNoViewMult",
    "fSneakSkillMult",
    "fSneakSpeedMultiplier",
    "fSneakUseDelay",
    "fSneakUseDist",
    "fSneakViewMult",
    "fSoulGemMult",
    "fSpecialSkillBonus",
    "fSpellMakingValueMult",
    "fSpellPriceMult",
    "fSpellValueMult",
    "fStromWalkMult",
    "fStromWindSpeed",
    "fSuffocationDamage",
    "fSwimHeightScale",
    "fSwimRunAthleticsMult",
    "fSwimRunBase",
    "fSwimWalkAthleticsMult",
    "fSwimWalkBase",
    "fSwingBlockBase",
    "fSwingBlockMult",
    "fTargetSpellMaxSpeed",
    "fThrownWeaponMaxSpeed",
    "fThrownWeaponMinSpeed",
    "fTrapCostMult",
    "fTravelMult",
    "fTravelTimeMult",
    "fUnarmoredBase1",
    "fUnarmoredBase2",
    "fVanityDelay",
    "fVoiceIdleOdds",
    "fWaterReflectUpdateAlways",
    "fWaterReflectUpdateSeldom",
    "fWeaponDamageMult",
    "fWeaponFatigueBlockMult",
    "fWeaponFatigueMult",
    "fWereWolfAcrobatics",
    "fWereWolfAgility",
    "fWereWolfAlchemy",
    "fWereWolfAlteration",
    "fWereWolfArmorer",
    "fWereWolfAthletics",
    "fWereWolfAxe",
    "fWereWolfBlock",
    "fWereWolfBluntWeapon",
    "fWereWolfConjuration",
    "fWereWolfDestruction",
    "fWereWolfEnchant",
    "fWereWolfEndurance",
    "fWereWolfFatigue",
    "fWereWolfHandtoHand",
    "fWereWolfHealth",
    "fWereWolfHeavyArmor",
    "fWereWolfIllusion",
    "fWereWolfIntellegence",
    "fWereWolfLightArmor",
    "fWereWolfLongBlade",
    "fWereWolfLuck",
    "fWereWolfMagicka",
    "fWereWolfMarksman",
    "fWereWolfMediumArmor",
    "fWereWolfMerchantile",
    "fWereWolfMysticism",
    "fWereWolfPersonality",
    "fWereWolfRestoration",
    "fWereWolfRunMult",
    "fWereWolfSecurity",
    "fWereWolfShortBlade",
    "fWereWolfSilverWeaponDamageMult",
    "fWereWolfSneak",
    "fWereWolfSpear",
    "fWereWolfSpeechcraft",
    "fWereWolfSpeed",
    "fWereWolfStrength",
    "fWereWolfUnarmored",
    "fWereWolfWillPower",
    "fWortChanceValue"
};

const char * CSMWorld::DefaultGmsts::Ints[CSMWorld::DefaultGmsts::IntCount] =
{
    "i1stPersonSneakDelta",
    "iAlarmAttack",
    "iAlarmKilling",
    "iAlarmPickPocket",
    "iAlarmStealing",
    "iAlarmTresspass",
    "iAlchemyMod",
    "iAutoPCSpellMax",
    "iAutoRepFacMod",
    "iAutoRepLevMod",
    "iAutoSpellAlterationMax",
    "iAutoSpellAttSkillMin",
    "iAutoSpellConjurationMax",
    "iAutoSpellDestructionMax",
    "iAutoSpellIllusionMax",
    "iAutoSpellMysticismMax",
    "iAutoSpellRestorationMax",
    "iAutoSpellTimesCanCast",
    "iBarterFailDisposition",
    "iBarterSuccessDisposition",
    "iBaseArmorSkill",
    "iBlockMaxChance",
    "iBlockMinChance",
    "iBootsWeight",
    "iCrimeAttack",
    "iCrimeKilling",
    "iCrimePickPocket",
    "iCrimeThreshold",
    "iCrimeThresholdMultiplier",
    "iCrimeTresspass",
    "iCuirassWeight",
    "iDaysinPrisonMod",
    "iDispAttackMod",
    "iDispKilling",
    "iDispTresspass",
    "iFightAlarmMult",
    "iFightAttack",
    "iFightAttacking",
    "iFightDistanceBase",
    "iFightKilling",
    "iFightPickpocket",
    "iFightTrespass",
    "iFlee",
    "iGauntletWeight",
    "iGreavesWeight",
    "iGreetDistanceMultiplier",
    "iGreetDuration",
    "iHelmWeight",
    "iKnockDownOddsBase",
    "iKnockDownOddsMult",
    "iLevelUp01Mult",
    "iLevelUp02Mult",
    "iLevelUp03Mult",
    "iLevelUp04Mult",
    "iLevelUp05Mult",
    "iLevelUp06Mult",
    "iLevelUp07Mult",
    "iLevelUp08Mult",
    "iLevelUp09Mult",
    "iLevelUp10Mult",
    "iLevelupMajorMult",
    "iLevelupMajorMultAttribute",
    "iLevelupMinorMult",
    "iLevelupMinorMultAttribute",
    "iLevelupMiscMultAttriubte",
    "iLevelupSpecialization",
    "iLevelupTotal",
    "iMagicItemChargeConst",
    "iMagicItemChargeOnce",
    "iMagicItemChargeStrike",
    "iMagicItemChargeUse",
    "iMaxActivateDist",
    "iMaxInfoDist",
    "iMonthsToRespawn",
    "iNumberCreatures",
    "iPauldronWeight",
    "iPerMinChance",
    "iPerMinChange",
    "iPickMaxChance",
    "iPickMinChance",
    "iShieldWeight",
    "iSoulAmountForConstantEffect",
    "iTrainingMod",
    "iVoiceAttackOdds",
    "iVoiceHitOdds",
    "iWereWolfBounty",
    "iWereWolfFightMod",
    "iWereWolfFleeMod",
    "iWereWolfLevelToAttack"
};

const char * CSMWorld::DefaultGmsts::Strings[CSMWorld::DefaultGmsts::StringCount] =
{
    "s3dAudio",
    "s3dHardware",
    "s3dSoftware",
    "sAbsorb",
    "sAcrobat",
    "sActivate",
    "sActivateXbox",
    "sActorInCombat",
    "sAdmire",
    "sAdmireFail",
    "sAdmireSuccess",
    "sAgent",
    "sAgiDesc",
    "sAIDistance",
    "sAlembic",
    "sAllTab",
    "sAlways",
    "sAlways_Run",
    "sand",
    "sApparatus",
    "sApparelTab",
    "sArcher",
    "sArea",
    "sAreaDes",
    "sArmor",
    "sArmorRating",
    "sAsk",
    "sAssassin",
    "sAt",
    "sAttack",
    "sAttributeAgility",
    "sAttributeEndurance",
    "sAttributeIntelligence",
    "sAttributeListTitle",
    "sAttributeLuck",
    "sAttributePersonality",
    "sAttributesMenu1",
    "sAttributeSpeed",
    "sAttributeStrength",
    "sAttributeWillpower",
    "sAudio",
    "sAuto_Run",
    "sBack",
    "sBackspace",
    "sBackXbox",
    "sBarbarian",
    "sBard",
    "sBarter",
    "sBarterDialog1",
    "sBarterDialog10",
    "sBarterDialog11",
    "sBarterDialog12",
    "sBarterDialog2",
    "sBarterDialog3",
    "sBarterDialog4",
    "sBarterDialog5",
    "sBarterDialog6",
    "sBarterDialog7",
    "sBarterDialog8",
    "sBarterDialog9",
    "sBattlemage",
    "sBestAttack",
    "sBirthSign",
    "sBirthsignmenu1",
    "sBirthsignmenu2",
    "sBlocks",
    "sBonusSkillTitle",
    "sBookPageOne",
    "sBookPageTwo",
    "sBookSkillMessage",
    "sBounty",
    "sBreath",
    "sBribe 10 Gold",
    "sBribe 100 Gold",
    "sBribe 1000 Gold",
    "sBribeFail",
    "sBribeSuccess",
    "sBuy",
    "sBye",
    "sCalcinator",
    "sCancel",
    "sCantEquipWeapWarning",
    "sCastCost",
    "sCaughtStealingMessage",
    "sCenter",
    "sChangedMastersMsg",
    "sCharges",
    "sChooseClassMenu1",
    "sChooseClassMenu2",
    "sChooseClassMenu3",
    "sChooseClassMenu4",
    "sChop",
    "sClass",
    "sClassChoiceMenu1",
    "sClassChoiceMenu2",
    "sClassChoiceMenu3",
    "sClose",
    "sCompanionShare",
    "sCompanionWarningButtonOne",
    "sCompanionWarningButtonTwo",
    "sCompanionWarningMessage",
    "sCondition",
    "sConsoleTitle",
    "sContainer",
    "sContentsMessage1",
    "sContentsMessage2",
    "sContentsMessage3",
    "sControlerVibration",
    "sControls",
    "sControlsMenu1",
    "sControlsMenu2",
    "sControlsMenu3",
    "sControlsMenu4",
    "sControlsMenu5",
    "sControlsMenu6",
    "sCostChance",
    "sCostCharge",
    "sCreate",
    "sCreateClassMenu1",
    "sCreateClassMenu2",
    "sCreateClassMenu3",
    "sCreateClassMenuHelp1",
    "sCreateClassMenuHelp2",
    "sCreateClassMenuWarning",
    "sCreatedEffects",
    "sCrimeHelp",
    "sCrimeMessage",
    "sCrouch_Sneak",
    "sCrouchXbox",
    "sCrusader",
    "sCursorOff",
    "sCustom",
    "sCustomClassName",
    "sDamage",
    "sDark_Gamma",
    "sDay",
    "sDefaultCellname",
    "sDelete",
    "sDeleteGame",
    "sDeleteNote",
    "sDeleteSpell",
    "sDeleteSpellError",
    "sDetail_Level",
    "sDialogMenu1",
    "sDialogText1Xbox",
    "sDialogText2Xbox",
    "sDialogText3Xbox",
    "sDifficulty",
    "sDisposeCorpseFail",
    "sDisposeofCorpse",
    "sDone",
    "sDoYouWantTo",
    "sDrain",
    "sDrop",
    "sDuration",
    "sDurationDes",
    "sEasy",
    "sEditNote",
    "sEffectAbsorbAttribute",
    "sEffectAbsorbFatigue",
    "sEffectAbsorbHealth",
    "sEffectAbsorbSkill",
    "sEffectAbsorbSpellPoints",
    "sEffectAlmsiviIntervention",
    "sEffectBlind",
    "sEffectBoundBattleAxe",
    "sEffectBoundBoots",
    "sEffectBoundCuirass",
    "sEffectBoundDagger",
    "sEffectBoundGloves",
    "sEffectBoundHelm",
    "sEffectBoundLongbow",
    "sEffectBoundLongsword",
    "sEffectBoundMace",
    "sEffectBoundShield",
    "sEffectBoundSpear",
    "sEffectBurden",
    "sEffectCalmCreature",
    "sEffectCalmHumanoid",
    "sEffectChameleon",
    "sEffectCharm",
    "sEffectCommandCreatures",
    "sEffectCommandHumanoids",
    "sEffectCorpus",
    "sEffectCureBlightDisease",
    "sEffectCureCommonDisease",
    "sEffectCureCorprusDisease",
    "sEffectCureParalyzation",
    "sEffectCurePoison",
    "sEffectDamageAttribute",
    "sEffectDamageFatigue",
    "sEffectDamageHealth",
    "sEffectDamageMagicka",
    "sEffectDamageSkill",
    "sEffectDemoralizeCreature",
    "sEffectDemoralizeHumanoid",
    "sEffectDetectAnimal",
    "sEffectDetectEnchantment",
    "sEffectDetectKey",
    "sEffectDisintegrateArmor",
    "sEffectDisintegrateWeapon",
    "sEffectDispel",
    "sEffectDivineIntervention",
    "sEffectDrainAttribute",
    "sEffectDrainFatigue",
    "sEffectDrainHealth",
    "sEffectDrainSkill",
    "sEffectDrainSpellpoints",
    "sEffectExtraSpell",
    "sEffectFeather",
    "sEffectFireDamage",
    "sEffectFireShield",
    "sEffectFortifyAttackBonus",
    "sEffectFortifyAttribute",
    "sEffectFortifyFatigue",
    "sEffectFortifyHealth",
    "sEffectFortifyMagickaMultiplier",
    "sEffectFortifySkill",
    "sEffectFortifySpellpoints",
    "sEffectFrenzyCreature",
    "sEffectFrenzyHumanoid",
    "sEffectFrostDamage",
    "sEffectFrostShield",
    "sEffectInvisibility",
    "sEffectJump",
    "sEffectLevitate",
    "sEffectLight",
    "sEffectLightningShield",
    "sEffectLock",
    "sEffectMark",
    "sEffectNightEye",
    "sEffectOpen",
    "sEffectParalyze",
    "sEffectPoison",
    "sEffectRallyCreature",
    "sEffectRallyHumanoid",
    "sEffectRecall",
    "sEffectReflect",
    "sEffectRemoveCurse",
    "sEffectResistBlightDisease",
    "sEffectResistCommonDisease",
    "sEffectResistCorprusDisease",
    "sEffectResistFire",
    "sEffectResistFrost",
    "sEffectResistMagicka",
    "sEffectResistNormalWeapons",
    "sEffectResistParalysis",
    "sEffectResistPoison",
    "sEffectResistShock",
    "sEffectRestoreAttribute",
    "sEffectRestoreFatigue",
    "sEffectRestoreHealth",
    "sEffectRestoreSkill",
    "sEffectRestoreSpellPoints",
    "sEffects",
    "sEffectSanctuary",
    "sEffectShield",
    "sEffectShockDamage",
    "sEffectSilence",
    "sEffectSlowFall",
    "sEffectSoultrap",
    "sEffectSound",
    "sEffectSpellAbsorption",
    "sEffectStuntedMagicka",
    "sEffectSummonAncestralGhost",
    "sEffectSummonBonelord",
    "sEffectSummonCenturionSphere",
    "sEffectSummonClannfear",
    "sEffectSummonCreature01",
    "sEffectSummonCreature02",
    "sEffectSummonCreature03",
    "sEffectSummonCreature04",
    "sEffectSummonCreature05",
    "sEffectSummonDaedroth",
    "sEffectSummonDremora",
    "sEffectSummonFabricant",
    "sEffectSummonFlameAtronach",
    "sEffectSummonFrostAtronach",
    "sEffectSummonGoldensaint",
    "sEffectSummonGreaterBonewalker",
    "sEffectSummonHunger",
    "sEffectSummonLeastBonewalker",
    "sEffectSummonScamp",
    "sEffectSummonSkeletalMinion",
    "sEffectSummonStormAtronach",
    "sEffectSummonWingedTwilight",
    "sEffectSunDamage",
    "sEffectSwiftSwim",
    "sEffectTelekinesis",
    "sEffectTurnUndead",
    "sEffectVampirism",
    "sEffectWaterBreathing",
    "sEffectWaterWalking",
    "sEffectWeaknessToBlightDisease",
    "sEffectWeaknessToCommonDisease",
    "sEffectWeaknessToCorprusDisease",
    "sEffectWeaknessToFire",
    "sEffectWeaknessToFrost",
    "sEffectWeaknessToMagicka",
    "sEffectWeaknessToNormalWeapons",
    "sEffectWeaknessToPoison",
    "sEffectWeaknessToShock",
    "sEnableJoystick",
    "sEnchanting",
    "sEnchantItems",
    "sEnchantmentHelp1",
    "sEnchantmentHelp10",
    "sEnchantmentHelp2",
    "sEnchantmentHelp3",
    "sEnchantmentHelp4",
    "sEnchantmentHelp5",
    "sEnchantmentHelp6",
    "sEnchantmentHelp7",
    "sEnchantmentHelp8",
    "sEnchantmentHelp9",
    "sEnchantmentMenu1",
    "sEnchantmentMenu10",
    "sEnchantmentMenu11",
    "sEnchantmentMenu12",
    "sEnchantmentMenu2",
    "sEnchantmentMenu3",
    "sEnchantmentMenu4",
    "sEnchantmentMenu5",
    "sEnchantmentMenu6",
    "sEnchantmentMenu7",
    "sEnchantmentMenu8",
    "sEnchantmentMenu9",
    "sEncumbrance",
    "sEndDesc",
    "sEquip",
    "sExitGame",
    "sExpelled",
    "sExpelledMessage",
    "sFace",
    "sFaction",
    "sFar",
    "sFast",
    "sFatDesc",
    "sFatigue",
    "sFavoriteSkills",
    "sfeet",
    "sFileSize",
    "sfootarea",
    "sFootsteps",
    "sfor",
    "sFortify",
    "sForward",
    "sForwardXbox",
    "sFull",
    "sGame",
    "sGameWithoutLauncherXbox",
    "sGamma_Correction",
    "sGeneralMastPlugMismatchMsg",
    "sGold",
    "sGoodbye",
    "sGoverningAttribute",
    "sgp",
    "sHair",
    "sHard",
    "sHeal",
    "sHealer",
    "sHealth",
    "sHealthDesc",
    "sHealthPerHourOfRest",
    "sHealthPerLevel",
    "sHeavy",
    "sHigh",
    "sin",
    "sInfo",
    "sInfoRefusal",
    "sIngredients",
    "sInPrisonTitle",
    "sInputMenu1",
    "sIntDesc",
    "sIntimidate",
    "sIntimidateFail",
    "sIntimidateSuccess",
    "sInvalidSaveGameMsg",
    "sInvalidSaveGameMsgXBOX",
    "sInventory",
    "sInventoryMenu1",
    "sInventoryMessage1",
    "sInventoryMessage2",
    "sInventoryMessage3",
    "sInventoryMessage4",
    "sInventoryMessage5",
    "sInventorySelectNoIngredients",
    "sInventorySelectNoItems",
    "sInventorySelectNoSoul",
    "sItem",
    "sItemCastConstant",
    "sItemCastOnce",
    "sItemCastWhenStrikes",
    "sItemCastWhenUsed",
    "sItemName",
    "sJournal",
    "sJournalCmd",
    "sJournalEntry",
    "sJournalXbox",
    "sJoystickHatShort",
    "sJoystickNotFound",
    "sJoystickShort",
    "sJump",
    "sJumpXbox",
    "sKeyName_00",
    "sKeyName_01",
    "sKeyName_02",
    "sKeyName_03",
    "sKeyName_04",
    "sKeyName_05",
    "sKeyName_06",
    "sKeyName_07",
    "sKeyName_08",
    "sKeyName_09",
    "sKeyName_0A",
    "sKeyName_0B",
    "sKeyName_0C",
    "sKeyName_0D",
    "sKeyName_0E",
    "sKeyName_0F",
    "sKeyName_10",
    "sKeyName_11",
    "sKeyName_12",
    "sKeyName_13",
    "sKeyName_14",
    "sKeyName_15",
    "sKeyName_16",
    "sKeyName_17",
    "sKeyName_18",
    "sKeyName_19",
    "sKeyName_1A",
    "sKeyName_1B",
    "sKeyName_1C",
    "sKeyName_1D",
    "sKeyName_1E",
    "sKeyName_1F",
    "sKeyName_20",
    "sKeyName_21",
    "sKeyName_22",
    "sKeyName_23",
    "sKeyName_24",
    "sKeyName_25",
    "sKeyName_26",
    "sKeyName_27",
    "sKeyName_28",
    "sKeyName_29",
    "sKeyName_2A",
    "sKeyName_2B",
    "sKeyName_2C",
    "sKeyName_2D",
    "sKeyName_2E",
    "sKeyName_2F",
    "sKeyName_30",
    "sKeyName_31",
    "sKeyName_32",
    "sKeyName_33",
    "sKeyName_34",
    "sKeyName_35",
    "sKeyName_36",
    "sKeyName_37",
    "sKeyName_38",
    "sKeyName_39",
    "sKeyName_3A",
    "sKeyName_3B",
    "sKeyName_3C",
    "sKeyName_3D",
    "sKeyName_3E",
    "sKeyName_3F",
    "sKeyName_40",
    "sKeyName_41",
    "sKeyName_42",
    "sKeyName_43",
    "sKeyName_44",
    "sKeyName_45",
    "sKeyName_46",
    "sKeyName_47",
    "sKeyName_48",
    "sKeyName_49",
    "sKeyName_4A",
    "sKeyName_4B",
    "sKeyName_4C",
    "sKeyName_4D",
    "sKeyName_4E",
    "sKeyName_4F",
    "sKeyName_50",
    "sKeyName_51",
    "sKeyName_52",
    "sKeyName_53",
    "sKeyName_54",
    "sKeyName_55",
    "sKeyName_56",
    "sKeyName_57",
    "sKeyName_58",
    "sKeyName_59",
    "sKeyName_5A",
    "sKeyName_5B",
    "sKeyName_5C",
    "sKeyName_5D",
    "sKeyName_5E",
    "sKeyName_5F",
    "sKeyName_60",
    "sKeyName_61",
    "sKeyName_62",
    "sKeyName_63",
    "sKeyName_64",
    "sKeyName_65",
    "sKeyName_66",
    "sKeyName_67",
    "sKeyName_68",
    "sKeyName_69",
    "sKeyName_6A",
    "sKeyName_6B",
    "sKeyName_6C",
    "sKeyName_6D",
    "sKeyName_6E",
    "sKeyName_6F",
    "sKeyName_70",
    "sKeyName_71",
    "sKeyName_72",
    "sKeyName_73",
    "sKeyName_74",
    "sKeyName_75",
    "sKeyName_76",
    "sKeyName_77",
    "sKeyName_78",
    "sKeyName_79",
    "sKeyName_7A",
    "sKeyName_7B",
    "sKeyName_7C",
    "sKeyName_7D",
    "sKeyName_7E",
    "sKeyName_7F",
    "sKeyName_80",
    "sKeyName_81",
    "sKeyName_82",
    "sKeyName_83",
    "sKeyName_84",
    "sKeyName_85",
    "sKeyName_86",
    "sKeyName_87",
    "sKeyName_88",
    "sKeyName_89",
    "sKeyName_8A",
    "sKeyName_8B",
    "sKeyName_8C",
    "sKeyName_8D",
    "sKeyName_8E",
    "sKeyName_8F",
    "sKeyName_90",
    "sKeyName_91",
    "sKeyName_92",
    "sKeyName_93",
    "sKeyName_94",
    "sKeyName_95",
    "sKeyName_96",
    "sKeyName_97",
    "sKeyName_98",
    "sKeyName_99",
    "sKeyName_9A",
    "sKeyName_9B",
    "sKeyName_9C",
    "sKeyName_9D",
    "sKeyName_9E",
    "sKeyName_9F",
    "sKeyName_A0",
    "sKeyName_A1",
    "sKeyName_A2",
    "sKeyName_A3",
    "sKeyName_A4",
    "sKeyName_A5",
    "sKeyName_A6",
    "sKeyName_A7",
    "sKeyName_A8",
    "sKeyName_A9",
    "sKeyName_AA",
    "sKeyName_AB",
    "sKeyName_AC",
    "sKeyName_AD",
    "sKeyName_AE",
    "sKeyName_AF",
    "sKeyName_B0",
    "sKeyName_B1",
    "sKeyName_B2",
    "sKeyName_B3",
    "sKeyName_B4",
    "sKeyName_B5",
    "sKeyName_B6",
    "sKeyName_B7",
    "sKeyName_B8",
    "sKeyName_B9",
    "sKeyName_BA",
    "sKeyName_BB",
    "sKeyName_BC",
    "sKeyName_BD",
    "sKeyName_BE",
    "sKeyName_BF",
    "sKeyName_C0",
    "sKeyName_C1",
    "sKeyName_C2",
    "sKeyName_C3",
    "sKeyName_C4",
    "sKeyName_C5",
    "sKeyName_C6",
    "sKeyName_C7",
    "sKeyName_C8",
    "sKeyName_C9",
    "sKeyName_CA",
    "sKeyName_CB",
    "sKeyName_CC",
    "sKeyName_CD",
    "sKeyName_CE",
    "sKeyName_CF",
    "sKeyName_D0",
    "sKeyName_D1",
    "sKeyName_D2",
    "sKeyName_D3",
    "sKeyName_D4",
    "sKeyName_D5",
    "sKeyName_D6",
    "sKeyName_D7",
    "sKeyName_D8",
    "sKeyName_D9",
    "sKeyName_DA",
    "sKeyName_DB",
    "sKeyName_DC",
    "sKeyName_DD",
    "sKeyName_DE",
    "sKeyName_DF",
    "sKeyName_E0",
    "sKeyName_E1",
    "sKeyName_E2",
    "sKeyName_E3",
    "sKeyName_E4",
    "sKeyName_E5",
    "sKeyName_E6",
    "sKeyName_E7",
    "sKeyName_E8",
    "sKeyName_E9",
    "sKeyName_EA",
    "sKeyName_EB",
    "sKeyName_EC",
    "sKeyName_ED",
    "sKeyName_EE",
    "sKeyName_EF",
    "sKeyName_F0",
    "sKeyName_F1",
    "sKeyName_F2",
    "sKeyName_F3",
    "sKeyName_F4",
    "sKeyName_F5",
    "sKeyName_F6",
    "sKeyName_F7",
    "sKeyName_F8",
    "sKeyName_F9",
    "sKeyName_FA",
    "sKeyName_FB",
    "sKeyName_FC",
    "sKeyName_FD",
    "sKeyName_FE",
    "sKeyName_FF",
    "sKeyUsed",
    "sKilledEssential",
    "sKnight",
    "sLeft",
    "sLess",
    "sLevel",
    "sLevelProgress",
    "sLevels",
    "sLevelUp",
    "sLevelUpMenu1",
    "sLevelUpMenu2",
    "sLevelUpMenu3",
    "sLevelUpMenu4",
    "sLevelUpMsg",
    "sLevitateDisabled",
    "sLight",
    "sLight_Gamma",
    "sLoadFailedMessage",
    "sLoadGame",
    "sLoadingErrorsMsg",
    "sLoadingMessage1",
    "sLoadingMessage14",
    "sLoadingMessage15",
    "sLoadingMessage2",
    "sLoadingMessage3",
    "sLoadingMessage4",
    "sLoadingMessage5",
    "sLoadingMessage9",
    "sLoadLastSaveMsg",
    "sLocal",
    "sLockFail",
    "sLockImpossible",
    "sLockLevel",
    "sLockSuccess",
    "sLookDownXbox",
    "sLookUpXbox",
    "sLow",
    "sLucDesc",
    "sMagDesc",
    "sMage",
    "sMagic",
    "sMagicAncestralGhostID",
    "sMagicBonelordID",
    "sMagicBoundBattleAxeID",
    "sMagicBoundBootsID",
    "sMagicBoundCuirassID",
    "sMagicBoundDaggerID",
    "sMagicBoundHelmID",
    "sMagicBoundLeftGauntletID",
    "sMagicBoundLongbowID",
    "sMagicBoundLongswordID",
    "sMagicBoundMaceID",
    "sMagicBoundRightGauntletID",
    "sMagicBoundShieldID",
    "sMagicBoundSpearID",
    "sMagicCannotRecast",
    "sMagicCenturionSphereID",
    "sMagicClannfearID",
    "sMagicContractDisease",
    "sMagicCorprusWorsens",
    "sMagicCreature01ID",
    "sMagicCreature02ID",
    "sMagicCreature03ID",
    "sMagicCreature04ID",
    "sMagicCreature05ID",
    "sMagicDaedrothID",
    "sMagicDremoraID",
    "sMagicEffects",
    "sMagicFabricantID",
    "sMagicFlameAtronachID",
    "sMagicFrostAtronachID",
    "sMagicGoldenSaintID",
    "sMagicGreaterBonewalkerID",
    "sMagicHungerID",
    "sMagicInsufficientCharge",
    "sMagicInsufficientSP",
    "sMagicInvalidEffect",
    "sMagicInvalidTarget",
    "sMagicItem",
    "sMagicLeastBonewalkerID",
    "sMagicLockSuccess",
    "sMagicMenu",
    "sMagicOpenSuccess",
    "sMagicPCResisted",
    "sMagicScampID",
    "sMagicSelectTitle",
    "sMagicSkeletalMinionID",
    "sMagicSkillFail",
    "sMagicStormAtronachID",
    "sMagicTab",
    "sMagicTargetResisted",
    "sMagicTargetResistsWeapons",
    "sMagicWingedTwilightID",
    "sMagnitude",
    "sMagnitudeDes",
    "sMake Enchantment",
    "sMap",
    "sMaster",
    "sMastPlugMismatchMsg",
    "sMaximumSaveGameMessage",
    "sMaxSale",
    "sMedium",
    "sMenu_Help_Delay",
    "sMenu_Mode",
    "sMenuModeXbox",
    "sMenuNextXbox",
    "sMenuPrevXbox",
    "sMenus",
    "sMessage1",
    "sMessage2",
    "sMessage3",
    "sMessage4",
    "sMessage5",
    "sMessageQuestionAnswer1",
    "sMessageQuestionAnswer2",
    "sMessageQuestionAnswer3",
    "sMiscTab",
    "sMissingMastersMsg",
    "sMonk",
    "sMonthEveningstar",
    "sMonthFirstseed",
    "sMonthFrostfall",
    "sMonthHeartfire",
    "sMonthLastseed",
    "sMonthMidyear",
    "sMonthMorningstar",
    "sMonthRainshand",
    "sMonthSecondseed",
    "sMonthSunsdawn",
    "sMonthSunsdusk",
    "sMonthSunsheight",
    "sMore",
    "sMortar",
    "sMouse",
    "sMouseFlip",
    "sMouseWheelDownShort",
    "sMouseWheelUpShort",
    "sMove",
    "sMoveDownXbox",
    "sMoveUpXbox",
    "sMusic",
    "sName",
    "sNameTitle",
    "sNear",
    "sNeedOneSkill",
    "sNeedTwoSkills",
    "sNewGame",
    "sNext",
    "sNextRank",
    "sNextSpell",
    "sNextSpellXbox",
    "sNextWeapon",
    "sNextWeaponXbox",
    "sNightblade",
    "sNo",
    "sNoName",
    "sNone",
    "sNotifyMessage1",
    "sNotifyMessage10",
    "sNotifyMessage11",
    "sNotifyMessage12",
    "sNotifyMessage13",
    "sNotifyMessage14",
    "sNotifyMessage15",
    "sNotifyMessage16",
    "sNotifyMessage16_a",
    "sNotifyMessage17",
    "sNotifyMessage18",
    "sNotifyMessage19",
    "sNotifyMessage2",
    "sNotifyMessage20",
    "sNotifyMessage21",
    "sNotifyMessage22",
    "sNotifyMessage23",
    "sNotifyMessage24",
    "sNotifyMessage25",
    "sNotifyMessage26",
    "sNotifyMessage27",
    "sNotifyMessage28",
    "sNotifyMessage29",
    "sNotifyMessage3",
    "sNotifyMessage30",
    "sNotifyMessage31",
    "sNotifyMessage32",
    "sNotifyMessage33",
    "sNotifyMessage34",
    "sNotifyMessage35",
    "sNotifyMessage36",
    "sNotifyMessage37",
    "sNotifyMessage38",
    "sNotifyMessage39",
    "sNotifyMessage4",
    "sNotifyMessage40",
    "sNotifyMessage41",
    "sNotifyMessage42",
    "sNotifyMessage43",
    "sNotifyMessage44",
    "sNotifyMessage45",
    "sNotifyMessage46",
    "sNotifyMessage47",
    "sNotifyMessage48",
    "sNotifyMessage49",
    "sNotifyMessage4XBOX",
    "sNotifyMessage5",
    "sNotifyMessage50",
    "sNotifyMessage51",
    "sNotifyMessage52",
    "sNotifyMessage53",
    "sNotifyMessage54",
    "sNotifyMessage55",
    "sNotifyMessage56",
    "sNotifyMessage57",
    "sNotifyMessage58",
    "sNotifyMessage59",
    "sNotifyMessage6",
    "sNotifyMessage60",
    "sNotifyMessage61",
    "sNotifyMessage62",
    "sNotifyMessage63",
    "sNotifyMessage64",
    "sNotifyMessage65",
    "sNotifyMessage66",
    "sNotifyMessage67",
    "sNotifyMessage6a",
    "sNotifyMessage7",
    "sNotifyMessage8",
    "sNotifyMessage9",
    "sOff",
    "sOffer",
    "sOfferMenuTitle",
    "sOK",
    "sOn",
    "sOnce",
    "sOneHanded",
    "sOnetypeEffectMessage",
    "sonword",
    "sOptions",
    "sOptionsMenuXbox",
    "spercent",
    "sPerDesc",
    "sPersuasion",
    "sPersuasionMenuTitle",
    "sPickUp",
    "sPilgrim",
    "spoint",
    "spoints",
    "sPotionSuccess",
    "sPowerAlreadyUsed",
    "sPowers",
    "sPreferences",
    "sPrefs",
    "sPrev",
    "sPrevSpell",
    "sPrevSpellXbox",
    "sPrevWeapon",
    "sPrevWeaponXbox",
    "sProfitValue",
    "sQuality",
    "sQuanityMenuMessage01",
    "sQuanityMenuMessage02",
    "sQuestionDeleteSpell",
    "sQuestionMark",
    "sQuick0Xbox",
    "sQuick10Cmd",
    "sQuick1Cmd",
    "sQuick2Cmd",
    "sQuick3Cmd",
    "sQuick4Cmd",
    "sQuick4Xbox",
    "sQuick5Cmd",
    "sQuick5Xbox",
    "sQuick6Cmd",
    "sQuick6Xbox",
    "sQuick7Cmd",
    "sQuick7Xbox",
    "sQuick8Cmd",
    "sQuick8Xbox",
    "sQuick9Cmd",
    "sQuick9Xbox",
    "sQuick_Save",
    "sQuickLoadCmd",
    "sQuickLoadXbox",
    "sQuickMenu",
    "sQuickMenu1",
    "sQuickMenu2",
    "sQuickMenu3",
    "sQuickMenu4",
    "sQuickMenu5",
    "sQuickMenu6",
    "sQuickMenuInstruc",
    "sQuickMenuTitle",
    "sQuickSaveCmd",
    "sQuickSaveXbox",
    "sRace",
    "sRaceMenu1",
    "sRaceMenu2",
    "sRaceMenu3",
    "sRaceMenu4",
    "sRaceMenu5",
    "sRaceMenu6",
    "sRaceMenu7",
    "sRacialTraits",
    "sRange",
    "sRangeDes",
    "sRangeSelf",
    "sRangeTarget",
    "sRangeTouch",
    "sReady_Magic",
    "sReady_Weapon",
    "sReadyItemXbox",
    "sReadyMagicXbox",
    "sRechargeEnchantment",
    "sRender_Distance",
    "sRepair",
    "sRepairFailed",
    "sRepairServiceTitle",
    "sRepairSuccess",
    "sReputation",
    "sResChangeWarning",
    "sRest",
    "sRestIllegal",
    "sRestKey",
    "sRestMenu1",
    "sRestMenu2",
    "sRestMenu3",
    "sRestMenu4",
    "sRestMenuXbox",
    "sRestore",
    "sRetort",
    "sReturnToGame",
    "sRight",
    "sRogue",
    "sRun",
    "sRunXbox",
    "sSave",
    "sSaveGame",
    "sSaveGameDenied",
    "sSaveGameFailed",
    "sSaveGameNoMemory",
    "sSaveGameTooBig",
    "sSaveMenu1",
    "sSaveMenuHelp01",
    "sSaveMenuHelp02",
    "sSaveMenuHelp03",
    "sSaveMenuHelp04",
    "sSaveMenuHelp05",
    "sSaveMenuHelp06",
    "sSchool",
    "sSchoolAlteration",
    "sSchoolConjuration",
    "sSchoolDestruction",
    "sSchoolIllusion",
    "sSchoolMysticism",
    "sSchoolRestoration",
    "sScout",
    "sScrolldown",
    "sScrollup",
    "ssecond",
    "sseconds",
    "sSeldom",
    "sSelect",
    "sSell",
    "sSellerGold",
    "sService",
    "sServiceRefusal",
    "sServiceRepairTitle",
    "sServiceSpellsTitle",
    "sServiceTrainingTitle",
    "sServiceTrainingWords",
    "sServiceTravelTitle",
    "sSetValueMessage01",
    "sSex",
    "sShadows",
    "sShadowText",
    "sShift",
    "sSkill",
    "sSkillAcrobatics",
    "sSkillAlchemy",
    "sSkillAlteration",
    "sSkillArmorer",
    "sSkillAthletics",
    "sSkillAxe",
    "sSkillBlock",
    "sSkillBluntweapon",
    "sSkillClassMajor",
    "sSkillClassMinor",
    "sSkillClassMisc",
    "sSkillConjuration",
    "sSkillDestruction",
    "sSkillEnchant",
    "sSkillHandtohand",
    "sSkillHeavyarmor",
    "sSkillIllusion",
    "sSkillLightarmor",
    "sSkillLongblade",
    "sSkillMarksman",
    "sSkillMaxReached",
    "sSkillMediumarmor",
    "sSkillMercantile",
    "sSkillMysticism",
    "sSkillProgress",
    "sSkillRestoration",
    "sSkillSecurity",
    "sSkillShortblade",
    "sSkillsMenu1",
    "sSkillsMenuReputationHelp",
    "sSkillSneak",
    "sSkillSpear",
    "sSkillSpeechcraft",
    "sSkillUnarmored",
    "sSlash",
    "sSleepInterrupt",
    "sSlideLeftXbox",
    "sSlideRightXbox",
    "sSlow",
    "sSorceror",
    "sSoulGem",
    "sSoulGemsWithSouls",
    "sSoultrapSuccess",
    "sSpace",
    "sSpdDesc",
    "sSpecialization",
    "sSpecializationCombat",
    "sSpecializationMagic",
    "sSpecializationMenu1",
    "sSpecializationStealth",
    "sSpellmaking",
    "sSpellmakingHelp1",
    "sSpellmakingHelp2",
    "sSpellmakingHelp3",
    "sSpellmakingHelp4",
    "sSpellmakingHelp5",
    "sSpellmakingHelp6",
    "sSpellmakingMenu1",
    "sSpellmakingMenuTitle",
    "sSpells",
    "sSpellServiceTitle",
    "sSpellsword",
    "sStartCell",
    "sStartCellError",
    "sStartError",
    "sStats",
    "sStrafe",
    "sStrDesc",
    "sStrip",
    "sSubtitles",
    "sSystemMenuXbox",
    "sTake",
    "sTakeAll",
    "sTargetCriticalStrike",
    "sTaunt",
    "sTauntFail",
    "sTauntSuccess",
    "sTeleportDisabled",
    "sThief",
    "sThrust",
    "sTo",
    "sTogglePOVCmd",
    "sTogglePOVXbox",
    "sToggleRunXbox",
    "sTopics",
    "sTotalCost",
    "sTotalSold",
    "sTraining",
    "sTrainingServiceTitle",
    "sTraits",
    "sTransparency_Menu",
    "sTrapFail",
    "sTrapImpossible",
    "sTrapped",
    "sTrapSuccess",
    "sTravel",
    "sTravelServiceTitle",
    "sTurn",
    "sTurnLeftXbox",
    "sTurnRightXbox",
    "sTwoHanded",
    "sType",
    "sTypeAbility",
    "sTypeBlightDisease",
    "sTypeCurse",
    "sTypeDisease",
    "sTypePower",
    "sTypeSpell",
    "sUnequip",
    "sUnlocked",
    "sUntilHealed",
    "sUse",
    "sUserDefinedClass",
    "sUses",
    "sUseXbox",
    "sValue",
    "sVideo",
    "sVideoWarning",
    "sVoice",
    "sWait",
    "sWarrior",
    "sWaterReflectUpdate",
    "sWaterTerrainReflect",
    "sWeaponTab",
    "sWeight",
    "sWerewolfAlarmMessage",
    "sWerewolfPopup",
    "sWerewolfRefusal",
    "sWerewolfRestMessage",
    "sWilDesc",
    "sWitchhunter",
    "sWorld",
    "sWornTab",
    "sXStrafe",
    "sXTimes",
    "sXTimesINT",
    "sYes",
    "sYourGold"
};

const char * CSMWorld::DefaultGmsts::OptionalFloats[CSMWorld::DefaultGmsts::OptionalFloatCount] =
{
    "fCombatDistanceWerewolfMod",
    "fFleeDistance",
    "fWereWolfAcrobatics",
    "fWereWolfAgility",
    "fWereWolfAlchemy",
    "fWereWolfAlteration",
    "fWereWolfArmorer",
    "fWereWolfAthletics",
    "fWereWolfAxe",
    "fWereWolfBlock",
    "fWereWolfBluntWeapon",
    "fWereWolfConjuration",
    "fWereWolfDestruction",
    "fWereWolfEnchant",
    "fWereWolfEndurance",
    "fWereWolfFatigue",
    "fWereWolfHandtoHand",
    "fWereWolfHealth",
    "fWereWolfHeavyArmor",
    "fWereWolfIllusion",
    "fWereWolfIntellegence",
    "fWereWolfLightArmor",
    "fWereWolfLongBlade",
    "fWereWolfLuck",
    "fWereWolfMagicka",
    "fWereWolfMarksman",
    "fWereWolfMediumArmor",
    "fWereWolfMerchantile",
    "fWereWolfMysticism",
    "fWereWolfPersonality",
    "fWereWolfRestoration",
    "fWereWolfRunMult",
    "fWereWolfSecurity",
    "fWereWolfShortBlade",
    "fWereWolfSilverWeaponDamageMult",
    "fWereWolfSneak",
    "fWereWolfSpear",
    "fWereWolfSpeechcraft",
    "fWereWolfSpeed",
    "fWereWolfStrength",
    "fWereWolfUnarmored",
    "fWereWolfWillPower"
};

const char * CSMWorld::DefaultGmsts::OptionalInts[CSMWorld::DefaultGmsts::OptionalIntCount] =
{
    "iWereWolfBounty",
    "iWereWolfFightMod",
    "iWereWolfFleeMod",
    "iWereWolfLevelToAttack"
};

const char * CSMWorld::DefaultGmsts::OptionalStrings[CSMWorld::DefaultGmsts::OptionalStringCount] =
{
    "sCompanionShare",
    "sCompanionWarningButtonOne",
    "sCompanionWarningButtonTwo",
    "sCompanionWarningMessage",
    "sDeleteNote",
    "sEditNote",
    "sEffectSummonCreature01",
    "sEffectSummonCreature02",
    "sEffectSummonCreature03",
    "sEffectSummonCreature04",
    "sEffectSummonCreature05",
    "sEffectSummonFabricant",
    "sLevitateDisabled",
    "sMagicCreature01ID",
    "sMagicCreature02ID",
    "sMagicCreature03ID",
    "sMagicCreature04ID",
    "sMagicCreature05ID",
    "sMagicFabricantID",
    "sMaxSale",
    "sProfitValue",
    "sTeleportDisabled",
    "sWerewolfAlarmMessage",
    "sWerewolfPopup",
    "sWerewolfRefusal",
    "sWerewolfRestMessage"
};

const float CSMWorld::DefaultGmsts::FloatsDefaultValues[CSMWorld::DefaultGmsts::FloatCount] =
{
    0.3f,    // fAIFleeFleeMult
    7.0f,    // fAIFleeHealthMult
    3.0f,    // fAIMagicSpellMult
    1.0f,    // fAIMeleeArmorMult
    1.0f,    // fAIMeleeSummWeaponMult
    2.0f,    // fAIMeleeWeaponMult
    5.0f,    // fAIRangeMagicSpellMult
    5.0f,    // fAIRangeMeleeWeaponMult
    2000.0f, // fAlarmRadius
    1.0f,    // fAthleticsRunBonus
    40.0f,   // fAudioDefaultMaxDistance
    5.0f,    // fAudioDefaultMinDistance
    50.0f,   // fAudioMaxDistanceMult
    20.0f,   // fAudioMinDistanceMult
    60.0f,   // fAudioVoiceDefaultMaxDistance
    10.0f,   // fAudioVoiceDefaultMinDistance
    50.0f,   // fAutoPCSpellChance
    80.0f,   // fAutoSpellChance
    50.0f,   // fBargainOfferBase
    -4.0f,   // fBargainOfferMulti
    24.0f,   // fBarterGoldResetDelay
    1.75f,   // fBaseRunMultiplier
    1.25f,   // fBlockStillBonus
    150.0f,  // fBribe1000Mod
    75.0f,   // fBribe100Mod
    35.0f,   // fBribe10Mod
    60.0f,   // fCombatAngleXY
    60.0f,   // fCombatAngleZ
    0.25f,   // fCombatArmorMinMult
    -90.0f,  // fCombatBlockLeftAngle
    30.0f,   // fCombatBlockRightAngle
    4.0f,    // fCombatCriticalStrikeMult
    0.1f,    // fCombatDelayCreature
    0.1f,    // fCombatDelayNPC
    128.0f,  // fCombatDistance
    0.3f,    // fCombatDistanceWerewolfMod
    30.0f,   // fCombatForceSideAngle
    0.2f,    // fCombatInvisoMult
    1.5f,    // fCombatKODamageMult
    45.0f,   // fCombatTorsoSideAngle
    0.3f,    // fCombatTorsoStartPercent
    0.8f,    // fCombatTorsoStopPercent
    15.0f,   // fConstantEffectMult
    72.0f,   // fCorpseClearDelay
    72.0f,   // fCorpseRespawnDelay
    0.5f,    // fCrimeGoldDiscountMult
    0.9f,    // fCrimeGoldTurnInMult
    1.0f,    // fCrimeStealing
    0.5f,    // fDamageStrengthBase
    0.1f,    // fDamageStrengthMult
    5.0f,    // fDifficultyMult
    2.5f,    // fDiseaseXferChance
    -10.0f,  // fDispAttacking
    -1.0f,   // fDispBargainFailMod
    1.0f,    // fDispBargainSuccessMod
    0.0f,    // fDispCrimeMod
    -10.0f,  // fDispDiseaseMod
    3.0f,    // fDispFactionMod
    1.0f,    // fDispFactionRankBase
    0.5f,    // fDispFactionRankMult
    1.0f,    // fDispositionMod
    50.0f,   // fDispPersonalityBase
    0.5f,    // fDispPersonalityMult
    -25.0f,  // fDispPickPocketMod
    5.0f,    // fDispRaceMod
    -0.5f,   // fDispStealing
    -5.0f,   // fDispWeaponDrawn
    0.5f,    // fEffectCostMult
    0.1f,    // fElementalShieldMult
    3.0f,    // fEnchantmentChanceMult
    0.5f,    // fEnchantmentConstantChanceMult
    100.0f,  // fEnchantmentConstantDurationMult
    0.1f,    // fEnchantmentMult
    1000.0f, // fEnchantmentValueMult
    0.3f,    // fEncumberedMoveEffect
    5.0f,    // fEncumbranceStrMult
    0.04f,   // fEndFatigueMult
    0.25f,   // fFallAcroBase
    0.01f,   // fFallAcroMult
    400.0f,  // fFallDamageDistanceMin
    0.0f,    // fFallDistanceBase
    0.07f,   // fFallDistanceMult
    2.0f,    // fFatigueAttackBase
    0.0f,    // fFatigueAttackMult
    1.25f,   // fFatigueBase
    4.0f,    // fFatigueBlockBase
    0.0f,    // fFatigueBlockMult
    5.0f,    // fFatigueJumpBase
    0.0f,    // fFatigueJumpMult
    0.5f,    // fFatigueMult
    2.5f,    // fFatigueReturnBase
    0.02f,   // fFatigueReturnMult
    5.0f,    // fFatigueRunBase
    2.0f,    // fFatigueRunMult
    1.5f,    // fFatigueSneakBase
    1.5f,    // fFatigueSneakMult
    0.0f,    // fFatigueSpellBase
    0.0f,    // fFatigueSpellCostMult
    0.0f,    // fFatigueSpellMult
    7.0f,    // fFatigueSwimRunBase
    0.0f,    // fFatigueSwimRunMult
    2.5f,    // fFatigueSwimWalkBase
    0.0f,    // fFatigueSwimWalkMult
    0.2f,    // fFightDispMult
    0.005f,  // fFightDistanceMultiplier
    50.0f,   // fFightStealing
    3000.0f, // fFleeDistance
    512.0f,  // fGreetDistanceReset
    0.1f,    // fHandtoHandHealthPer
    1.0f,    // fHandToHandReach
    0.5f,    // fHoldBreathEndMult
    20.0f,   // fHoldBreathTime
    0.75f,   // fIdleChanceMultiplier
    1.0f,    // fIngredientMult
    0.5f,    // fInteriorHeadTrackMult
    128.0f,  // fJumpAcrobaticsBase
    4.0f,    // fJumpAcroMultiplier
    0.5f,    // fJumpEncumbranceBase
    1.0f,    // fJumpEncumbranceMultiplier
    0.5f,    // fJumpMoveBase
    0.5f,    // fJumpMoveMult
    1.0f,    // fJumpRunMultiplier
    0.5f,    // fKnockDownMult
    5.0f,    // fLevelMod
    0.1f,    // fLevelUpHealthEndMult
    0.6f,    // fLightMaxMod
    10.0f,   // fLuckMod
    10.0f,   // fMagesGuildTravel
    1.5f,    // fMagicCreatureCastDelay
    0.0167f, // fMagicDetectRefreshRate
    1.0f,    // fMagicItemConstantMult
    1.0f,    // fMagicItemCostMult
    1.0f,    // fMagicItemOnceMult
    1.0f,    // fMagicItemPriceMult
    0.05f,   // fMagicItemRechargePerSecond
    1.0f,    // fMagicItemStrikeMult
    1.0f,    // fMagicItemUsedMult
    3.0f,    // fMagicStartIconBlink
    0.5f,    // fMagicSunBlockedMult
    0.75f,   // fMajorSkillBonus
    300.0f,  // fMaxFlySpeed
    0.5f,    // fMaxHandToHandMult
    400.0f,  // fMaxHeadTrackDistance
    200.0f,  // fMaxWalkSpeed
    300.0f,  // fMaxWalkSpeedCreature
    0.9f,    // fMedMaxMod
    0.1f,    // fMessageTimePerChar
    5.0f,    // fMinFlySpeed
    0.1f,    // fMinHandToHandMult
    1.0f,    // fMinorSkillBonus
    100.0f,  // fMinWalkSpeed
    5.0f,    // fMinWalkSpeedCreature
    1.25f,   // fMiscSkillBonus
    2.0f,    // fNPCbaseMagickaMult
    0.5f,    // fNPCHealthBarFade
    3.0f,    // fNPCHealthBarTime
    1.0f,    // fPCbaseMagickaMult
    0.3f,    // fPerDieRollMult
    5.0f,    // fPersonalityMod
    1.0f,    // fPerTempMult
    -1.0f,   // fPickLockMult
    0.3f,    // fPickPocketMod
    20.0f,   // fPotionMinUsefulDuration
    0.5f,    // fPotionStrengthMult
    0.5f,    // fPotionT1DurMult
    1.5f,    // fPotionT1MagMult
    20.0f,   // fPotionT4BaseStrengthMult
    12.0f,   // fPotionT4EquipStrengthMult
    3000.0f, // fProjectileMaxSpeed
    400.0f,  // fProjectileMinSpeed
    25.0f,   // fProjectileThrownStoreChance
    3.0f,    // fRepairAmountMult
    1.0f,    // fRepairMult
    1.0f,    // fReputationMod
    0.15f,   // fRestMagicMult
    0.0f,    // fSeriousWoundMult
    0.25f,   // fSleepRandMod
    0.3f,    // fSleepRestMod
    -1.0f,   // fSneakBootMult
    0.5f,    // fSneakDistanceBase
    0.002f,  // fSneakDistanceMultiplier
    0.5f,    // fSneakNoViewMult
    1.0f,    // fSneakSkillMult
    0.75f,   // fSneakSpeedMultiplier
    1.0f,    // fSneakUseDelay
    500.0f,  // fSneakUseDist
    1.5f,    // fSneakViewMult
    3.0f,    // fSoulGemMult
    0.8f,    // fSpecialSkillBonus
    7.0f,    // fSpellMakingValueMult
    2.0f,    // fSpellPriceMult
    10.0f,   // fSpellValueMult
    0.25f,   // fStromWalkMult
    0.7f,    // fStromWindSpeed
    3.0f,    // fSuffocationDamage
    0.9f,    // fSwimHeightScale
    0.1f,    // fSwimRunAthleticsMult
    0.5f,    // fSwimRunBase
    0.02f,   // fSwimWalkAthleticsMult
    0.5f,    // fSwimWalkBase
    1.0f,    // fSwingBlockBase
    1.0f,    // fSwingBlockMult
    1000.0f, // fTargetSpellMaxSpeed
    1000.0f, // fThrownWeaponMaxSpeed
    300.0f,  // fThrownWeaponMinSpeed
    0.0f,    // fTrapCostMult
    4000.0f, // fTravelMult
    16000.0f,// fTravelTimeMult
    0.1f,    // fUnarmoredBase1
    0.065f,  // fUnarmoredBase2
    30.0f,   // fVanityDelay
    10.0f,   // fVoiceIdleOdds
    0.0f,    // fWaterReflectUpdateAlways
    10.0f,   // fWaterReflectUpdateSeldom
    0.1f,    // fWeaponDamageMult
    1.0f,    // fWeaponFatigueBlockMult
    0.25f,   // fWeaponFatigueMult
    150.0f,  // fWereWolfAcrobatics
    150.0f,  // fWereWolfAgility
    1.0f,    // fWereWolfAlchemy
    1.0f,    // fWereWolfAlteration
    1.0f,    // fWereWolfArmorer
    150.0f,  // fWereWolfAthletics
    1.0f,    // fWereWolfAxe
    1.0f,    // fWereWolfBlock
    1.0f,    // fWereWolfBluntWeapon
    1.0f,    // fWereWolfConjuration
    1.0f,    // fWereWolfDestruction
    1.0f,    // fWereWolfEnchant
    150.0f,  // fWereWolfEndurance
    400.0f,  // fWereWolfFatigue
    100.0f,  // fWereWolfHandtoHand
    2.0f,    // fWereWolfHealth
    1.0f,    // fWereWolfHeavyArmor
    1.0f,    // fWereWolfIllusion
    1.0f,    // fWereWolfIntellegence
    1.0f,    // fWereWolfLightArmor
    1.0f,    // fWereWolfLongBlade
    1.0f,    // fWereWolfLuck
    100.0f,  // fWereWolfMagicka
    1.0f,    // fWereWolfMarksman
    1.0f,    // fWereWolfMediumArmor
    1.0f,    // fWereWolfMerchantile
    1.0f,    // fWereWolfMysticism
    1.0f,    // fWereWolfPersonality
    1.0f,    // fWereWolfRestoration
    1.5f,    // fWereWolfRunMult
    1.0f,    // fWereWolfSecurity
    1.0f,    // fWereWolfShortBlade
    1.5f,    // fWereWolfSilverWeaponDamageMult
    1.0f,    // fWereWolfSneak
    1.0f,    // fWereWolfSpear
    1.0f,    // fWereWolfSpeechcraft
    150.0f,  // fWereWolfSpeed
    150.0f,  // fWereWolfStrength
    100.0f,  // fWereWolfUnarmored
    1.0f,    // fWereWolfWillPower
    15.0f   // fWortChanceValue
};

const int CSMWorld::DefaultGmsts::IntsDefaultValues[CSMWorld::DefaultGmsts::IntCount] =
{
    10,     // i1stPersonSneakDelta
    50,     // iAlarmAttack
    90,     // iAlarmKilling
    20,     // iAlarmPickPocket
    1,      // iAlarmStealing
    5,      // iAlarmTresspass
    2,      // iAlchemyMod
    100,    // iAutoPCSpellMax
    2,      // iAutoRepFacMod
    0,      // iAutoRepLevMod
    5,      // iAutoSpellAlterationMax
    70,     // iAutoSpellAttSkillMin
    2,      // iAutoSpellConjurationMax
    5,      // iAutoSpellDestructionMax
    5,      // iAutoSpellIllusionMax
    5,      // iAutoSpellMysticismMax
    5,      // iAutoSpellRestorationMax
    3,      // iAutoSpellTimesCanCast
    -1,     // iBarterFailDisposition
    1,      // iBarterSuccessDisposition
    30,     // iBaseArmorSkill
    50,     // iBlockMaxChance
    10,     // iBlockMinChance
    20,     // iBootsWeight
    40,     // iCrimeAttack
    1000,   // iCrimeKilling
    25,     // iCrimePickPocket
    1000,   // iCrimeThreshold
    10,     // iCrimeThresholdMultiplier
    5,      // iCrimeTresspass
    30,     // iCuirassWeight
    100,    // iDaysinPrisonMod
    -50,    // iDispAttackMod
    -50,    // iDispKilling
    -20,    // iDispTresspass
    1,      // iFightAlarmMult
    100,    // iFightAttack
    50,     // iFightAttacking
    20,     // iFightDistanceBase
    50,     // iFightKilling
    25,     // iFightPickpocket
    25,     // iFightTrespass
    0,      // iFlee
    5,      // iGauntletWeight
    15,     // iGreavesWeight
    6,      // iGreetDistanceMultiplier
    4,      // iGreetDuration
    5,      // iHelmWeight
    50,     // iKnockDownOddsBase
    50,     // iKnockDownOddsMult
    2,      // iLevelUp01Mult
    2,      // iLevelUp02Mult
    2,      // iLevelUp03Mult
    2,      // iLevelUp04Mult
    3,      // iLevelUp05Mult
    3,      // iLevelUp06Mult
    3,      // iLevelUp07Mult
    4,      // iLevelUp08Mult
    4,      // iLevelUp09Mult
    5,      // iLevelUp10Mult
    1,      // iLevelupMajorMult
    1,      // iLevelupMajorMultAttribute
    1,      // iLevelupMinorMult
    1,      // iLevelupMinorMultAttribute
    1,      // iLevelupMiscMultAttriubte
    1,      // iLevelupSpecialization
    10,     // iLevelupTotal
    10,     // iMagicItemChargeConst
    1,      // iMagicItemChargeOnce
    10,     // iMagicItemChargeStrike
    5,      // iMagicItemChargeUse
    192,    // iMaxActivateDist
    192,    // iMaxInfoDist
    4,      // iMonthsToRespawn
    1,      // iNumberCreatures
    10,     // iPauldronWeight
    5,      // iPerMinChance
    10,     // iPerMinChange
    75,     // iPickMaxChance
    5,      // iPickMinChance
    15,     // iShieldWeight
    400,    // iSoulAmountForConstantEffect
    10,     // iTrainingMod
    10,     // iVoiceAttackOdds
    30,     // iVoiceHitOdds
    10000,  // iWereWolfBounty
    100,    // iWereWolfFightMod
    100,    // iWereWolfFleeMod
    20     // iWereWolfLevelToAttack
};

const float CSMWorld::DefaultGmsts::FloatLimits[CSMWorld::DefaultGmsts::FloatCount * 2] =
{
    -FInf,      FInf,       // fAIFleeFleeMult
    -FInf,      FInf,       // fAIFleeHealthMult
    -FInf,      FInf,       // fAIMagicSpellMult
    -FInf,      FInf,       // fAIMeleeArmorMult
    -FInf,      FInf,       // fAIMeleeSummWeaponMult
    -FInf,      FInf,       // fAIMeleeWeaponMult
    -FInf,      FInf,       // fAIRangeMagicSpellMult
    -FInf,      FInf,       // fAIRangeMeleeWeaponMult
    0,          FInf,       // fAlarmRadius
    -FInf,      FInf,       // fAthleticsRunBonus
    0,          FInf,       // fAudioDefaultMaxDistance
    0,          FInf,       // fAudioDefaultMinDistance
    0,          FInf,       // fAudioMaxDistanceMult
    0,          FInf,       // fAudioMinDistanceMult
    0,          FInf,       // fAudioVoiceDefaultMaxDistance
    0,          FInf,       // fAudioVoiceDefaultMinDistance
    0,          FInf,       // fAutoPCSpellChance
    0,          FInf,       // fAutoSpellChance
    -FInf,      FInf,       // fBargainOfferBase
    -FInf,      0,          // fBargainOfferMulti
    -FInf,      FInf,       // fBarterGoldResetDelay
    0,          FInf,       // fBaseRunMultiplier
    -FInf,      FInf,       // fBlockStillBonus
    0,          FInf,       // fBribe1000Mod
    0,          FInf,       // fBribe100Mod
    0,          FInf,       // fBribe10Mod
    0,          FInf,       // fCombatAngleXY
    0,          FInf,       // fCombatAngleZ
    0,          1,          // fCombatArmorMinMult
    -180,       0,          // fCombatBlockLeftAngle
    0,          180,        // fCombatBlockRightAngle
    0,          FInf,       // fCombatCriticalStrikeMult
    0,          FInf,       // fCombatDelayCreature
    0,          FInf,       // fCombatDelayNPC
    0,          FInf,       // fCombatDistance
    -FInf,      FInf,       // fCombatDistanceWerewolfMod
    -FInf,      FInf,       // fCombatForceSideAngle
    0,          FInf,       // fCombatInvisoMult
    0,          FInf,       // fCombatKODamageMult
    -FInf,      FInf,       // fCombatTorsoSideAngle
    -FInf,      FInf,       // fCombatTorsoStartPercent
    -FInf,      FInf,       // fCombatTorsoStopPercent
    -FInf,      FInf,       // fConstantEffectMult
    -FInf,      FInf,       // fCorpseClearDelay
    -FInf,      FInf,       // fCorpseRespawnDelay
    0,          1,          // fCrimeGoldDiscountMult
    0,          FInf,       // fCrimeGoldTurnInMult
    0,          FInf,       // fCrimeStealing
    0,          FInf,       // fDamageStrengthBase
    0,          FInf,       // fDamageStrengthMult
    -FInf,      FInf,       // fDifficultyMult
    0,          FInf,       // fDiseaseXferChance
    -FInf,      0,          // fDispAttacking
    -FInf,      FInf,       // fDispBargainFailMod
    -FInf,      FInf,       // fDispBargainSuccessMod
    -FInf,      0,          // fDispCrimeMod
    -FInf,      0,          // fDispDiseaseMod
    0,          FInf,       // fDispFactionMod
    0,          FInf,       // fDispFactionRankBase
    0,          FInf,       // fDispFactionRankMult
    0,          FInf,       // fDispositionMod
    0,          FInf,       // fDispPersonalityBase
    0,          FInf,       // fDispPersonalityMult
    -FInf,      0,          // fDispPickPocketMod
    0,          FInf,       // fDispRaceMod
    -FInf,      0,          // fDispStealing
    -FInf,      0,          // fDispWeaponDrawn
    0,          FInf,       // fEffectCostMult
    0,          FInf,       // fElementalShieldMult
    FEps,       FInf,       // fEnchantmentChanceMult
    0,          FInf,       // fEnchantmentConstantChanceMult
    0,          FInf,       // fEnchantmentConstantDurationMult
    0,          FInf,       // fEnchantmentMult
    0,          FInf,       // fEnchantmentValueMult
    0,          FInf,       // fEncumberedMoveEffect
    0,          FInf,       // fEncumbranceStrMult
    0,          FInf,       // fEndFatigueMult
    -FInf,      FInf,       // fFallAcroBase
    0,          FInf,       // fFallAcroMult
    0,          FInf,       // fFallDamageDistanceMin
    -FInf,      FInf,       // fFallDistanceBase
    0,          FInf,       // fFallDistanceMult
    -FInf,      FInf,       // fFatigueAttackBase
    0,          FInf,       // fFatigueAttackMult
    0,          FInf,       // fFatigueBase
    0,          FInf,       // fFatigueBlockBase
    0,          FInf,       // fFatigueBlockMult
    0,          FInf,       // fFatigueJumpBase
    0,          FInf,       // fFatigueJumpMult
    0,          FInf,       // fFatigueMult
    -FInf,      FInf,       // fFatigueReturnBase
    0,          FInf,       // fFatigueReturnMult
    -FInf,      FInf,       // fFatigueRunBase
    0,          FInf,       // fFatigueRunMult
    -FInf,      FInf,       // fFatigueSneakBase
    0,          FInf,       // fFatigueSneakMult
    -FInf,      FInf,       // fFatigueSpellBase
    -FInf,      FInf,       // fFatigueSpellCostMult
    0,          FInf,       // fFatigueSpellMult
    -FInf,      FInf,       // fFatigueSwimRunBase
    0,          FInf,       // fFatigueSwimRunMult
    -FInf,      FInf,       // fFatigueSwimWalkBase
    0,          FInf,       // fFatigueSwimWalkMult
    -FInf,      FInf,       // fFightDispMult
    -FInf,      FInf,       // fFightDistanceMultiplier
    -FInf,      FInf,       // fFightStealing
    -FInf,      FInf,       // fFleeDistance
    -FInf,      FInf,       // fGreetDistanceReset
    0,          FInf,       // fHandtoHandHealthPer
    0,          FInf,       // fHandToHandReach
    -FInf,      FInf,       // fHoldBreathEndMult
    0,          FInf,       // fHoldBreathTime
    0,          FInf,       // fIdleChanceMultiplier
    -FInf,      FInf,       // fIngredientMult
    0,          FInf,       // fInteriorHeadTrackMult
    -FInf,      FInf,       // fJumpAcrobaticsBase
    0,          FInf,       // fJumpAcroMultiplier
    -FInf,      FInf,       // fJumpEncumbranceBase
    0,          FInf,       // fJumpEncumbranceMultiplier
    -FInf,      FInf,       // fJumpMoveBase
    0,          FInf,       // fJumpMoveMult
    0,          FInf,       // fJumpRunMultiplier
    -FInf,      FInf,       // fKnockDownMult
    0,          FInf,       // fLevelMod
    0,          FInf,       // fLevelUpHealthEndMult
    0,          FInf,       // fLightMaxMod
    0,          FInf,       // fLuckMod
    0,          FInf,       // fMagesGuildTravel
    -FInf,      FInf,       // fMagicCreatureCastDelay
    -FInf,      FInf,       // fMagicDetectRefreshRate
    -FInf,      FInf,       // fMagicItemConstantMult
    -FInf,      FInf,       // fMagicItemCostMult
    -FInf,      FInf,       // fMagicItemOnceMult
    -FInf,      FInf,       // fMagicItemPriceMult
    0,          FInf,       // fMagicItemRechargePerSecond
    -FInf,      FInf,       // fMagicItemStrikeMult
    -FInf,      FInf,       // fMagicItemUsedMult
    0,          FInf,       // fMagicStartIconBlink
    0,          FInf,       // fMagicSunBlockedMult
    FEps,       FInf,       // fMajorSkillBonus
    0,          FInf,       // fMaxFlySpeed
    0,          FInf,       // fMaxHandToHandMult
    0,          FInf,       // fMaxHeadTrackDistance
    0,          FInf,       // fMaxWalkSpeed
    0,          FInf,       // fMaxWalkSpeedCreature
    0,          FInf,       // fMedMaxMod
    0,          FInf,       // fMessageTimePerChar
    0,          FInf,       // fMinFlySpeed
    0,          FInf,       // fMinHandToHandMult
    FEps,       FInf,       // fMinorSkillBonus
    0,          FInf,       // fMinWalkSpeed
    0,          FInf,       // fMinWalkSpeedCreature
    FEps,       FInf,       // fMiscSkillBonus
    0,          FInf,       // fNPCbaseMagickaMult
    0,          FInf,       // fNPCHealthBarFade
    0,          FInf,       // fNPCHealthBarTime
    0,          FInf,       // fPCbaseMagickaMult
    0,          FInf,       // fPerDieRollMult
    0,          FInf,       // fPersonalityMod
    0,          FInf,       // fPerTempMult
    -FInf,      0,          // fPickLockMult
    0,          FInf,       // fPickPocketMod
    -FInf,      FInf,       // fPotionMinUsefulDuration
    0,          FInf,       // fPotionStrengthMult
    FEps,       FInf,       // fPotionT1DurMult
    FEps,       FInf,       // fPotionT1MagMult
    -FInf,      FInf,       // fPotionT4BaseStrengthMult
    -FInf,      FInf,       // fPotionT4EquipStrengthMult
    0,          FInf,       // fProjectileMaxSpeed
    0,          FInf,       // fProjectileMinSpeed
    0,          FInf,       // fProjectileThrownStoreChance
    0,          FInf,       // fRepairAmountMult
    0,          FInf,       // fRepairMult
    0,          FInf,       // fReputationMod
    0,          FInf,       // fRestMagicMult
    -FInf,      FInf,       // fSeriousWoundMult
    0,          FInf,       // fSleepRandMod
    0,          FInf,       // fSleepRestMod
    -FInf,      0,          // fSneakBootMult
    -FInf,      FInf,       // fSneakDistanceBase
    0,          FInf,       // fSneakDistanceMultiplier
    0,          FInf,       // fSneakNoViewMult
    0,          FInf,       // fSneakSkillMult
    0,          FInf,       // fSneakSpeedMultiplier
    0,          FInf,       // fSneakUseDelay
    0,          FInf,       // fSneakUseDist
    0,          FInf,       // fSneakViewMult
    0,          FInf,       // fSoulGemMult
    0,          FInf,       // fSpecialSkillBonus
    0,          FInf,       // fSpellMakingValueMult
    -FInf,      FInf,       // fSpellPriceMult
    0,          FInf,       // fSpellValueMult
    0,          FInf,       // fStromWalkMult
    0,          FInf,       // fStromWindSpeed
    0,          FInf,       // fSuffocationDamage
    0,          FInf,       // fSwimHeightScale
    0,          FInf,       // fSwimRunAthleticsMult
    0,          FInf,       // fSwimRunBase
    -FInf,      FInf,       // fSwimWalkAthleticsMult
    -FInf,      FInf,       // fSwimWalkBase
    0,          FInf,       // fSwingBlockBase
    0,          FInf,       // fSwingBlockMult
    0,          FInf,       // fTargetSpellMaxSpeed
    0,          FInf,       // fThrownWeaponMaxSpeed
    0,          FInf,       // fThrownWeaponMinSpeed
    0,          FInf,       // fTrapCostMult
    0,          FInf,       // fTravelMult
    0,          FInf,       // fTravelTimeMult
    0,          FInf,       // fUnarmoredBase1
    0,          FInf,       // fUnarmoredBase2
    0,          FInf,       // fVanityDelay
    0,          FInf,       // fVoiceIdleOdds
    -FInf,      FInf,       // fWaterReflectUpdateAlways
    -FInf,      FInf,       // fWaterReflectUpdateSeldom
    0,          FInf,       // fWeaponDamageMult
    0,          FInf,       // fWeaponFatigueBlockMult
    0,          FInf,       // fWeaponFatigueMult
    0,          FInf,       // fWereWolfAcrobatics
    -FInf,      FInf,       // fWereWolfAgility
    -FInf,      FInf,       // fWereWolfAlchemy
    -FInf,      FInf,       // fWereWolfAlteration
    -FInf,      FInf,       // fWereWolfArmorer
    -FInf,      FInf,       // fWereWolfAthletics
    -FInf,      FInf,       // fWereWolfAxe
    -FInf,      FInf,       // fWereWolfBlock
    -FInf,      FInf,       // fWereWolfBluntWeapon
    -FInf,      FInf,       // fWereWolfConjuration
    -FInf,      FInf,       // fWereWolfDestruction
    -FInf,      FInf,       // fWereWolfEnchant
    -FInf,      FInf,       // fWereWolfEndurance
    -FInf,      FInf,       // fWereWolfFatigue
    -FInf,      FInf,       // fWereWolfHandtoHand
    -FInf,      FInf,       // fWereWolfHealth
    -FInf,      FInf,       // fWereWolfHeavyArmor
    -FInf,      FInf,       // fWereWolfIllusion
    -FInf,      FInf,       // fWereWolfIntellegence
    -FInf,      FInf,       // fWereWolfLightArmor
    -FInf,      FInf,       // fWereWolfLongBlade
    -FInf,      FInf,       // fWereWolfLuck
    -FInf,      FInf,       // fWereWolfMagicka
    -FInf,      FInf,       // fWereWolfMarksman
    -FInf,      FInf,       // fWereWolfMediumArmor
    -FInf,      FInf,       // fWereWolfMerchantile
    -FInf,      FInf,       // fWereWolfMysticism
    -FInf,      FInf,       // fWereWolfPersonality
    -FInf,      FInf,       // fWereWolfRestoration
    0,          FInf,       // fWereWolfRunMult
    -FInf,      FInf,       // fWereWolfSecurity
    -FInf,      FInf,       // fWereWolfShortBlade
    -FInf,      FInf,       // fWereWolfSilverWeaponDamageMult
    -FInf,      FInf,       // fWereWolfSneak
    -FInf,      FInf,       // fWereWolfSpear
    -FInf,      FInf,       // fWereWolfSpeechcraft
    -FInf,      FInf,       // fWereWolfSpeed
    -FInf,      FInf,       // fWereWolfStrength
    -FInf,      FInf,       // fWereWolfUnarmored
    -FInf,      FInf,       // fWereWolfWillPower
    0,          FInf        // fWortChanceValue
};

const int CSMWorld::DefaultGmsts::IntLimits[CSMWorld::DefaultGmsts::IntCount * 2] =
{
    IMin,    IMax,  // i1stPersonSneakDelta
    IMin,    IMax,  // iAlarmAttack
    IMin,    IMax,  // iAlarmKilling
    IMin,    IMax,  // iAlarmPickPocket
    IMin,    IMax,  // iAlarmStealing
    IMin,    IMax,  // iAlarmTresspass
    IMin,    IMax,  // iAlchemyMod
    0,       IMax,  // iAutoPCSpellMax
    IMin,    IMax,  // iAutoRepFacMod
    IMin,    IMax,  // iAutoRepLevMod
    IMin,    IMax,  // iAutoSpellAlterationMax
    0,       IMax,  // iAutoSpellAttSkillMin
    IMin,    IMax,  // iAutoSpellConjurationMax
    IMin,    IMax,  // iAutoSpellDestructionMax
    IMin,    IMax,  // iAutoSpellIllusionMax
    IMin,    IMax,  // iAutoSpellMysticismMax
    IMin,    IMax,  // iAutoSpellRestorationMax
    0,       IMax,  // iAutoSpellTimesCanCast
    IMin,    0,     // iBarterFailDisposition
    0,       IMax,  // iBarterSuccessDisposition
    1,       IMax,  // iBaseArmorSkill
    0,       IMax,  // iBlockMaxChance
    0,       IMax,  // iBlockMinChance
    0,       IMax,  // iBootsWeight
    IMin,    IMax,  // iCrimeAttack
    IMin,    IMax,  // iCrimeKilling
    IMin,    IMax,  // iCrimePickPocket
    0,       IMax,  // iCrimeThreshold
    0,       IMax,  // iCrimeThresholdMultiplier
    IMin,    IMax,  // iCrimeTresspass
    0,       IMax,  // iCuirassWeight
    1,       IMax,  // iDaysinPrisonMod
    IMin,    0,     // iDispAttackMod
    IMin,    0,     // iDispKilling
    IMin,    0,     // iDispTresspass
    IMin,    IMax,  // iFightAlarmMult
    IMin,    IMax,  // iFightAttack
    IMin,    IMax,  // iFightAttacking
    0,       IMax,  // iFightDistanceBase
    IMin,    IMax,  // iFightKilling
    IMin,    IMax,  // iFightPickpocket
    IMin,    IMax,  // iFightTrespass
    IMin,    IMax,  // iFlee
    0,       IMax,  // iGauntletWeight
    0,       IMax,  // iGreavesWeight
    0,       IMax,  // iGreetDistanceMultiplier
    0,       IMax,  // iGreetDuration
    0,       IMax,  // iHelmWeight
    IMin,    IMax,  // iKnockDownOddsBase
    IMin,    IMax,  // iKnockDownOddsMult
    IMin,    IMax,  // iLevelUp01Mult
    IMin,    IMax,  // iLevelUp02Mult
    IMin,    IMax,  // iLevelUp03Mult
    IMin,    IMax,  // iLevelUp04Mult
    IMin,    IMax,  // iLevelUp05Mult
    IMin,    IMax,  // iLevelUp06Mult
    IMin,    IMax,  // iLevelUp07Mult
    IMin,    IMax,  // iLevelUp08Mult
    IMin,    IMax,  // iLevelUp09Mult
    IMin,    IMax,  // iLevelUp10Mult
    IMin,    IMax,  // iLevelupMajorMult
    IMin,    IMax,  // iLevelupMajorMultAttribute
    IMin,    IMax,  // iLevelupMinorMult
    IMin,    IMax,  // iLevelupMinorMultAttribute
    IMin,    IMax,  // iLevelupMiscMultAttriubte
    IMin,    IMax,  // iLevelupSpecialization
    IMin,    IMax,  // iLevelupTotal
    IMin,    IMax,  // iMagicItemChargeConst
    IMin,    IMax,  // iMagicItemChargeOnce
    IMin,    IMax,  // iMagicItemChargeStrike
    IMin,    IMax,  // iMagicItemChargeUse
    IMin,    IMax,  // iMaxActivateDist
    IMin,    IMax,  // iMaxInfoDist
    0,       IMax,  // iMonthsToRespawn
    0,       IMax,  // iNumberCreatures
    0,       IMax,  // iPauldronWeight
    0,       IMax,  // iPerMinChance
    0,       IMax,  // iPerMinChange
    0,       IMax,  // iPickMaxChance
    0,       IMax,  // iPickMinChance
    0,       IMax,  // iShieldWeight
    0,       IMax,  // iSoulAmountForConstantEffect
    0,       IMax,  // iTrainingMod
    0,       IMax,  // iVoiceAttackOdds
    0,       IMax,  // iVoiceHitOdds
    IMin,    IMax,  // iWereWolfBounty
    IMin,    IMax,  // iWereWolfFightMod
    IMin,    IMax,  // iWereWolfFleeMod
    IMin,    IMax   // iWereWolfLevelToAttack
};
