#ifndef OPENMW_SPELLS_HPP
#define OPENMW_SPELLS_HPP

#define SPELLAPI \
    {"GetSpellbookSize", SpellFunctions::GetSpellbookSize},\
    \
    {"AddSpell",         SpellFunctions::AddSpell},\
    {"RemoveSpell",      SpellFunctions::RemoveSpell},\
    {"ClearSpellbook",   SpellFunctions::ClearInventory},\
    \
    {"HasSpell",         SpellFunctions::HasSpell},\
    {"GetSpellId",       SpellFunctions::GetSpellId},\
    \
    {"SendSpellbook",    SpellFunctions::SendSpellbook}

class SpellFunctions
{
public:

    static unsigned int GetSpellbookSize(unsigned short pid) noexcept;

    static void AddSpell(unsigned short pid, const char* spellId) noexcept;
    static void RemoveSpell(unsigned short pid, const char* spellId) noexcept;
    static void ClearSpellbook(unsigned short pid) noexcept;

    static bool HasSpell(unsigned short pid, const char* itemName);
    static const char *GetSpellId(unsigned short pid, unsigned int i) noexcept;

    static void SendSpellbook(unsigned short pid) noexcept;
private:

};

#endif //OPENMW_SPELLS_HPP
