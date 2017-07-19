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
    static unsigned int GetSpellbookAction(unsigned short pid) noexcept;

    /**
    * \brief Prepare the addition of a spell to a player.
    *
    * In practice, this changes the action type of the PlayerSpellbook packet to
    * ADD and adds this spell to it.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param topicId The spellId of the spell.
    * \return void
    */
    static void AddSpell(unsigned short pid, const char* spellId) noexcept;

    /**
    * \brief Prepare the removal of a spell from a player.
    *
    * In practice, this changes the action type of the PlayerSpellbook packet to
    * REMOVE and adds this spell to it.
    *
    * \param pid The player ID whose spellbook changes should be used.
    * \param topicId The spellId of the spell.
    * \return void
    */
    static void RemoveSpell(unsigned short pid, const char* spellId) noexcept;

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
