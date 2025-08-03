#pragma once
#include <array>
#include <string_view>
#include <components/esm3/loadnpc.hpp>  // for ESM::NPC constants

inline constexpr std::array<std::pair<int, std::string_view>, 19> ServiceNames = {{
    { ESM::NPC::Spells, "Spells" },
    { ESM::NPC::Spellmaking, "Spellmaking" },
    { ESM::NPC::Enchanting, "Enchanting" },
    { ESM::NPC::Training, "Training" },
    { ESM::NPC::Repair, "Repair" },
    { ESM::NPC::AllItems, "Barter" },
    { ESM::NPC::Weapon, "Weapon" },
    { ESM::NPC::Armor, "Armor" },
    { ESM::NPC::Clothing, "Clothing" },
    { ESM::NPC::Books, "Books" },
    { ESM::NPC::Ingredients, "Ingredients" },
    { ESM::NPC::Picks, "Picks" },
    { ESM::NPC::Probes, "Probes" },
    { ESM::NPC::Lights, "Lights" },
    { ESM::NPC::Apparatus, "Apparatus" },
    { ESM::NPC::RepairItem, "RepairItem" },
    { ESM::NPC::Misc, "Misc" },
    { ESM::NPC::Potions, "Potions" },
    { ESM::NPC::MagicItems, "MagicItems" }
}};