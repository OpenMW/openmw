#ifndef OPENMW_SPELLAPI_HPP
#define OPENMW_SPELLAPI_HPP

#define SPELLAPI \
    {"InitializeSpellbookChanges", SpellFunctions::InitializeSpellbookChanges},\
    \
    {"GetSpellbookChangesSize",    SpellFunctions::GetSpellbookChangesSize},\
    {"GetSpellbookAction",         SpellFunctions::GetSpellbookAction},\
    \
    {"AddSpell",                   SpellFunctions::AddSpell},\
    {"RemoveSpell",                SpellFunctions::RemoveSpell},\
    \
    {"GetSpellId",                 SpellFunctions::GetSpellId},\
    \
    {"SendSpellbookChanges",       SpellFunctions::SendSpellbookChanges}

class SpellFunctions
{
public:

    static void InitializeSpellbookChanges(unsigned short pid) noexcept;

    static unsigned int GetSpellbookChangesSize(unsigned short pid) noexcept;
    static unsigned int GetSpellbookAction(unsigned short pid) noexcept;

    static void AddSpell(unsigned short pid, const char* spellId) noexcept;
    static void RemoveSpell(unsigned short pid, const char* spellId) noexcept;

    static const char *GetSpellId(unsigned short pid, unsigned int i) noexcept;

    static void SendSpellbookChanges(unsigned short pid, bool toOthers = false) noexcept;
private:

};

#endif //OPENMW_SPELLAPI_HPP
