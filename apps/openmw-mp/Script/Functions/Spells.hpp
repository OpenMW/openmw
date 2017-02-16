#ifndef OPENMW_SPELLS_HPP
#define OPENMW_SPELLS_HPP

#define SPELLAPI \
    {"GetSpellbookChangesSize", SpellFunctions::GetSpellbookChangesSize},\
    {"GetSpellbookAction",      SpellFunctions::GetSpellbookAction},\
    \
    {"AddSpell",                SpellFunctions::AddSpell},\
    {"RemoveSpell",             SpellFunctions::RemoveSpell},\
    {"ClearSpellbook",          SpellFunctions::ClearSpellbook},\
    \
    {"GetSpellId",              SpellFunctions::GetSpellId},\
    \
    {"SendSpellbookChanges",    SpellFunctions::SendSpellbookChanges}

class SpellFunctions
{
public:

    static unsigned int GetSpellbookChangesSize(unsigned short pid) noexcept;
    static unsigned int GetSpellbookAction(unsigned short pid) noexcept;

    static void AddSpell(unsigned short pid, const char* spellId) noexcept;
    static void RemoveSpell(unsigned short pid, const char* spellId) noexcept;
    static void ClearSpellbook(unsigned short pid) noexcept;

    static const char *GetSpellId(unsigned short pid, unsigned int i) noexcept;

    static void SendSpellbookChanges(unsigned short pid) noexcept;
private:

};

#endif //OPENMW_SPELLS_HPP
