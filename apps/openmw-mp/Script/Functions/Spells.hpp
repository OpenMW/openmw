#ifndef OPENMW_SPELLAPI_HPP
#define OPENMW_SPELLAPI_HPP

#define SPELLAPI \
    {"InitializeSpellbookChanges", SpellFunctions::InitializeSpellbookChanges},\
    \
    {"GetSpellbookChangesSize",    SpellFunctions::GetSpellbookChangesSize},\
    {"GetSpellbookChangesAction",  SpellFunctions::GetSpellbookChangesAction},\
    \
    {"SetSpellbookChangesAction",  SpellFunctions::SetSpellbookChangesAction},\
    {"AddSpell",                   SpellFunctions::AddSpell},\
    {"AddCustomSpell",                   SpellFunctions::AddCustomSpell},\
    {"AddCustomSpellData",                   SpellFunctions::AddCustomSpellData},\
    {"AddCustomSpellEffect",                   SpellFunctions::AddCustomSpellEffect},\
    \
    {"GetSpellId",                 SpellFunctions::GetSpellId},\
    {"GetSpellName",                 SpellFunctions::GetSpellName},\
    {"GetSpellType",                 SpellFunctions::GetSpellType},\
    {"GetSpellCost",                 SpellFunctions::GetSpellCost},\
    {"GetSpellFlags",                 SpellFunctions::GetSpellFlags},\
    {"GetSpellEffectCount",                 SpellFunctions::GetSpellEffectCount},\
    {"GetSpellEffectId",                 SpellFunctions::GetSpellEffectId},\
    {"GetSpellEffectSkill",                 SpellFunctions::GetSpellEffectSkill},\
    {"GetSpellEffectAttribute",                 SpellFunctions::GetSpellEffectAttribute},\
    {"GetSpellEffectRange",                 SpellFunctions::GetSpellEffectRange},\
    {"GetSpellEffectArea",                 SpellFunctions::GetSpellEffectArea},\
    {"GetSpellEffectDuration",                 SpellFunctions::GetSpellEffectDuration},\
    {"GetSpellEffectMagnMin",                 SpellFunctions::GetSpellEffectMagnMin},\
    {"GetSpellEffectMagnMax",                 SpellFunctions::GetSpellEffectMagnMax},\
    \
    {"SendSpellbookChanges",       SpellFunctions::SendSpellbookChanges}

class SpellFunctions
{
public:

    /**
    * \brief Clear the last recorded spellbook changes for a player.
    *
    * This is used to initialize the sending of new PlayerSpellbook packets.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \return void
    */
    static void InitializeSpellbookChanges(unsigned short pid) noexcept;

    /**
    * \brief Get the number of indexes in a player's latest spellbook changes.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \return The number of indexes.
    */
    static unsigned int GetSpellbookChangesSize(unsigned short pid) noexcept;

    /**
    * \brief Get the action type used in a player's latest spellbook changes.
    *
    * \param pid The player ID whose faction changes should be used.
    * \return The action type (0 for SET, 1 for ADD, 2 for REMOVE).
    */
    static unsigned int GetSpellbookChangesAction(unsigned short pid) noexcept;

    /**
    * \brief Set the action type in a player's spellbook changes.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param action The action (0 for SET, 1 for ADD, 2 for REMOVE).
    * \return void
    */
    static void SetSpellbookChangesAction(unsigned short pid, unsigned char action) noexcept;

    /**
    * \brief Add a new spell to the spellbook changes for a player.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param spellId The spellId of the spell.
    * \return void
    */
    static void AddSpell(unsigned short pid, const char* spellId) noexcept;

    /**
    * \brief Add a new custom spell to the spellbook changes for a player.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param spellId The spellId of the spell.
    * \param spellName The name of the spell.
    * \return void
    */
    static void AddCustomSpell(unsigned short pid, const char* spellId, const char* spellName) noexcept;

    /**
    * \brief Add custom spell data to the spellbook changes for a player.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param spellId The spellId of the spell.
    * \param type The type of the spell.
    * \param cost The cost of the spell.
    * \param flags The flags of the spell.
    * \return void
    */
    static void AddCustomSpellData(unsigned short pid, const char* spellId, int type, int cost, int flags) noexcept;

    /**
    * \brief Add custom spell effect to the spellbook changes for a player.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param spellId The spellId of the spell.
    * \param effectId The effectId of the spell effect.
    * \param mSkill The skill affected by the spell effect.
    * \param mAttribute The attribute affected by the spell effect.
    * \param mRange The range of the spell effect.
    * \param mArea The area of the spell effect.
    * \param mDuration The duration of the spell effect.
    * \param mMagnMin The minimum magnitude of the spell effect.
    * \param mMagnMax The maximum magnitude of the spell effect.
    * \return void
    */
    static void AddCustomSpellEffect(unsigned short pid, const char* spellId, short effectId, signed char mSkill, signed char mAttribute, int mRange, int mArea, int mDuration, int mMagnMin, int mMagnMax) noexcept;

    /**
    * \brief Get the spellId at a certain index in a player's latest spellbook changes.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param i The index of the spell.
    * \return The spellId.
    */
    static const char *GetSpellId(unsigned short pid, unsigned int i) noexcept;

    /**
    * \brief Get the name of the spell at a certain index in a player's latest spellbook changes.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param i The index of the spell.
    * \return The spell name.
    */
    static const char *GetSpellName(unsigned short pid, unsigned int i) noexcept;

    /**
    * \brief Get the type of the spell at a certain index in a player's latest spellbook changes.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param i The index of the spell.
    * \return The spell type.
    */
    static int GetSpellType(unsigned short pid, unsigned int i) noexcept;

    /**
    * \brief Get the cost of the spell at a certain index in a player's latest spellbook changes.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param i The index of the spell.
    * \return The spell cost.
    */
    static int GetSpellCost(unsigned short pid, unsigned int i) noexcept;

    /**
    * \brief Get the flags of the spell at a certain index in a player's latest spellbook changes.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param i The index of the spell.
    * \return The spell flags.
    */
    static int GetSpellFlags(unsigned short pid, unsigned int i) noexcept;

    /**
    * \brief Get the number of effects on the spell at a certain index in a player's latest spellbook changes.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param i The index of the spell.
    * \return The spell effect count.
    */
    static unsigned int GetSpellEffectCount(unsigned short pid, unsigned int i) noexcept;

    /**
    * \brief Get the effectId of the effect at a certain index in the spell at another certain index in a player's latest spellbook changes.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param i The index of the spell.
    * \param j The index of the effect.
    * \return The effectId.
    */
    static short GetSpellEffectId(unsigned short pid, unsigned int i, unsigned int j) noexcept;

    /**
    * \brief Get the affected skill of the effect at a certain index in the spell at another certain index in a player's latest spellbook changes.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param i The index of the spell.
    * \param j The index of the effect.
    * \return The affected skill.
    */
    static signed char GetSpellEffectSkill(unsigned short pid, unsigned int i, unsigned int j) noexcept;

    /**
    * \brief Get the affected attribute of the effect at a certain index in the spell at another certain index in a player's latest spellbook changes.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param i The index of the spell.
    * \param j The index of the effect.
    * \return The affected attribute.
    */
    static signed char GetSpellEffectAttribute(unsigned short pid, unsigned int i, unsigned int j) noexcept;

    /**
    * \brief Get the range of the effect at a certain index in the spell at another certain index in a player's latest spellbook changes.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param i The index of the spell.
    * \param j The index of the effect.
    * \return The range.
    */
    static int GetSpellEffectRange(unsigned short pid, unsigned int i, unsigned int j) noexcept;

    /**
    * \brief Get the area of the effect at a certain index in the spell at another certain index in a player's latest spellbook changes.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param i The index of the spell.
    * \param j The index of the effect.
    * \return The area.
    */
    static int GetSpellEffectArea(unsigned short pid, unsigned int i, unsigned int j) noexcept;

    /**
    * \brief Get the duration of the effect at a certain index in the spell at another certain index in a player's latest spellbook changes.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param i The index of the spell.
    * \param j The index of the effect.
    * \return The duration.
    */
    static int GetSpellEffectDuration(unsigned short pid, unsigned int i, unsigned int j) noexcept;

    /**
    * \brief Get the minimum magnitude of the effect at a certain index in the spell at another certain index in a player's latest spellbook changes.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param i The index of the spell.
    * \param j The index of the effect.
    * \return The minimum magnitude.
    */
    static int GetSpellEffectMagnMin(unsigned short pid, unsigned int i, unsigned int j) noexcept;

    /**
    * \brief Get the maximum magnitude of the effect at a certain index in the spell at another certain index in a player's latest spellbook changes.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param i The index of the spell.
    * \param j The index of the effect.
    * \return The maximum magnitude.
    */
    static int GetSpellEffectMagnMax(unsigned short pid, unsigned int i, unsigned int j) noexcept;

    /**
    * \brief Send a PlayerSpellbook packet with a player's recorded spellbook changes.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param toOthers Whether this packet should be sent only to other players or
    *                 only to the player it is about.
    * \return void
    */
    static void SendSpellbookChanges(unsigned short pid, bool toOthers = false) noexcept;

private:

};

#endif //OPENMW_SPELLAPI_HPP
