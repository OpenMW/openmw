#include "extensions0.hpp"

#include "opcodes.hpp"
#include "extensions.hpp"

namespace Compiler
{
    void registerExtensions (Extensions& extensions, bool consoleOnly)
    {
        Ai::registerExtensions (extensions);
        Animation::registerExtensions (extensions);
        Cell::registerExtensions (extensions);
        Container::registerExtensions (extensions);
        Control::registerExtensions (extensions);
        Dialogue::registerExtensions (extensions);
        Gui::registerExtensions (extensions);
        Misc::registerExtensions (extensions);
        Sky::registerExtensions (extensions);
        Sound::registerExtensions (extensions);
        Stats::registerExtensions (extensions);
        Transformation::registerExtensions (extensions);

        if (consoleOnly)
        {
            Console::registerExtensions (extensions);
            User::registerExtensions (extensions);
        }
    }

    namespace Ai
    {
        void registerExtensions (Extensions& extensions)
        {
            extensions.registerInstruction ("aiactivate", "c/l", opcodeAIActivate,
                opcodeAIActivateExplicit);
            extensions.registerInstruction ("aitravel", "fff/lx", opcodeAiTravel,
                opcodeAiTravelExplicit);
            extensions.registerInstruction ("aiescort", "cffff/l", opcodeAiEscort,
                opcodeAiEscortExplicit);
            extensions.registerInstruction ("aiescortcell", "ccffff/l", opcodeAiEscortCell,
                opcodeAiEscortCellExplicit);
            extensions.registerInstruction ("aiwander", "fff/llllllllll", opcodeAiWander,
                opcodeAiWanderExplicit);
            extensions.registerInstruction ("aifollow", "cffff/llllllll", opcodeAiFollow,
                opcodeAiFollowExplicit);
            extensions.registerInstruction ("aifollowcell", "ccffff/l", opcodeAiFollowCell,
                opcodeAiFollowCellExplicit);
            extensions.registerFunction ("getaipackagedone", 'l', "", opcodeGetAiPackageDone,
                opcodeGetAiPackageDoneExplicit);
            extensions.registerFunction ("getcurrentaipackage", 'l', "", opcodeGetCurrentAiPackage,
                opcodeGetCurrentAiPackageExplicit);
            extensions.registerFunction ("getdetected", 'l', "c", opcodeGetDetected,
                opcodeGetDetectedExplicit);
            extensions.registerInstruction ("sethello", "l", opcodeSetHello, opcodeSetHelloExplicit);
            extensions.registerInstruction ("setfight", "l", opcodeSetFight, opcodeSetFightExplicit);
            extensions.registerInstruction ("setflee", "l", opcodeSetFlee, opcodeSetFleeExplicit);
            extensions.registerInstruction ("setalarm", "l", opcodeSetAlarm, opcodeSetAlarmExplicit);
            extensions.registerInstruction ("modhello", "l", opcodeModHello, opcodeModHelloExplicit);
            extensions.registerInstruction ("modfight", "l", opcodeModFight, opcodeModFightExplicit);
            extensions.registerInstruction ("modflee", "l", opcodeModFlee, opcodeModFleeExplicit);
            extensions.registerInstruction ("modalarm", "l", opcodeModAlarm, opcodeModAlarmExplicit);
            extensions.registerInstruction ("toggleai", "", opcodeToggleAI);
            extensions.registerInstruction ("tai", "", opcodeToggleAI);
            extensions.registerInstruction("startcombat", "c", opcodeStartCombat, opcodeStartCombatExplicit);
            extensions.registerInstruction("stopcombat", "x", opcodeStopCombat, opcodeStopCombatExplicit);
            extensions.registerFunction ("gethello", 'l', "", opcodeGetHello, opcodeGetHelloExplicit);
            extensions.registerFunction ("getfight", 'l', "", opcodeGetFight, opcodeGetFightExplicit);
            extensions.registerFunction ("getflee", 'l', "", opcodeGetFlee, opcodeGetFleeExplicit);
            extensions.registerFunction ("getalarm", 'l', "", opcodeGetAlarm, opcodeGetAlarmExplicit);
            extensions.registerFunction ("getlineofsight", 'l', "c", opcodeGetLineOfSight, opcodeGetLineOfSightExplicit);
            extensions.registerFunction ("getlos", 'l', "c", opcodeGetLineOfSight, opcodeGetLineOfSightExplicit);
            extensions.registerFunction("gettarget", 'l', "c", opcodeGetTarget, opcodeGetTargetExplicit);
            extensions.registerInstruction("face", "ffX", opcodeFace, opcodeFaceExplicit);
        }
    }

    namespace Animation
    {
        void registerExtensions (Extensions& extensions)
        {
            extensions.registerInstruction ("skipanim", "", opcodeSkipAnim, opcodeSkipAnimExplicit);
            extensions.registerInstruction ("playgroup", "c/l", opcodePlayAnim, opcodePlayAnimExplicit);
            extensions.registerInstruction ("loopgroup", "cl/l", opcodeLoopAnim, opcodeLoopAnimExplicit);
        }
    }

    namespace Cell
    {
        void registerExtensions (Extensions& extensions)
        {
            extensions.registerFunction ("cellchanged", 'l', "", opcodeCellChanged);
            extensions.registerInstruction("testcells", "", opcodeTestCells);
            extensions.registerInstruction("testinteriorcells", "", opcodeTestInteriorCells);
            extensions.registerInstruction ("coc", "S", opcodeCOC);
            extensions.registerInstruction ("centeroncell", "S", opcodeCOC);
            extensions.registerInstruction ("coe", "ll", opcodeCOE);
            extensions.registerInstruction ("centeronexterior", "ll", opcodeCOE);
            extensions.registerInstruction ("setwaterlevel", "f", opcodeSetWaterLevel);
            extensions.registerInstruction ("modwaterlevel", "f", opcodeModWaterLevel);
            extensions.registerFunction ("getinterior", 'l', "", opcodeGetInterior);
            extensions.registerFunction ("getpccell", 'l', "c", opcodeGetPCCell);
            extensions.registerFunction ("getwaterlevel", 'f', "", opcodeGetWaterLevel);
        }
    }

    namespace Console
    {
        void registerExtensions (Extensions& extensions)
        {

        }
    }

    namespace Container
    {
        void registerExtensions (Extensions& extensions)
        {
            extensions.registerInstruction ("additem", "clX", opcodeAddItem, opcodeAddItemExplicit);
            extensions.registerFunction ("getitemcount", 'l', "cX", opcodeGetItemCount,
                opcodeGetItemCountExplicit);
            extensions.registerInstruction ("removeitem", "clX", opcodeRemoveItem,
                opcodeRemoveItemExplicit);
            extensions.registerInstruction ("equip", "cX", opcodeEquip, opcodeEquipExplicit);
            extensions.registerFunction ("getarmortype", 'l', "l", opcodeGetArmorType, opcodeGetArmorTypeExplicit);
            extensions.registerFunction ("hasitemequipped", 'l', "c", opcodeHasItemEquipped, opcodeHasItemEquippedExplicit);
            extensions.registerFunction ("hassoulgem", 'l', "c", opcodeHasSoulGem, opcodeHasSoulGemExplicit);
            extensions.registerFunction ("getweapontype", 'l', "", opcodeGetWeaponType, opcodeGetWeaponTypeExplicit);
        }
    }

    namespace Control
    {
        void registerExtensions (Extensions& extensions)
        {
            std::string enable ("enable");
            std::string disable ("disable");

            for (int i=0; i<numberOfControls; ++i)
            {
                extensions.registerInstruction (enable + controls[i], "", opcodeEnable+i);
                extensions.registerInstruction (disable + controls[i], "", opcodeDisable+i);
                extensions.registerFunction (std::string("get") + controls[i] + std::string("disabled"), 'l', "", opcodeGetDisabled+i);
            }

            extensions.registerInstruction ("togglecollision", "", opcodeToggleCollision);
            extensions.registerInstruction ("tcl", "", opcodeToggleCollision);

            extensions.registerInstruction ("clearforcerun", "", opcodeClearForceRun,
                opcodeClearForceRunExplicit);
            extensions.registerInstruction ("forcerun", "", opcodeForceRun,
                opcodeForceRunExplicit);

            extensions.registerInstruction ("clearforcejump", "", opcodeClearForceJump,
                opcodeClearForceJumpExplicit);
            extensions.registerInstruction ("forcejump", "", opcodeForceJump,
                opcodeForceJumpExplicit);

            extensions.registerInstruction ("clearforcemovejump", "", opcodeClearForceMoveJump,
                opcodeClearForceMoveJumpExplicit);
            extensions.registerInstruction ("forcemovejump", "", opcodeForceMoveJump,
                opcodeForceMoveJumpExplicit);

            extensions.registerInstruction ("clearforcesneak", "", opcodeClearForceSneak,
                opcodeClearForceSneakExplicit);
            extensions.registerInstruction ("forcesneak", "", opcodeForceSneak,
                opcodeForceSneakExplicit);
            extensions.registerFunction ("getpcrunning", 'l', "", opcodeGetPcRunning);
            extensions.registerFunction ("getpcsneaking", 'l', "", opcodeGetPcSneaking);
            extensions.registerFunction ("getforcerun", 'l', "", opcodeGetForceRun, opcodeGetForceRunExplicit);
            extensions.registerFunction ("getforcejump", 'l', "", opcodeGetForceJump, opcodeGetForceJumpExplicit);
            extensions.registerFunction ("getforcemovejump", 'l', "", opcodeGetForceMoveJump, opcodeGetForceMoveJumpExplicit);
            extensions.registerFunction ("getforcesneak", 'l', "", opcodeGetForceSneak, opcodeGetForceSneakExplicit);
        }
    }

    namespace Dialogue
    {
        void registerExtensions (Extensions& extensions)
        {
            extensions.registerInstruction ("journal", "cl", opcodeJournal, opcodeJournalExplicit);
            extensions.registerInstruction ("setjournalindex", "cl", opcodeSetJournalIndex);
            extensions.registerFunction ("getjournalindex", 'l', "c", opcodeGetJournalIndex);
            extensions.registerInstruction ("addtopic", "S" , opcodeAddTopic);
            extensions.registerInstruction ("choice", "j/SlSlSlSlSlSlSlSlSlSlSlSlSlSlSlSlSlSlSlSlSlSlSlSl", opcodeChoice);
            extensions.registerInstruction("forcegreeting","z",opcodeForceGreeting,
                opcodeForceGreetingExplicit);
            extensions.registerInstruction("goodbye", "", opcodeGoodbye);
            extensions.registerInstruction("setreputation", "l", opcodeSetReputation,
                opcodeSetReputationExplicit);
            extensions.registerInstruction("modreputation", "l", opcodeModReputation,
                opcodeModReputationExplicit);
            extensions.registerFunction("getreputation", 'l', "", opcodeGetReputation,
                opcodeGetReputationExplicit);
            extensions.registerFunction("samefaction", 'l', "", opcodeSameFaction,
                opcodeSameFactionExplicit);
            extensions.registerInstruction("modfactionreaction", "ccl", opcodeModFactionReaction);
            extensions.registerInstruction("setfactionreaction", "ccl", opcodeSetFactionReaction);
            extensions.registerFunction("getfactionreaction", 'l', "ccX", opcodeGetFactionReaction);
            extensions.registerInstruction("clearinfoactor", "", opcodeClearInfoActor, opcodeClearInfoActorExplicit);
        }
    }

    namespace Gui
    {
        void registerExtensions (Extensions& extensions)
        {
            extensions.registerInstruction ("enablebirthmenu", "", opcodeEnableBirthMenu);
            extensions.registerInstruction ("enableclassmenu", "", opcodeEnableClassMenu);
            extensions.registerInstruction ("enablenamemenu", "", opcodeEnableNameMenu);
            extensions.registerInstruction ("enableracemenu", "", opcodeEnableRaceMenu);
            extensions.registerInstruction ("enablestatreviewmenu", "",
                opcodeEnableStatsReviewMenu);

            extensions.registerInstruction ("enableinventorymenu", "", opcodeEnableInventoryMenu);
            extensions.registerInstruction ("enablemagicmenu", "", opcodeEnableMagicMenu);
            extensions.registerInstruction ("enablemapmenu", "", opcodeEnableMapMenu);
            extensions.registerInstruction ("enablestatsmenu", "", opcodeEnableStatsMenu);

            extensions.registerInstruction ("enablerest", "", opcodeEnableRest);
            extensions.registerInstruction ("enablelevelupmenu", "", opcodeEnableLevelupMenu);

            extensions.registerInstruction ("showrestmenu", "", opcodeShowRestMenu, opcodeShowRestMenuExplicit);

            extensions.registerFunction ("getbuttonpressed", 'l', "", opcodeGetButtonPressed);

            extensions.registerInstruction ("togglefogofwar", "", opcodeToggleFogOfWar);
            extensions.registerInstruction ("tfow", "", opcodeToggleFogOfWar);

            extensions.registerInstruction ("togglefullhelp", "", opcodeToggleFullHelp);
            extensions.registerInstruction ("tfh", "", opcodeToggleFullHelp);

            extensions.registerInstruction ("showmap", "Sxxxx", opcodeShowMap);
            extensions.registerInstruction ("fillmap", "", opcodeFillMap);
            extensions.registerInstruction ("menutest", "/l", opcodeMenuTest);
            extensions.registerInstruction ("togglemenus", "", opcodeToggleMenus);
            extensions.registerInstruction ("tm", "", opcodeToggleMenus);
        }
    }

    namespace Misc
    {
        void registerExtensions (Extensions& extensions)
        {
            extensions.registerFunction ("xbox", 'l', "", opcodeXBox);
            extensions.registerFunction ("onactivate", 'l', "", opcodeOnActivate, opcodeOnActivateExplicit);
            extensions.registerInstruction ("activate", "x", opcodeActivate, opcodeActivateExplicit);
            extensions.registerInstruction ("lock", "/l", opcodeLock, opcodeLockExplicit);
            extensions.registerInstruction ("unlock", "", opcodeUnlock, opcodeUnlockExplicit);
            extensions.registerInstruction ("cast", "SS", opcodeCast, opcodeCastExplicit);
            extensions.registerInstruction ("explodespell", "S", opcodeExplodeSpell, opcodeExplodeSpellExplicit);
            extensions.registerInstruction ("togglecollisionboxes", "", opcodeToggleCollisionBoxes);
            extensions.registerInstruction ("togglecollisiongrid", "", opcodeToggleCollisionDebug);
            extensions.registerInstruction ("tcb", "", opcodeToggleCollisionBoxes);
            extensions.registerInstruction ("tcg", "", opcodeToggleCollisionDebug);
            extensions.registerInstruction ("twf", "", opcodeToggleWireframe);
            extensions.registerInstruction ("togglewireframe", "", opcodeToggleWireframe);
            extensions.registerInstruction ("fadein", "f", opcodeFadeIn);
            extensions.registerInstruction ("fadeout", "f", opcodeFadeOut);
            extensions.registerInstruction ("fadeto", "ff", opcodeFadeTo);
            extensions.registerInstruction ("togglewater", "", opcodeToggleWater);
            extensions.registerInstruction ("twa", "", opcodeToggleWater);
            extensions.registerInstruction ("toggleworld", "", opcodeToggleWorld);
            extensions.registerInstruction ("tw", "", opcodeToggleWorld);
            extensions.registerInstruction ("togglepathgrid", "", opcodeTogglePathgrid);
            extensions.registerInstruction ("tpg", "", opcodeTogglePathgrid);
            extensions.registerInstruction ("dontsaveobject", "", opcodeDontSaveObject);
            extensions.registerInstruction ("pcforce1stperson", "", opcodePcForce1stPerson);
            extensions.registerInstruction ("pcforce3rdperson", "", opcodePcForce3rdPerson);
            extensions.registerFunction ("pcget3rdperson", 'l', "", opcodePcGet3rdPerson);
            extensions.registerInstruction ("togglevanitymode", "", opcodeToggleVanityMode);
            extensions.registerInstruction ("tvm", "", opcodeToggleVanityMode);
            extensions.registerFunction ("getpcsleep", 'l', "", opcodeGetPcSleep);
            extensions.registerFunction ("getpcjumping", 'l', "", opcodeGetPcJumping);
            extensions.registerInstruction ("wakeuppc", "", opcodeWakeUpPc);
            extensions.registerInstruction ("playbink", "Sl", opcodePlayBink);
            extensions.registerInstruction ("payfine", "", opcodePayFine);
            extensions.registerInstruction ("payfinethief", "", opcodePayFineThief);
            extensions.registerInstruction ("gotojail", "", opcodeGoToJail);
            extensions.registerFunction ("getlocked", 'l', "", opcodeGetLocked, opcodeGetLockedExplicit);
            extensions.registerFunction ("geteffect", 'l', "S", opcodeGetEffect, opcodeGetEffectExplicit);
            extensions.registerInstruction ("addsoulgem", "ccX", opcodeAddSoulGem, opcodeAddSoulGemExplicit);
            extensions.registerInstruction ("removesoulgem", "c/l", opcodeRemoveSoulGem, opcodeRemoveSoulGemExplicit);
            extensions.registerInstruction ("drop", "cl", opcodeDrop, opcodeDropExplicit);
            extensions.registerInstruction ("dropsoulgem", "c", opcodeDropSoulGem, opcodeDropSoulGemExplicit);
            extensions.registerFunction ("getattacked", 'l', "", opcodeGetAttacked, opcodeGetAttackedExplicit);
            extensions.registerFunction ("getweapondrawn", 'l', "", opcodeGetWeaponDrawn, opcodeGetWeaponDrawnExplicit);
            extensions.registerFunction ("getspellreadied", 'l', "", opcodeGetSpellReadied, opcodeGetSpellReadiedExplicit);
            extensions.registerFunction ("getspelleffects", 'l', "c", opcodeGetSpellEffects, opcodeGetSpellEffectsExplicit);
            extensions.registerFunction ("getcurrenttime", 'f', "", opcodeGetCurrentTime);
            extensions.registerInstruction ("setdelete", "l", opcodeSetDelete, opcodeSetDeleteExplicit);
            extensions.registerFunction ("getsquareroot", 'f', "f", opcodeGetSquareRoot);
            extensions.registerInstruction ("fall", "", opcodeFall, opcodeFallExplicit);
            extensions.registerFunction ("getstandingpc", 'l', "", opcodeGetStandingPc, opcodeGetStandingPcExplicit);
            extensions.registerFunction ("getstandingactor", 'l', "", opcodeGetStandingActor, opcodeGetStandingActorExplicit);
            extensions.registerFunction ("getcollidingpc", 'l', "", opcodeGetCollidingPc, opcodeGetCollidingPcExplicit);
            extensions.registerFunction ("getcollidingactor", 'l', "", opcodeGetCollidingActor, opcodeGetCollidingActorExplicit);
            extensions.registerInstruction ("hurtstandingactor", "f", opcodeHurtStandingActor, opcodeHurtStandingActorExplicit);
            extensions.registerInstruction ("hurtcollidingactor", "f", opcodeHurtCollidingActor, opcodeHurtCollidingActorExplicit);
            extensions.registerFunction ("getwindspeed", 'f', "", opcodeGetWindSpeed);
            extensions.registerFunction ("hitonme", 'l', "S", opcodeHitOnMe, opcodeHitOnMeExplicit);
            extensions.registerFunction ("hitattemptonme", 'l', "S", opcodeHitAttemptOnMe, opcodeHitAttemptOnMeExplicit);
            extensions.registerInstruction ("disableteleporting", "", opcodeDisableTeleporting);
            extensions.registerInstruction ("enableteleporting", "", opcodeEnableTeleporting);
            extensions.registerInstruction ("showvars", "", opcodeShowVars, opcodeShowVarsExplicit);
            extensions.registerInstruction ("show", "c", opcodeShow, opcodeShowExplicit);
            extensions.registerInstruction ("sv", "", opcodeShowVars, opcodeShowVarsExplicit);
            extensions.registerInstruction("tgm", "", opcodeToggleGodMode);
            extensions.registerInstruction("togglegodmode", "", opcodeToggleGodMode);
            extensions.registerInstruction("togglescripts", "", opcodeToggleScripts);
            extensions.registerInstruction ("disablelevitation", "", opcodeDisableLevitation);
            extensions.registerInstruction ("enablelevitation", "", opcodeEnableLevitation);
            extensions.registerFunction ("getpcinjail", 'l', "", opcodeGetPcInJail);
            extensions.registerFunction ("getpctraveling", 'l', "", opcodeGetPcTraveling);
            extensions.registerInstruction ("betacomment", "/S", opcodeBetaComment, opcodeBetaCommentExplicit);
            extensions.registerInstruction ("bc", "/S", opcodeBetaComment, opcodeBetaCommentExplicit);
            extensions.registerInstruction ("ori", "/S", opcodeBetaComment, opcodeBetaCommentExplicit); // 'ori' stands for 'ObjectReferenceInfo'
            extensions.registerInstruction ("showscenegraph", "/l", opcodeShowSceneGraph, opcodeShowSceneGraphExplicit);
            extensions.registerInstruction ("ssg", "/l", opcodeShowSceneGraph, opcodeShowSceneGraphExplicit);
            extensions.registerInstruction ("addtolevcreature", "ccl", opcodeAddToLevCreature);
            extensions.registerInstruction ("removefromlevcreature", "ccl", opcodeRemoveFromLevCreature);
            extensions.registerInstruction ("addtolevitem", "ccl", opcodeAddToLevItem);
            extensions.registerInstruction ("removefromlevitem", "ccl", opcodeRemoveFromLevItem);
            extensions.registerInstruction ("tb", "", opcodeToggleBorders);
            extensions.registerInstruction ("toggleborders", "", opcodeToggleBorders);
            extensions.registerInstruction ("togglenavmesh", "", opcodeToggleNavMesh);
            extensions.registerInstruction ("tap", "", opcodeToggleActorsPaths);
            extensions.registerInstruction ("toggleactorspaths", "", opcodeToggleActorsPaths);
            extensions.registerInstruction ("setnavmeshnumber", "l", opcodeSetNavMeshNumberToRender);
            extensions.registerFunction ("repairedonme", 'l', "S", opcodeRepairedOnMe, opcodeRepairedOnMeExplicit);
            extensions.registerInstruction ("togglerecastmesh", "", opcodeToggleRecastMesh);
        }
    }

    namespace Sky
    {
        void registerExtensions (Extensions& extensions)
        {
            extensions.registerInstruction ("togglesky", "", opcodeToggleSky);
            extensions.registerInstruction ("ts", "", opcodeToggleSky);
            extensions.registerInstruction ("turnmoonwhite", "", opcodeTurnMoonWhite);
            extensions.registerInstruction ("turnmoonred", "", opcodeTurnMoonRed);
            extensions.registerInstruction ("changeweather", "Sl", opcodeChangeWeather);
            extensions.registerFunction ("getmasserphase", 'l', "", opcodeGetMasserPhase);
            extensions.registerFunction ("getsecundaphase", 'l', "", opcodeGetSecundaPhase);
            extensions.registerFunction ("getcurrentweather", 'l', "", opcodeGetCurrentWeather);
            extensions.registerInstruction ("modregion", "S/llllllllllX", opcodeModRegion);
        }
    }

    namespace Sound
    {
        void registerExtensions (Extensions& extensions)
        {
            extensions.registerInstruction ("say", "SS", opcodeSay, opcodeSayExplicit);
            extensions.registerFunction ("saydone", 'l', "", opcodeSayDone, opcodeSayDoneExplicit);
            extensions.registerInstruction ("streammusic", "S", opcodeStreamMusic);
            extensions.registerInstruction ("playsound", "cXX", opcodePlaySound);
            extensions.registerInstruction ("playsoundvp", "cff", opcodePlaySoundVP);
            extensions.registerInstruction ("playsound3d", "cXX", opcodePlaySound3D,
                opcodePlaySound3DExplicit);
            extensions.registerInstruction ("playsound3dvp", "cff", opcodePlaySound3DVP,
                opcodePlaySound3DVPExplicit);
            extensions.registerInstruction ("playloopsound3d", "cXX", opcodePlayLoopSound3D,
                opcodePlayLoopSound3DExplicit);
            extensions.registerInstruction ("playloopsound3dvp", "cff", opcodePlayLoopSound3DVP,
                opcodePlayLoopSound3DVPExplicit);
            extensions.registerInstruction ("stopsound", "cXX", opcodeStopSound,
                opcodeStopSoundExplicit);
            extensions.registerFunction ("getsoundplaying", 'l', "c", opcodeGetSoundPlaying,
                opcodeGetSoundPlayingExplicit);
        }
    }

    namespace Stats
    {
        void registerExtensions (Extensions& extensions)
        {
            static const char *attributes[numberOfAttributes] =
            {
                "strength", "intelligence", "willpower", "agility", "speed", "endurance",
                "personality", "luck"
            };

            static const char *dynamics[numberOfDynamics] =
            {
                "health", "magicka", "fatigue"
            };

            static const char *skills[numberOfSkills] =
            {
                "block", "armorer", "mediumarmor", "heavyarmor", "bluntweapon",
                "longblade", "axe", "spear", "athletics", "enchant", "destruction",
                "alteration", "illusion", "conjuration", "mysticism",
                "restoration", "alchemy", "unarmored", "security", "sneak",
                "acrobatics", "lightarmor", "shortblade", "marksman",
                "mercantile", "speechcraft", "handtohand"
            };

            static const char *magicEffects[numberOfMagicEffects] =
            {
                "resistmagicka", "resistfire", "resistfrost", "resistshock",
                "resistdisease", "resistblight", "resistcorprus", "resistpoison",
                "resistparalysis", "resistnormalweapons", "waterbreathing", "chameleon",
                "waterwalking", "swimspeed", "superjump", "flying",
                "armorbonus", "castpenalty", "silence", "blindness",
                "paralysis", "invisible", "attackbonus", "defendbonus"
            };

            std::string get ("get");
            std::string set ("set");
            std::string mod ("mod");
            std::string modCurrent ("modcurrent");
            std::string getRatio ("getratio");

            for (int i=0; i<numberOfAttributes; ++i)
            {
                extensions.registerFunction (get + attributes[i], 'l', "",
                    opcodeGetAttribute+i, opcodeGetAttributeExplicit+i);

                extensions.registerInstruction (set + attributes[i], "l",
                    opcodeSetAttribute+i, opcodeSetAttributeExplicit+i);

                extensions.registerInstruction (mod + attributes[i], "l",
                    opcodeModAttribute+i, opcodeModAttributeExplicit+i);
            }

            for (int i=0; i<numberOfDynamics; ++i)
            {
                extensions.registerFunction (get + dynamics[i], 'f', "x",
                    opcodeGetDynamic+i, opcodeGetDynamicExplicit+i);

                extensions.registerInstruction (set + dynamics[i], "f",
                    opcodeSetDynamic+i, opcodeSetDynamicExplicit+i);

                extensions.registerInstruction (mod + dynamics[i], "f",
                    opcodeModDynamic+i, opcodeModDynamicExplicit+i);

                extensions.registerInstruction (modCurrent + dynamics[i], "f",
                    opcodeModCurrentDynamic+i, opcodeModCurrentDynamicExplicit+i);

                extensions.registerFunction (get + dynamics[i] + getRatio, 'f', "",
                    opcodeGetDynamicGetRatio+i, opcodeGetDynamicGetRatioExplicit+i);
            }

            for (int i=0; i<numberOfSkills; ++i)
            {
                extensions.registerFunction (get + skills[i], 'l', "",
                    opcodeGetSkill+i, opcodeGetSkillExplicit+i);

                extensions.registerInstruction (set + skills[i], "l",
                    opcodeSetSkill+i, opcodeSetSkillExplicit+i);

                extensions.registerInstruction (mod + skills[i], "l",
                    opcodeModSkill+i, opcodeModSkillExplicit+i);
            }

            for (int i=0; i<numberOfMagicEffects; ++i)
            {
                extensions.registerFunction (get + magicEffects[i], 'l', "",
                    opcodeGetMagicEffect+i, opcodeGetMagicEffectExplicit+i);

                extensions.registerInstruction (set + magicEffects[i], "l",
                    opcodeSetMagicEffect+i, opcodeSetMagicEffectExplicit+i);

                extensions.registerInstruction(mod + magicEffects[i], "l",
                    opcodeModMagicEffect+i, opcodeModMagicEffectExplicit+i);
            }

            extensions.registerFunction ("getpccrimelevel", 'f', "", opcodeGetPCCrimeLevel);
            extensions.registerInstruction ("setpccrimelevel", "f", opcodeSetPCCrimeLevel);
            extensions.registerInstruction ("modpccrimelevel", "f", opcodeModPCCrimeLevel);

            extensions.registerInstruction ("addspell", "cz", opcodeAddSpell, opcodeAddSpellExplicit);
            extensions.registerInstruction ("removespell", "cz", opcodeRemoveSpell,
                opcodeRemoveSpellExplicit);
            extensions.registerInstruction ("removespelleffects", "c", opcodeRemoveSpellEffects,
                opcodeRemoveSpellEffectsExplicit);
            extensions.registerInstruction ("removeeffects", "l", opcodeRemoveEffects,
                opcodeRemoveEffectsExplicit);
            extensions.registerInstruction ("resurrect", "", opcodeResurrect,
                opcodeResurrectExplicit);
            extensions.registerFunction ("getspell", 'l', "c", opcodeGetSpell, opcodeGetSpellExplicit);

            extensions.registerInstruction("pcraiserank","/S",opcodePCRaiseRank, opcodePCRaiseRankExplicit);
            extensions.registerInstruction("pclowerrank","/S",opcodePCLowerRank, opcodePCLowerRankExplicit);
            extensions.registerInstruction("pcjoinfaction","/S",opcodePCJoinFaction, opcodePCJoinFactionExplicit);
            extensions.registerInstruction ("moddisposition","l",opcodeModDisposition,
                opcodeModDispositionExplicit);
            extensions.registerInstruction ("setdisposition","l",opcodeSetDisposition,
                opcodeSetDispositionExplicit);
            extensions.registerFunction ("getdisposition",'l', "",opcodeGetDisposition,
                opcodeGetDispositionExplicit);
            extensions.registerFunction("getpcrank",'l',"/S",opcodeGetPCRank,opcodeGetPCRankExplicit);

            extensions.registerInstruction("setlevel", "l", opcodeSetLevel, opcodeSetLevelExplicit);
            extensions.registerFunction("getlevel", 'l', "", opcodeGetLevel, opcodeGetLevelExplicit);

            extensions.registerFunction("getstat", 'l', "c", opcodeGetStat, opcodeGetStatExplicit);

            extensions.registerFunction ("getdeadcount", 'l', "c", opcodeGetDeadCount);

            extensions.registerFunction ("getpcfacrep", 'l', "/c", opcodeGetPCFacRep, opcodeGetPCFacRepExplicit);
            extensions.registerInstruction ("setpcfacrep", "l/c", opcodeSetPCFacRep, opcodeSetPCFacRepExplicit);
            extensions.registerInstruction ("modpcfacrep", "l/c", opcodeModPCFacRep, opcodeModPCFacRepExplicit);

            extensions.registerFunction ("getcommondisease", 'l', "", opcodeGetCommonDisease,
                opcodeGetCommonDiseaseExplicit);
            extensions.registerFunction ("getblightdisease", 'l', "", opcodeGetBlightDisease,
                opcodeGetBlightDiseaseExplicit);

            extensions.registerFunction ("getrace", 'l', "c", opcodeGetRace,
                opcodeGetRaceExplicit);
            extensions.registerFunction ("getwerewolfkills", 'l', "", opcodeGetWerewolfKills);
            extensions.registerFunction ("pcexpelled", 'l', "/S", opcodePcExpelled, opcodePcExpelledExplicit);
            extensions.registerInstruction ("pcexpell", "/S", opcodePcExpell, opcodePcExpellExplicit);
            extensions.registerInstruction ("pcclearexpelled", "/S", opcodePcClearExpelled, opcodePcClearExpelledExplicit);
            extensions.registerInstruction ("raiserank", "x", opcodeRaiseRank, opcodeRaiseRankExplicit);
            extensions.registerInstruction ("lowerrank", "x", opcodeLowerRank, opcodeLowerRankExplicit);

            extensions.registerFunction ("ondeath", 'l', "", opcodeOnDeath, opcodeOnDeathExplicit);
            extensions.registerFunction ("onmurder", 'l', "", opcodeOnMurder, opcodeOnMurderExplicit);
            extensions.registerFunction ("onknockout", 'l', "", opcodeOnKnockout, opcodeOnKnockoutExplicit);

            extensions.registerFunction ("iswerewolf", 'l', "", opcodeIsWerewolf, opcodeIsWerewolfExplicit);

            extensions.registerInstruction("becomewerewolf", "", opcodeBecomeWerewolf, opcodeBecomeWerewolfExplicit);
            extensions.registerInstruction("undowerewolf", "", opcodeUndoWerewolf, opcodeUndoWerewolfExplicit);
            extensions.registerInstruction("setwerewolfacrobatics", "", opcodeSetWerewolfAcrobatics, opcodeSetWerewolfAcrobaticsExplicit);
        }
    }

    namespace Transformation
    {
        void registerExtensions (Extensions& extensions)
        {
            extensions.registerInstruction("setscale","f",opcodeSetScale,opcodeSetScaleExplicit);
            extensions.registerFunction("getscale",'f',"",opcodeGetScale,opcodeGetScaleExplicit);
            extensions.registerInstruction("setangle","cf",opcodeSetAngle,opcodeSetAngleExplicit);
            extensions.registerFunction("getangle",'f',"c",opcodeGetAngle,opcodeGetAngleExplicit);
            extensions.registerInstruction("setpos","cf",opcodeSetPos,opcodeSetPosExplicit);
            extensions.registerFunction("getpos",'f',"c",opcodeGetPos,opcodeGetPosExplicit);
            extensions.registerFunction("getstartingpos",'f',"c",opcodeGetStartingPos,opcodeGetStartingPosExplicit);
            extensions.registerInstruction("position","ffffz",opcodePosition,opcodePositionExplicit);
            extensions.registerInstruction("positioncell","ffffcX",opcodePositionCell,opcodePositionCellExplicit);
            extensions.registerInstruction("placeitemcell","ccffffX",opcodePlaceItemCell);
            extensions.registerInstruction("placeitem","cffffX",opcodePlaceItem);
            extensions.registerInstruction("placeatpc","clflX",opcodePlaceAtPc);
            extensions.registerInstruction("placeatme","clflX",opcodePlaceAtMe,opcodePlaceAtMeExplicit);
            extensions.registerInstruction("modscale","f",opcodeModScale,opcodeModScaleExplicit);
            extensions.registerInstruction("rotate","cf",opcodeRotate,opcodeRotateExplicit);
            extensions.registerInstruction("rotateworld","cf",opcodeRotateWorld,opcodeRotateWorldExplicit);
            extensions.registerInstruction("setatstart","",opcodeSetAtStart,opcodeSetAtStartExplicit);
            extensions.registerInstruction("move","cf",opcodeMove,opcodeMoveExplicit);
            extensions.registerInstruction("moveworld","cf",opcodeMoveWorld,opcodeMoveWorldExplicit);
            extensions.registerFunction("getstartingangle",'f',"c",opcodeGetStartingAngle,opcodeGetStartingAngleExplicit);
            extensions.registerInstruction("resetactors","",opcodeResetActors);
            extensions.registerInstruction("fixme","",opcodeFixme);
            extensions.registerInstruction("ra","",opcodeResetActors);
        }
    }

    namespace User
    {
        void registerExtensions (Extensions& extensions)
        {
            extensions.registerInstruction ("user1", "", opcodeUser1);
            extensions.registerInstruction ("user2", "", opcodeUser2);
            extensions.registerInstruction ("user3", "", opcodeUser3, opcodeUser3);
            extensions.registerInstruction ("user4", "", opcodeUser4, opcodeUser4);
        }
    }
}
