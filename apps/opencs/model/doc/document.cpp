#include "document.hpp"

#include <cassert>
#include <fstream>

#include <boost/filesystem.hpp>

#ifndef Q_MOC_RUN
#include <components/files/configurationmanager.hpp>
#endif

#include "../../view/world/physicssystem.hpp"

void CSMDoc::Document::addGmsts()
{
    static const char *gmstFloats[] =
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
        "fWortChanceValue",
        0
    };

    static const float gmstFloatsValues[] =
    {
        0.3,    // fAIFleeFleeMult
        7.0,    // fAIFleeHealthMult
        3.0,    // fAIMagicSpellMult
        1.0,    // fAIMeleeArmorMult
        1.0,    // fAIMeleeSummWeaponMult
        2.0,    // fAIMeleeWeaponMult
        5.0,    // fAIRangeMagicSpellMult
        5.0,    // fAIRangeMeleeWeaponMult
        2000.0, // fAlarmRadius
        1.0,    // fAthleticsRunBonus
        40.0,   // fAudioDefaultMaxDistance
        5.0,    // fAudioDefaultMinDistance
        50.0,   // fAudioMaxDistanceMult
        20.0,   // fAudioMinDistanceMult
        60.0,   // fAudioVoiceDefaultMaxDistance
        10.0,   // fAudioVoiceDefaultMinDistance
        50.0,   // fAutoPCSpellChance
        80.0,   // fAutoSpellChance
        50.0,   // fBargainOfferBase
        -4.0,   // fBargainOfferMulti
        24.0,   // fBarterGoldResetDelay
        1.75,   // fBaseRunMultiplier
        1.25,   // fBlockStillBonus
        150.0,  // fBribe1000Mod
        75.0,   // fBribe100Mod
        35.0,   // fBribe10Mod
        60.0,   // fCombatAngleXY
        60.0,   // fCombatAngleZ
        0.25,   // fCombatArmorMinMult
        -90.0,  // fCombatBlockLeftAngle
        30.0,   // fCombatBlockRightAngle
        4.0,    // fCombatCriticalStrikeMult
        0.1,    // fCombatDelayCreature
        0.1,    // fCombatDelayNPC
        128.0,  // fCombatDistance
        0.3,    // fCombatDistanceWerewolfMod
        30.0,   // fCombatForceSideAngle
        0.2,    // fCombatInvisoMult
        1.5,    // fCombatKODamageMult
        45.0,   // fCombatTorsoSideAngle
        0.3,    // fCombatTorsoStartPercent
        0.8,    // fCombatTorsoStopPercent
        15.0,   // fConstantEffectMult
        72.0,   // fCorpseClearDelay
        72.0,   // fCorpseRespawnDelay
        0.5,    // fCrimeGoldDiscountMult
        0.9,    // fCrimeGoldTurnInMult
        1.0,    // fCrimeStealing
        0.5,    // fDamageStrengthBase
        0.1,    // fDamageStrengthMult
        5.0,    // fDifficultyMult
        2.5,    // fDiseaseXferChance
        -10.0,  // fDispAttacking
        -1.0,   // fDispBargainFailMod
        1.0,    // fDispBargainSuccessMod
        0.0,    // fDispCrimeMod
        -10.0,  // fDispDiseaseMod
        3.0,    // fDispFactionMod
        1.0,    // fDispFactionRankBase
        0.5,    // fDispFactionRankMult
        1.0,    // fDispositionMod
        50.0,   // fDispPersonalityBase
        0.5,    // fDispPersonalityMult
        -25.0,  // fDispPickPocketMod
        5.0,    // fDispRaceMod
        -0.5,   // fDispStealing
        -5.0,   // fDispWeaponDrawn
        0.5,    // fEffectCostMult
        0.1,    // fElementalShieldMult
        3.0,    // fEnchantmentChanceMult
        0.5,    // fEnchantmentConstantChanceMult
        100.0,  // fEnchantmentConstantDurationMult
        0.1,    // fEnchantmentMult
        1000.0, // fEnchantmentValueMult
        0.3,    // fEncumberedMoveEffect
        5.0,    // fEncumbranceStrMult
        0.04,   // fEndFatigueMult
        0.25,   // fFallAcroBase
        0.01,   // fFallAcroMult
        400.0,  // fFallDamageDistanceMin
        0.0,    // fFallDistanceBase
        0.07,   // fFallDistanceMult
        2.0,    // fFatigueAttackBase
        0.0,    // fFatigueAttackMult
        1.25,   // fFatigueBase
        4.0,    // fFatigueBlockBase
        0.0,    // fFatigueBlockMult
        5.0,    // fFatigueJumpBase
        0.0,    // fFatigueJumpMult
        0.5,    // fFatigueMult
        2.5,    // fFatigueReturnBase
        0.02,   // fFatigueReturnMult
        5.0,    // fFatigueRunBase
        2.0,    // fFatigueRunMult
        1.5,    // fFatigueSneakBase
        1.5,    // fFatigueSneakMult
        0.0,    // fFatigueSpellBase
        0.0,    // fFatigueSpellCostMult
        0.0,    // fFatigueSpellMult
        7.0,    // fFatigueSwimRunBase
        0.0,    // fFatigueSwimRunMult
        2.5,    // fFatigueSwimWalkBase
        0.0,    // fFatigueSwimWalkMult
        0.2,    // fFightDispMult
        0.005,  // fFightDistanceMultiplier
        50.0,   // fFightStealing
        3000.0, // fFleeDistance
        512.0,  // fGreetDistanceReset
        0.1,    // fHandtoHandHealthPer
        1.0,    // fHandToHandReach
        0.5,    // fHoldBreathEndMult
        20.0,   // fHoldBreathTime
        0.75,   // fIdleChanceMultiplier
        1.0,    // fIngredientMult
        0.5,    // fInteriorHeadTrackMult
        128.0,  // fJumpAcrobaticsBase
        4.0,    // fJumpAcroMultiplier
        0.5,    // fJumpEncumbranceBase
        1.0,    // fJumpEncumbranceMultiplier
        0.5,    // fJumpMoveBase
        0.5,    // fJumpMoveMult
        1.0,    // fJumpRunMultiplier
        0.5,    // fKnockDownMult
        5.0,    // fLevelMod
        0.1,    // fLevelUpHealthEndMult
        0.6,    // fLightMaxMod
        10.0,   // fLuckMod
        10.0,   // fMagesGuildTravel
        1.5,    // fMagicCreatureCastDelay
        0.0167, // fMagicDetectRefreshRate
        1.0,    // fMagicItemConstantMult
        1.0,    // fMagicItemCostMult
        1.0,    // fMagicItemOnceMult
        1.0,    // fMagicItemPriceMult
        0.05,   // fMagicItemRechargePerSecond
        1.0,    // fMagicItemStrikeMult
        1.0,    // fMagicItemUsedMult
        3.0,    // fMagicStartIconBlink
        0.5,    // fMagicSunBlockedMult
        0.75,   // fMajorSkillBonus
        300.0,  // fMaxFlySpeed
        0.5,    // fMaxHandToHandMult
        400.0,  // fMaxHeadTrackDistance
        200.0,  // fMaxWalkSpeed
        300.0,  // fMaxWalkSpeedCreature
        0.9,    // fMedMaxMod
        0.1,    // fMessageTimePerChar
        5.0,    // fMinFlySpeed
        0.1,    // fMinHandToHandMult
        1.0,    // fMinorSkillBonus
        100.0,  // fMinWalkSpeed
        5.0,    // fMinWalkSpeedCreature
        1.25,   // fMiscSkillBonus
        2.0,    // fNPCbaseMagickaMult
        0.5,    // fNPCHealthBarFade
        3.0,    // fNPCHealthBarTime
        1.0,    // fPCbaseMagickaMult
        0.3,    // fPerDieRollMult
        5.0,    // fPersonalityMod
        1.0,    // fPerTempMult
        -1.0,   // fPickLockMult
        0.3,    // fPickPocketMod
        20.0,   // fPotionMinUsefulDuration
        0.5,    // fPotionStrengthMult
        0.5,    // fPotionT1DurMult
        1.5,    // fPotionT1MagMult
        20.0,   // fPotionT4BaseStrengthMult
        12.0,   // fPotionT4EquipStrengthMult
        3000.0, // fProjectileMaxSpeed
        400.0,  // fProjectileMinSpeed
        25.0,   // fProjectileThrownStoreChance
        3.0,    // fRepairAmountMult
        1.0,    // fRepairMult
        1.0,    // fReputationMod
        0.15,   // fRestMagicMult
        0.0,    // fSeriousWoundMult
        0.25,   // fSleepRandMod
        0.3,    // fSleepRestMod
        -1.0,   // fSneakBootMult
        0.5,    // fSneakDistanceBase
        0.002,  // fSneakDistanceMultiplier
        0.5,    // fSneakNoViewMult
        1.0,    // fSneakSkillMult
        0.75,   // fSneakSpeedMultiplier
        1.0,    // fSneakUseDelay
        500.0,  // fSneakUseDist
        1.5,    // fSneakViewMult
        3.0,    // fSoulGemMult
        0.8,    // fSpecialSkillBonus
        7.0,    // fSpellMakingValueMult
        2.0,    // fSpellPriceMult
        10.0,   // fSpellValueMult
        0.25,   // fStromWalkMult
        0.7,    // fStromWindSpeed
        3.0,    // fSuffocationDamage
        0.9,    // fSwimHeightScale
        0.1,    // fSwimRunAthleticsMult
        0.5,    // fSwimRunBase
        0.02,   // fSwimWalkAthleticsMult
        0.5,    // fSwimWalkBase
        1.0,    // fSwingBlockBase
        1.0,    // fSwingBlockMult
        1000.0, // fTargetSpellMaxSpeed
        1000.0, // fThrownWeaponMaxSpeed
        300.0,  // fThrownWeaponMinSpeed
        0.0,    // fTrapCostMult
        4000.0, // fTravelMult
        16000.0,// fTravelTimeMult
        0.1,    // fUnarmoredBase1
        0.065,  // fUnarmoredBase2
        30.0,   // fVanityDelay
        10.0,   // fVoiceIdleOdds
        0.0,    // fWaterReflectUpdateAlways
        10.0,   // fWaterReflectUpdateSeldom
        0.1,    // fWeaponDamageMult
        1.0,    // fWeaponFatigueBlockMult
        0.25,   // fWeaponFatigueMult
        150.0,  // fWereWolfAcrobatics
        150.0,  // fWereWolfAgility
        1.0,    // fWereWolfAlchemy
        1.0,    // fWereWolfAlteration
        1.0,    // fWereWolfArmorer
        150.0,  // fWereWolfAthletics
        1.0,    // fWereWolfAxe
        1.0,    // fWereWolfBlock
        1.0,    // fWereWolfBluntWeapon
        1.0,    // fWereWolfConjuration
        1.0,    // fWereWolfDestruction
        1.0,    // fWereWolfEnchant
        150.0,  // fWereWolfEndurance
        400.0,  // fWereWolfFatigue
        100.0,  // fWereWolfHandtoHand
        2.0,    // fWereWolfHealth
        1.0,    // fWereWolfHeavyArmor
        1.0,    // fWereWolfIllusion
        1.0,    // fWereWolfIntellegence
        1.0,    // fWereWolfLightArmor
        1.0,    // fWereWolfLongBlade
        1.0,    // fWereWolfLuck
        100.0,  // fWereWolfMagicka
        1.0,    // fWereWolfMarksman
        1.0,    // fWereWolfMediumArmor
        1.0,    // fWereWolfMerchantile
        1.0,    // fWereWolfMysticism
        1.0,    // fWereWolfPersonality
        1.0,    // fWereWolfRestoration
        1.5,    // fWereWolfRunMult
        1.0,    // fWereWolfSecurity
        1.0,    // fWereWolfShortBlade
        1.5,    // fWereWolfSilverWeaponDamageMult
        1.0,    // fWereWolfSneak
        1.0,    // fWereWolfSpear
        1.0,    // fWereWolfSpeechcraft
        150.0,  // fWereWolfSpeed
        150.0,  // fWereWolfStrength
        100.0,  // fWereWolfUnarmored
        1.0,    // fWereWolfWillPower
        15.0,   // fWortChanceValue
    };

    static const char *gmstIntegers[] =
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
        "iWereWolfLevelToAttack",
        0
    };

    static const int gmstIntegersValues[] =
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
        20,     // iWereWolfLevelToAttack
    };

    static const char *gmstStrings[] =
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
        "sBribe",
        "sBribe",
        "sBribe",
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
        "sMake",
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
        "sYourGold",
        0
    };

    for (int i=0; gmstFloats[i]; i++)
    {
        ESM::GameSetting gmst;
        gmst.mId = gmstFloats[i];
        gmst.mValue.setType (ESM::VT_Float);
        gmst.mValue.setFloat (gmstFloatsValues[i]);
        getData().getGmsts().add (gmst);
    }

    for (int i=0; gmstIntegers[i]; i++)
    {
        ESM::GameSetting gmst;
        gmst.mId = gmstIntegers[i];
        gmst.mValue.setType (ESM::VT_Int);
        gmst.mValue.setInteger (gmstIntegersValues[i]);
        getData().getGmsts().add (gmst);
    }

    for (int i=0; gmstStrings[i]; i++)
    {
        ESM::GameSetting gmst;
        gmst.mId = gmstStrings[i];
        gmst.mValue.setType (ESM::VT_String);
        gmst.mValue.setString ("");
        getData().getGmsts().add (gmst);
    }
}

void CSMDoc::Document::addOptionalGmsts()
{
    static const char *sFloats[] =
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
        "fWereWolfWillPower",
        0
    };

    static const char *sIntegers[] =
    {
        "iWereWolfBounty",
        "iWereWolfFightMod",
        "iWereWolfFleeMod",
        "iWereWolfLevelToAttack",
        0
    };

    static const char *sStrings[] =
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
        "sWerewolfRestMessage",
        0
    };

    for (int i=0; sFloats[i]; ++i)
    {
        ESM::GameSetting gmst;
        gmst.mId = sFloats[i];
        gmst.blank();
        gmst.mValue.setType (ESM::VT_Float);
        addOptionalGmst (gmst);
    }

    for (int i=0; sIntegers[i]; ++i)
    {
        ESM::GameSetting gmst;
        gmst.mId = sIntegers[i];
        gmst.blank();
        gmst.mValue.setType (ESM::VT_Int);
        addOptionalGmst (gmst);
    }

    for (int i=0; sStrings[i]; ++i)
    {
        ESM::GameSetting gmst;
        gmst.mId = sStrings[i];
        gmst.blank();
        gmst.mValue.setType (ESM::VT_String);
        gmst.mValue.setString ("<no text>");
        addOptionalGmst (gmst);
    }
}

void CSMDoc::Document::addOptionalGlobals()
{
    static const char *sGlobals[] =
    {
        "DaysPassed",
        "PCWerewolf",
        "PCYear",
        0
    };

    for (int i=0; sGlobals[i]; ++i)
    {
        ESM::Global global;
        global.mId = sGlobals[i];
        global.blank();
        global.mValue.setType (ESM::VT_Long);

        if (i==0)
            global.mValue.setInteger (1); // dayspassed starts counting at 1

        addOptionalGlobal (global);
    }
}

void CSMDoc::Document::addOptionalMagicEffects()
{
    for (int i=ESM::MagicEffect::SummonFabricant; i<=ESM::MagicEffect::SummonCreature05; ++i)
    {
        ESM::MagicEffect effect;
        effect.mIndex = i;
        effect.mId = ESM::MagicEffect::indexToId (i);
        effect.blank();

        addOptionalMagicEffect (effect);
    }
}

void CSMDoc::Document::addOptionalGmst (const ESM::GameSetting& gmst)
{
    if (getData().getGmsts().searchId (gmst.mId)==-1)
    {
        CSMWorld::Record<ESM::GameSetting> record;
        record.mBase = gmst;
        record.mState = CSMWorld::RecordBase::State_BaseOnly;
        getData().getGmsts().appendRecord (record);
    }
}

void CSMDoc::Document::addOptionalGlobal (const ESM::Global& global)
{
    if (getData().getGlobals().searchId (global.mId)==-1)
    {
        CSMWorld::Record<ESM::Global> record;
        record.mBase = global;
        record.mState = CSMWorld::RecordBase::State_BaseOnly;
        getData().getGlobals().appendRecord (record);
    }
}

void CSMDoc::Document::addOptionalMagicEffect (const ESM::MagicEffect& magicEffect)
{
    if (getData().getMagicEffects().searchId (magicEffect.mId)==-1)
    {
        CSMWorld::Record<ESM::MagicEffect> record;
        record.mBase = magicEffect;
        record.mState = CSMWorld::RecordBase::State_BaseOnly;
        getData().getMagicEffects().appendRecord (record);
    }
}

void CSMDoc::Document::createBase()
{
    static const char *sGlobals[] =
    {
        "Day",
        "DaysPassed",
        "GameHour",
        "Month",
        "PCRace",
        "PCVampire",
        "PCWerewolf",
        "PCYear",
        0
    };

    for (int i=0; sGlobals[i]; ++i)
    {
        ESM::Global record;
        record.mId = sGlobals[i];
        record.mValue.setType (i==2 ? ESM::VT_Float : ESM::VT_Long);

        if (i==0 || i==1)
            record.mValue.setInteger (1);

        getData().getGlobals().add (record);
    }

    addGmsts();

    for (int i=0; i<27; ++i)
    {
        ESM::Skill record;
        record.mIndex = i;
        record.mId = ESM::Skill::indexToId (record.mIndex);
        record.blank();

        getData().getSkills().add (record);
    }

    static const char *sVoice[] =
    {
        "Intruder",
        "Attack",
        "Hello",
        "Thief",
        "Alarm",
        "Idle",
        "Flee",
        "Hit",
        0
    };

    for (int i=0; sVoice[i]; ++i)
    {
        ESM::Dialogue record;
        record.mId = sVoice[i];
        record.mType = ESM::Dialogue::Voice;
        record.blank();

        getData().getTopics().add (record);
    }

    static const char *sGreetings[] =
    {
        "Greeting 0",
        "Greeting 1",
        "Greeting 2",
        "Greeting 3",
        "Greeting 4",
        "Greeting 5",
        "Greeting 6",
        "Greeting 7",
        "Greeting 8",
        "Greeting 9",
        0
    };

    for (int i=0; sGreetings[i]; ++i)
    {
        ESM::Dialogue record;
        record.mId = sGreetings[i];
        record.mType = ESM::Dialogue::Greeting;
        record.blank();

        getData().getTopics().add (record);
    }

    static const char *sPersuasion[] =
    {
        "Intimidate Success",
        "Intimidate Fail",
        "Service Refusal",
        "Admire Success",
        "Taunt Success",
        "Bribe Success",
        "Info Refusal",
        "Admire Fail",
        "Taunt Fail",
        "Bribe Fail",
        0
    };

    for (int i=0; sPersuasion[i]; ++i)
    {
        ESM::Dialogue record;
        record.mId = sPersuasion[i];
        record.mType = ESM::Dialogue::Persuasion;
        record.blank();

        getData().getTopics().add (record);
    }

    for (int i=0; i<ESM::MagicEffect::Length; ++i)
    {
        ESM::MagicEffect record;

        record.mIndex = i;
        record.mId = ESM::MagicEffect::indexToId (i);

        record.blank();

        getData().getMagicEffects().add (record);
    }
}

CSMDoc::Document::Document (const Files::ConfigurationManager& configuration,
    const std::vector< boost::filesystem::path >& files, bool new_,
    const boost::filesystem::path& savePath, const boost::filesystem::path& resDir,
    ToUTF8::FromType encoding, const CSMWorld::ResourcesManager& resourcesManager,
    const std::vector<std::string>& blacklistedScripts)
: mSavePath (savePath), mContentFiles (files), mNew (new_), mData (encoding, resourcesManager),
  mTools (*this), mResDir(resDir),
  mProjectPath ((configuration.getUserDataPath() / "projects") /
  (savePath.filename().string() + ".project")),
  mSavingOperation (*this, mProjectPath, encoding),
  mSaving (&mSavingOperation),
  mRunner (mProjectPath), mPhysics(boost::shared_ptr<CSVWorld::PhysicsSystem>())
{
    if (mContentFiles.empty())
        throw std::runtime_error ("Empty content file sequence");

    if (!boost::filesystem::exists (mProjectPath))
    {
        boost::filesystem::path customFiltersPath (configuration.getUserDataPath());
        customFiltersPath /= "defaultfilters";

        std::ofstream destination (mProjectPath.string().c_str(), std::ios::binary);

        if (boost::filesystem::exists (customFiltersPath))
        {
            destination << std::ifstream(customFiltersPath.c_str(), std::ios::binary).rdbuf();
        }
        else
        {
            destination << std::ifstream(std::string(mResDir.string() + "/defaultfilters").c_str(), std::ios::binary).rdbuf();
        }
    }

    if (mNew)
    {
        mData.setDescription ("");
        mData.setAuthor ("");

        if (mContentFiles.size()==1)
            createBase();
    }

    mBlacklist.add (CSMWorld::UniversalId::Type_Script, blacklistedScripts);

    addOptionalGmsts();
    addOptionalGlobals();
    addOptionalMagicEffects();

    connect (&mUndoStack, SIGNAL (cleanChanged (bool)), this, SLOT (modificationStateChanged (bool)));

    connect (&mTools, SIGNAL (progress (int, int, int)), this, SLOT (progress (int, int, int)));
    connect (&mTools, SIGNAL (done (int, bool)), this, SLOT (operationDone (int, bool)));

    connect (&mSaving, SIGNAL (progress (int, int, int)), this, SLOT (progress (int, int, int)));
    connect (&mSaving, SIGNAL (done (int, bool)), this, SLOT (operationDone (int, bool)));

    connect (
        &mSaving, SIGNAL (reportMessage (const CSMWorld::UniversalId&, const std::string&, const std::string&, int)),
        this, SLOT (reportMessage (const CSMWorld::UniversalId&, const std::string&, const std::string&, int)));

    connect (&mRunner, SIGNAL (runStateChanged()), this, SLOT (runStateChanged()));
}

CSMDoc::Document::~Document()
{
}

QUndoStack& CSMDoc::Document::getUndoStack()
{
    return mUndoStack;
}

int CSMDoc::Document::getState() const
{
    int state = 0;

    if (!mUndoStack.isClean())
        state |= State_Modified;

    if (mSaving.isRunning())
        state |= State_Locked | State_Saving | State_Operation;

    if (mRunner.isRunning())
        state |= State_Locked | State_Running;

    if (int operations = mTools.getRunningOperations())
        state |= State_Locked | State_Operation | operations;

    return state;
}

const boost::filesystem::path& CSMDoc::Document::getSavePath() const
{
    return mSavePath;
}

const boost::filesystem::path& CSMDoc::Document::getProjectPath() const
{
    return mProjectPath;
}

const std::vector<boost::filesystem::path>& CSMDoc::Document::getContentFiles() const
{
    return mContentFiles;
}

bool CSMDoc::Document::isNew() const
{
    return mNew;
}

void CSMDoc::Document::save()
{
    if (mSaving.isRunning())
        throw std::logic_error (
            "Failed to initiate save, because a save operation is already running.");

    mSaving.start();

    emit stateChanged (getState(), this);
}

CSMWorld::UniversalId CSMDoc::Document::verify()
{
    CSMWorld::UniversalId id = mTools.runVerifier();
    emit stateChanged (getState(), this);
    return id;
}


CSMWorld::UniversalId CSMDoc::Document::newSearch()
{
    return mTools.newSearch();
}

void CSMDoc::Document::runSearch (const CSMWorld::UniversalId& searchId, const CSMTools::Search& search)
{
    mTools.runSearch (searchId, search);
    emit stateChanged (getState(), this);
}

void CSMDoc::Document::abortOperation (int type)
{
    if (type==State_Saving)
        mSaving.abort();
    else
        mTools.abortOperation (type);
}

void CSMDoc::Document::modificationStateChanged (bool clean)
{
    emit stateChanged (getState(), this);
}

void CSMDoc::Document::reportMessage (const CSMWorld::UniversalId& id, const std::string& message,
    const std::string& hint, int type)
{
    /// \todo find a better way to get these messages to the user.
    std::cout << message << std::endl;
}

void CSMDoc::Document::operationDone (int type, bool failed)
{
    emit stateChanged (getState(), this);
}

const CSMWorld::Data& CSMDoc::Document::getData() const
{
    return mData;
}

CSMWorld::Data& CSMDoc::Document::getData()
{
    return mData;
}

CSMTools::ReportModel *CSMDoc::Document::getReport (const CSMWorld::UniversalId& id)
{
    return mTools.getReport (id);
}

bool CSMDoc::Document::isBlacklisted (const CSMWorld::UniversalId& id)
    const
{
    return mBlacklist.isBlacklisted (id);
}

void CSMDoc::Document::startRunning (const std::string& profile,
    const std::string& startupInstruction)
{
    std::vector<std::string> contentFiles;

    for (std::vector<boost::filesystem::path>::const_iterator iter (mContentFiles.begin());
        iter!=mContentFiles.end(); ++iter)
        contentFiles.push_back (iter->filename().string());

    mRunner.configure (getData().getDebugProfiles().getRecord (profile).get(), contentFiles,
        startupInstruction);

    int state = getState();

    if (state & State_Modified)
    {
        // need to save first
        mRunner.start (true);

        new SaveWatcher (&mRunner, &mSaving); // no, that is not a memory leak. Qt is weird.

        if (!(state & State_Saving))
            save();
    }
    else
        mRunner.start();
}

void CSMDoc::Document::stopRunning()
{
    mRunner.stop();
}

QTextDocument *CSMDoc::Document::getRunLog()
{
    return mRunner.getLog();
}

void CSMDoc::Document::runStateChanged()
{
    emit stateChanged (getState(), this);
}

void CSMDoc::Document::progress (int current, int max, int type)
{
    emit progress (current, max, type, 1, this);
}

boost::shared_ptr<CSVWorld::PhysicsSystem> CSMDoc::Document::getPhysics ()
{
    if(!mPhysics)
        mPhysics = boost::shared_ptr<CSVWorld::PhysicsSystem> (new CSVWorld::PhysicsSystem());

    return mPhysics;
}
