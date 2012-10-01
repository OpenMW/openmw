#ifndef OPENMW_ESM_GMST_H
#define OPENMW_ESM_GMST_H

#include <string>

#include "defs.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 *  Game setting, with automatic cleaning of "dirty" entries.
 *
 */

struct GameSetting
{
    std::string mId;
    // One of these is used depending on the variable type
    std::string mStr;
    int mI;
    float mF;
    VarType mType;

    // Set to true if this is a 'dirty' entry which should be ignored
    bool mDirty;

    /*
     These functions check if this game setting is one of the "dirty"
     GMST records found in many mods. These are due to a serious bug in
     the official TES3 editor. It only occurs in the newer editor
     versions that came with Tribunal and Bloodmoon, and only if a
     modder tries to make a mod without loading the corresponding
     expansion master file. For example, if you have Tribunal installed
     and try to make a mod without loading Tribunal.esm, the editor
     will insert these GMST records as a replacement for the entries it
     cannot find in the ESMs.

     The values of these "dirty" records differ in general from their
     values as defined in Tribunal.esm and Bloodmoon.esm, and are
     always set to the same "default" values. Most of these values are
     nonsensical, ie. changing the "Seller Max" string to "Max Sale",
     or change the stats of werewolves to useless values like 1. Some
     of them break certain spell effects.

     It is most likely that these values are just leftover values from
     an early stage of development that are inserted as default values
     by the editor code. They are supposed to be overridden when the
     correct esm file is loaded. When it isn't loaded however, you get
     stuck with the initial value, and this gets written to every mod
     by the editor, for some reason.

     Bethesda themselves have fallen for this bug. If you install both
     Tribunal and Bloodmoon, the updated Tribunal.esm will contain the
     dirty GMST settings from Bloodmoon, and Bloodmoon.esm will contain
     some of the dirty settings from Tribunal. In other words, this bug
     affects the game EVEN IF YOU DO NOT USE ANY MODS!

     The guys at Bethesda are well aware of this bug (and many others),
     as the mod community and fan base complained about them for a long
     time. But unfortunately it was never fixed.

     There are several tools available to help modders remove these
     records from their files, but not all modders use them, and they
     really shouldn't have to. In this file we choose instead to reject
     all the corrupt values at load time.

     These functions checks if the current game setting is one of the
     "dirty" ones as described above. TODO: I have not checked this
     against other sources yet, do that later. Currently recognizes 22
     values for tribunal and 50 for bloodmoon. Legitimate GMSTs in mods
     (setting values other than the default "dirty" ones) are not
     affected and will work correctly.
     */

    /*
     Checks for dirty tribunal values. These will be ignored if found
     in any file except when they are found in "Tribunal.esm".
     */
    bool isDirtyTribunal();

    // Bloodmoon variant
    bool isDirtyBloodmoon();

    void load(ESMReader &esm);
    
    int getInt() const;
    ///< Throws an exception if GMST is not of type int or float.
    
    float getFloat() const;
    ///< Throws an exception if GMST is not of type int or float.
    
    std::string getString() const;
    ///< Throwns an exception if GMST is not of type string.

    void save(ESMWriter &esm);
};
}
#endif
