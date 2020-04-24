#ifndef OPENMW_ESMTOOL_LABELS_H
#define OPENMW_ESMTOOL_LABELS_H

#include <string>

std::string bodyPartLabel(int idx);
std::string meshPartLabel(int idx);
std::string meshTypeLabel(int idx);
std::string clothingTypeLabel(int idx);
std::string armorTypeLabel(int idx);
std::string dialogTypeLabel(int idx);
std::string questStatusLabel(int idx);
std::string creatureTypeLabel(int idx);
std::string soundTypeLabel(int idx);
std::string weaponTypeLabel(int idx);

// This function's a bit different because the types are record types,
// not consecutive values.
std::string aiTypeLabel(int type);

// This one's also a bit different, because it enumerates dialog
// select rule functions, not types.  Structurally, it still converts
// indexes to strings for display.
std::string ruleFunction(int idx);

// The labels below here can all be loaded from GMSTs, but are not
// currently because among other things, that requires loading the
// GMSTs before dumping any of the records.

// If the data format supported ordered lists of GMSTs (post 1.0), the
// lists could define the valid values, their localization strings,
// and the indexes for referencing the types in other records in the
// database.  Then a single label function could work for all types.

std::string magicEffectLabel(int idx);
std::string attributeLabel(int idx);
std::string spellTypeLabel(int idx);
std::string specializationLabel(int idx);
std::string skillLabel(int idx);
std::string apparatusTypeLabel(int idx);
std::string rangeTypeLabel(int idx);
std::string schoolLabel(int idx);
std::string enchantTypeLabel(int idx);

// The are the flag functions that convert a bitmask into a list of
// human readble strings representing the set bits.

std::string bodyPartFlags(int flags);
std::string cellFlags(int flags);
std::string containerFlags(int flags);
std::string creatureFlags(int flags);
std::string enchantmentFlags(int flags);
std::string landFlags(int flags);
std::string creatureListFlags(int flags);
std::string itemListFlags(int flags);
std::string lightFlags(int flags);
std::string magicEffectFlags(int flags);
std::string npcFlags(int flags);
std::string raceFlags(int flags);
std::string spellFlags(int flags);
std::string weaponFlags(int flags);

// Missing flags functions:
// aiServicesFlags, possibly more

#endif
