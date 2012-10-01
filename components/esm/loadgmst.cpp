#include "loadgmst.hpp"

#include <stdexcept>

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

/// \todo Review GMST "fixing". Probably remove completely or at least make it optional. Its definitely not
/// working properly in its current state and I doubt it can be fixed without breaking other stuff.

// Some handy macros
#define cI(s,x) { if(mId == (s)) return (mI == (x)); }
#define cF(s,x) { if(mId == (s)) return (mF == (x)); }
#define cS(s,x) { if(mId == (s)) return (mStr == (x)); }

bool GameSetting::isDirtyTribunal()
{
    /*
     Here, mId contains the game setting name, and we check the
     setting for certain values. If it matches, this is a "mDirty"
     entry. The correct entry (as defined in Tribunal and Bloodmoon
     esms) are given in the comments. Many of the values are correct,
     and are marked as 'same'. We still ignore them though, as they
     are still in the wrong file and might override custom values
     from other mods.
     */

    // Strings
    cS("sProfitValue", "Profit Value"); // 'Profit:'
    cS("sEditNote", "Edit Note"); // same
    cS("sDeleteNote", "Delete Note?"); // same
    cS("sMaxSale", "Max Sale"); // 'Seller Max'
    cS("sMagicFabricantID", "Fabricant"); // 'Fabricant_summon'
    cS("sTeleportDisabled",
            "Teleportation magic does not work here.");// same
    cS("sLevitateDisabled",
            "Levitation magic does not work here."); // same
    cS("sCompanionShare", "Companion Share"); // 'Share'
    cS("sCompanionWarningButtonOne",
            "Let the mercenary quit."); // same
    cS("sCompanionWarningButtonTwo",
            "Return to Companion Share display."); // same
    cS("sCompanionWarningMessage",
            "Your mercenary is poorer now than when he contracted with you.  Your mercenary will quit if you do not give him gold or goods to bring his Profit Value to a positive value.");
    // 'Your mercenary is poorer now than when he contracted with
    // you.  Your mercenary will quit if you do not give him gold
    // or goods to bring his Profit to a positive value.'
    // [The difference here is "Profit Value" -> "Profit"]

    // Strings that matches the mId
    cS("sEffectSummonFabricant", mId);// 'Summon Fabricant'
    return false;
}

// Bloodmoon variant
bool GameSetting::isDirtyBloodmoon()
{
    // Strings
    cS("sWerewolfPopup", "Werewolf"); // same
    cS("sWerewolfRestMessage",
            "You cannot rest in werewolf form."); // same
    cS("sWerewolfRefusal",
            "You cannot do this as a werewolf."); // same
    cS("sWerewolfAlarmMessage",
            "You have been detected changing from a werewolf state.");
    // 'You have been detected as a known werewolf.'

    // Strings that matches the mId
    cS("sMagicCreature01ID", mId); // 'BM_wolf_grey_summon'
    cS("sMagicCreature02ID", mId); // 'BM_bear_black_summon'
    cS("sMagicCreature03ID", mId); // 'BM_wolf_bone_summon'
    cS("sMagicCreature04ID", mId); // same
    cS("sMagicCreature05ID", mId); // same
    cS("sEffectSummonCreature01", mId); // 'Calf Wolf'
    cS("sEffectSummonCreature02", mId); // 'Calf Bear'
    cS("sEffectSummonCreature03", mId); // 'Summon Bonewolf'
    cS("sEffectSummonCreature04", mId); // same
    cS("sEffectSummonCreature05", mId); // same

    // Integers
    cI("iWereWolfBounty", 10000); // 1000
    cI("iWereWolfFightMod", 100); // same
    cI("iWereWolfFleeMod", 100); // same
    cI("iWereWolfLevelToAttack", 20); // same

    // Floats
    cF("fFleeDistance", 3000); // same
    cF("fCombatDistanceWerewolfMod", 0.3); // same
    cF("fWereWolfFatigue", 400); // same
    cF("fWereWolfEnchant", 1); // 0
    cF("fWereWolfArmorer", 1); // 0
    cF("fWereWolfBlock", 1); // 0
    cF("fWereWolfSneak", 1); // 95
    cF("fWereWolfDestruction", 1); // 0
    cF("fWereWolfEndurance", 150); // same
    cF("fWereWolfConjuration", 1); // 0
    cF("fWereWolfRestoration", 1); // 0
    cF("fWereWolfAthletics", 150); // 50
    cF("fWereWolfLuck", 1); // 25
    cF("fWereWolfSilverWeaponDamageMult", 1.5); // 2
    cF("fWereWolfMediumArmor", 1); // 0
    cF("fWereWolfShortBlade", 1); // 0
    cF("fWereWolfAcrobatics", 150); // 80
    cF("fWereWolfSpeechcraft", 1); // 0
    cF("fWereWolfAlteration", 1); // 0
    cF("fWereWolfIllusion", 1); // 0
    cF("fWereWolfLongBlade", 1); // 0
    cF("fWereWolfMarksman", 1); // 0
    cF("fWereWolfHandtoHand", 100); // same
    cF("fWereWolfIntellegence", 1); // 0
    cF("fWereWolfAlchemy", 1); // 0
    cF("fWereWolfUnarmored", 100); // same
    cF("fWereWolfAxe", 1); // 0
    cF("fWereWolfRunMult", 1.5); // 1.3
    cF("fWereWolfMagicka", 100); // same
    cF("fWereWolfAgility", 150); // same
    cF("fWereWolfBluntWeapon", 1); // 0
    cF("fWereWolfSecurity", 1); // 0
    cF("fWereWolfPersonality", 1); // 0
    cF("fWereWolfMerchantile", 1); // 0
    cF("fWereWolfHeavyArmor", 1); // 0
    cF("fWereWolfSpear", 1); // 0
    cF("fWereWolfStrength", 150); // same
    cF("fWereWolfHealth", 2); // same
    cF("fWereWolfMysticism", 1); // 0
    cF("fWereWolfLightArmor", 1); // 0
    cF("fWereWolfWillPower", 1); // 0
    cF("fWereWolfSpeed", 150); // 90
    return false;
}

void GameSetting::load(ESMReader &esm)
{
    assert(mId != "");

    mDirty = false;

    // We are apparently allowed to be empty
    if (!esm.hasMoreSubs())
    {
        mType = VT_None;
        return;
    }

    // Load some data
    esm.getSubName();
    NAME n = esm.retSubName();
    if (n == "STRV")
    {
        mStr = esm.getHString();
        mType = VT_String;
    }
    else if (n == "INTV")
    {
        esm.getHT(mI);
        mType = VT_Int;
    }
    else if (n == "FLTV")
    {
        esm.getHT(mF);
        mType = VT_Float;
    }
    else
        esm.fail("Unwanted subrecord type");

    int spf = esm.getSpecial();

    // Check if this is one of the mDirty values mentioned above. If it
    // is, we set the mDirty flag. This will ONLY work if you've set
    // the 'id' string correctly before calling load().

    if ((spf != SF_Tribunal && isDirtyTribunal()) || (spf != SF_Bloodmoon
            && isDirtyBloodmoon()))
        mDirty = true;
}
void GameSetting::save(ESMWriter &esm)
{
    switch(mType)
    {
    case VT_String: esm.writeHNString("STRV", mStr); break;
    case VT_Int: esm.writeHNT("INTV", mI); break;
    case VT_Float: esm.writeHNT("FLTV", mF); break;
    default: break;
    }
}

int GameSetting::getInt() const
{
    switch (mType)
    {
        case VT_Float: return static_cast<int> (mF);
        case VT_Int: return mI;
        default: throw std::runtime_error ("GMST " + mId + " is not of a numeric type");
    }
}

float GameSetting::getFloat() const
{
    switch (mType)
    {
        case VT_Float: return mF;
        case VT_Int: return mI;
        default: throw std::runtime_error ("GMST " + mId + " is not of a numeric type");
    }
}

std::string GameSetting::getString() const
{
    if (mType==VT_String)
        return mStr;
        
    throw std::runtime_error ("GMST " + mId + " is not a string");
}

}
