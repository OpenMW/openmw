#ifndef OPENMW_ESMTOOL_LABELS_H
#define OPENMW_ESMTOOL_LABELS_H

#include <cstdint>
#include <string>
#include <string_view>

#include <components/esm3/aipackage.hpp>

std::string_view bodyPartLabel(int idx);
std::string_view meshPartLabel(int idx);
std::string_view meshTypeLabel(int idx);
std::string_view clothingTypeLabel(int idx);
std::string_view armorTypeLabel(int idx);
std::string_view dialogTypeLabel(int idx);
std::string_view questStatusLabel(int idx);
std::string_view creatureTypeLabel(int idx);
std::string_view soundTypeLabel(int idx);
std::string_view weaponTypeLabel(int idx);

// This function's a bit different because the types are record types,
// not consecutive values.
std::string_view aiTypeLabel(ESM::AiPackageType type);

// This one's also a bit different, because it enumerates dialog
// select rule functions, not types.  Structurally, it still converts
// indexes to strings for display.
std::string_view ruleFunction(int idx);

// The labels below here can all be loaded from GMSTs, but are not
// currently because among other things, that requires loading the
// GMSTs before dumping any of the records.

// If the data format supported ordered lists of GMSTs (post 1.0), the
// lists could define the valid values, their localization strings,
// and the indexes for referencing the types in other records in the
// database.  Then a single label function could work for all types.

std::string_view magicEffectLabel(int idx);
std::string_view attributeLabel(int idx);
std::string_view spellTypeLabel(int idx);
std::string_view specializationLabel(int idx);
std::string_view skillLabel(int idx);
std::string_view apparatusTypeLabel(int idx);
std::string_view rangeTypeLabel(int idx);
std::string_view schoolLabel(int idx);
std::string_view enchantTypeLabel(int idx);

// The are the flag functions that convert a bitmask into a list of
// human readble strings representing the set bits.

std::string bodyPartFlags(int flags);
std::string cellFlags(int flags);
std::string containerFlags(int flags);
std::string creatureFlags(int flags);
std::string enchantmentFlags(int flags);
std::string landFlags(std::uint32_t flags);
std::string creatureListFlags(int flags);
std::string itemListFlags(int flags);
std::string lightFlags(int flags);
std::string magicEffectFlags(int flags);
std::string npcFlags(int flags);
std::string potionFlags(int flags);
std::string raceFlags(int flags);
std::string spellFlags(int flags);
std::string weaponFlags(int flags);

std::string recordFlags(uint32_t flags);

// Missing flags functions:
// aiServicesFlags, possibly more

#endif
