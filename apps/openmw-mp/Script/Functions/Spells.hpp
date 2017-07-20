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
    \
    {"GetSpellId",                 SpellFunctions::GetSpellId},\
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
    * \param topicId The spellId of the spell.
    * \return void
    */
    static void AddSpell(unsigned short pid, const char* spellId) noexcept;

    /**
    * \brief Get the spellId at a certain index in a player's latest spellbook changes.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param i The index of the spell.
    * \return The spellId.
    */
    static const char *GetSpellId(unsigned short pid, unsigned int i) noexcept;

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
