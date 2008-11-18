/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (loadgmst.d) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */

module esm.loadgmst;
import esm.imports;

/*
 *  Game setting
 */

// TODO: It's likely that we don't need this struct any longer, given
// that game settings are now stored in Monster code. We will still
// use the loading code and the dirty value cleaning code of course,
// but there's no longer any need to store it in a separate lookup
// list, since Monster variables can be looked up just as fast.
struct GameSetting
{
  LoadState state;
  char[] id;

  union
  {
    char[] str;
    int i;
    float f;
  }
  VarType type;

  // These functions check if this game setting is one of the "dirty"
  // GMST records found in many mods. These are due to a serious bug
  // in the official TES3 editor. It only occurs in the newer editor
  // versions that came with Tribunal and Bloodmoon, and only if a
  // modder tries to make a mod without loading the corresponding
  // expansion master file. For example, if you have Tribunal
  // installed and try to make a mod without loading Tribunal.esm, the
  // editor will insert these GMST records.

  // The values of these "dirty" records differ in general from their
  // values as defined in Tribunal.esm and Bloodmoon.esm, and are
  // always set to the same "default" values. Most of these values are
  // nonsensical, ie. changing the "Seller Max" string to "Max Sale",
  // or change the stats of werewolves to useless values like 1. Some
  // of them break certain spell effects.
  
  // It is most likely that these values are just leftover values from
  // an early stage of development that are inserted as default values
  // by the editor code. They are supposed to be overridden when the
  // correct esm file is loaded. When it isn't loaded, you get stuck
  // with the initial value, and this gets written to every mod for
  // some reason.

  // Bethesda themselves have fallen for this bug. If you install both
  // Tribunal and Bloodmoon, the updated Tribunal.esm will contain the
  // dirty GMST settings from Bloodmoon, and Bloodmoon.esm will
  // contain some of the dirty settings from Tribunal. In other words,
  // this bug affects the game EVEN IF YOU DO NOT USE ANY MODS!

  // The guys at Bethesda are well aware of this bug (and many
  // others), as the mod community and fan base complained about them
  // for a long time. But it was never fixed.

  // There are several programs available to help modders remove these
  // records from their files, but not all modders use them, and they
  // really shouldn't have to. In this file we choose instead to
  // reject all the corrupt values at load time.

  // These functions checks if the current game setting is one of the
  // "dirty" ones as described above. TODO: I have not checked this
  // against other sources yet, do that later. Currently recognizes 22
  // values for tribunal and 50 for bloodmoon. Legitimate GMSTs in
  // mods (setting values other than the default "dirty" ones) are not
  // affected and will work correctly.

  // Checks for dirty tribunal values. These will be ignored if found
  // in any file except when they are found in "Tribunal.esm".
  bool isDirtyTribunal()
  {
    bool result = false;
    void cI(int ii) { if(ii == i) result = true; }
    void cF(float ff) { if(ff == f) result = true; }
    void cS(char[] ss) { if(ss == str) result = true; }

    // Here, id contains the game setting name, and we check the
    // setting for certain values. If it matches, this is a "dirty"
    // entry. The correct entry (as defined in Tribunal and Bloodmoon
    // esms) are given in the comments. Many of the values are
    // correct, and are marked as 'same'. We still ignore them though,
    // as they are still in the wrong file and might override custom
    // values from other mods.

    switch(id)
      {
	// Strings
      case "sProfitValue": cS("Profit Value"); break; // 'Profit:'
      case "sEditNote": cS("Edit Note"); break; // same
      case "sDeleteNote": cS("Delete Note?"); break; // same
      case "sMaxSale": cS("Max Sale"); break; // 'Seller Max'
      case "sMagicFabricantID": cS("Fabricant"); break; // 'Fabricant_summon'
      case "sTeleportDisabled": cS("Teleportation magic does not work here.");
	break; // same
      case "sLevitateDisabled": cS("Levitation magic does not work here.");
	break; // same
      case "sCompanionShare": cS("Companion Share"); break; // 'Share'
      case "sCompanionWarningButtonOne": cS("Let the mercenary quit.");
	break; // same
      case "sCompanionWarningButtonTwo": cS("Return to Companion Share display."); break; // same
      case "sCompanionWarningMessage": cS("Your mercenary is poorer now than when he contracted with you.  Your mercenary will quit if you do not give him gold or goods to bring his Profit Value to a positive value."); break;
	// 'Your mercenary is poorer now than when he contracted with
	// you.  Your mercenary will quit if you do not give him gold
	// or goods to bring his Profit to a positive value.'
	// [The difference here is "Profit Value" -> "Profit"}

      	// Strings that matches the id
      case "sEffectSummonFabricant": // 'Summon Fabricant'
	cS(id);
	break;

      default:
      }
    return result;
  }

  // Bloodmoon variant
  bool isDirtyBloodmoon()
  {
    bool result = false;
    void cI(int ii) { if(ii == i) result = true; }
    void cF(float ff) { if(ff == f) result = true; }
    void cS(char[] ss) { if(ss == str) result = true; }

    switch(id)
      {
	// Strings
      case "sWerewolfPopup": cS("Werewolf"); break; // same
      case "sWerewolfRestMessage": cS("You cannot rest in werewolf form.");
	break; // same
      case "sWerewolfRefusal": cS("You cannot do this as a werewolf.");
	break; // same
      case "sWerewolfAlarmMessage": cS("You have been detected changing from a werewolf state."); break;
	// 'You have been detected as a known werewolf.'

      	// Strings that matches the id
      case "sMagicCreature01ID": // 'BM_wolf_grey_summon'
      case "sMagicCreature02ID": // 'BM_bear_black_summon'
      case "sMagicCreature03ID": // 'BM_wolf_bone_summon'
      case "sMagicCreature04ID": // same
      case "sMagicCreature05ID": // same
      case "sEffectSummonCreature01": // 'Calf Wolf'
      case "sEffectSummonCreature02": // 'Calf Bear'
      case "sEffectSummonCreature03": // 'Summon Bonewolf'
      case "sEffectSummonCreature04": // same
      case "sEffectSummonCreature05": // same
	cS(id);
	break;

	// Integers
      case "iWereWolfBounty": cI(10000); break; // 1000
      case "iWereWolfFightMod": cI(100); break; // same
      case "iWereWolfFleeMod": cI(100); break; // same
      case "iWereWolfLevelToAttack": cI(20); break; // same

	// Floats
      case "fFleeDistance": cF(3000); break; // same
      case "fCombatDistanceWerewolfMod": cF(0.3); break; // same
      case "fWereWolfFatigue": cF(400); break; // same
      case "fWereWolfEnchant": cF(1); break; // 0
      case "fWereWolfArmorer": cF(1); break; // 0
      case "fWereWolfBlock": cF(1); break; // 0
      case "fWereWolfSneak": cF(1); break; // 95
      case "fWereWolfDestruction": cF(1); break; // 0
      case "fWereWolfEndurance": cF(150); break; // same
      case "fWereWolfConjuration": cF(1); break; // 0
      case "fWereWolfRestoration": cF(1); break; // 0
      case "fWereWolfAthletics": cF(150); break; // 50
      case "fWereWolfLuck": cF(1); break; // 25
      case "fWereWolfSilverWeaponDamageMult": cF(1.5); break; // 2
      case "fWereWolfMediumArmor": cF(1); break; // 0
      case "fWereWolfShortBlade": cF(1); break; // 0
      case "fWereWolfAcrobatics": cF(150); break; // 80
      case "fWereWolfSpeechcraft": cF(1); break; // 0
      case "fWereWolfAlteration": cF(1); break; // 0
      case "fWereWolfIllusion": cF(1); break; // 0
      case "fWereWolfLongBlade": cF(1); break; // 0
      case "fWereWolfMarksman": cF(1); break; // 0
      case "fWereWolfHandtoHand": cF(100); break; // same
      case "fWereWolfIntellegence": cF(1); break; // 0
      case "fWereWolfAlchemy": cF(1); break; // 0
      case "fWereWolfUnarmored": cF(100); break; // same
      case "fWereWolfAxe": cF(1); break; // 0
      case "fWereWolfRunMult": cF(1.5); break; // 1.3
      case "fWereWolfMagicka": cF(100); break; // same
      case "fWereWolfAgility": cF(150); break; // same
      case "fWereWolfBluntWeapon": cF(1); break; // 0
      case "fWereWolfSecurity": cF(1); break; // 0
      case "fWereWolfPersonality": cF(1); break; // 0
      case "fWereWolfMerchantile": cF(1); break; // 0
      case "fWereWolfHeavyArmor": cF(1); break; // 0
      case "fWereWolfSpear": cF(1); break; // 0
      case "fWereWolfStrength": cF(150); break; // same
      case "fWereWolfHealth": cF(2); break; // same
      case "fWereWolfMysticism": cF(1); break; // 0
      case "fWereWolfLightArmor": cF(1); break; // 0
      case "fWereWolfWillPower": cF(1); break; // 0
      case "fWereWolfSpeed": cF(150); break; // 90

      default:
      }
    return result;
  }

  char[] toString()
  {
    if(type == VarType.Int) return format(i);
    else if(type == VarType.Float) return format(f);
    else if(type == VarType.String) return str;
    else if(type == VarType.None) return "(no value)";
    assert(0);
  }

  void load()
  {
    // We are apparently allowed not to have any value at all.
    if(!esFile.hasMoreSubs())
      {
	if(state == LoadState.Previous)
	  writefln("Warning: Overwriting game setting %s with void value", id);
	type = VarType.None;
	return;
      }

    // If this is not the first record of this type to be loaded, then
    // we do a little "trick" to avoid overwriting the current data
    // with one of the dirty GMSTs.
    if(state == LoadState.Previous)
      {
	// Load the data in a temporary game setting instead
	GameSetting g;
	g.state = LoadState.Unloaded;
	g.id = id;
	g.load();

	// Only copy it if it was valid
	if(g.type != VarType.Ignored)
	  {
	    // Don't allow a change of type, unless the setting we are
	    // overwriting is an ignored value.
	    if(g.type != type && type != VarType.Ignored)
	      esFile.fail(format("GMST changed type from %d to %d",
			  cast(int)type, cast(int)g.type));

	    str = g.str; // This will copy all the data no matter what
			 // type it is
	  }
	return;
      }

    // Actually load some data
    esFile.getSubName();
    switch(esFile.retSubName())
      {
      case "STRV":
	str = esFile.getHString();
	type = VarType.String;
	break;
      case "INTV":
	i = esFile.getHInt();
	type = VarType.Int;
	break;
      case "FLTV":
	f = esFile.getHFloat();
	type = VarType.Float;
	break;
      default:
	esFile.fail("Unwanted subrecord type");
      }

    SpecialFile spf = esFile.getSpecial();

    // Check if this is one of the dirty values mentioned above. If it
    // is, we set the VarType to Ignored. This leaves the posibility
    // that this becomes the final var type, for example if you load a
    // plugin with tribunal-gmst settings without loading tribunal
    // first. (Then there would only exist dirty values for this
    // settings, no "real" value.) But this is not likely a problem,
    // since these empty values will never be used and we can check
    // for them in any case.

    if( ( spf != SpecialFile.Tribunal && isDirtyTribunal() ) ||
	( spf != SpecialFile.Bloodmoon && isDirtyBloodmoon() ) )
      type = VarType.Ignored;
  }
}
ListID!(GameSetting) gameSettings;
