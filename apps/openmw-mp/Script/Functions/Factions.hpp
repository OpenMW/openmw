#ifndef OPENMW_FACTIONAPI_HPP
#define OPENMW_FACTIONAPI_HPP

#define FACTIONAPI \
    {"InitializeFactionChanges", FactionFunctions::InitializeFactionChanges},\
    \
    {"GetFactionChangesSize",    FactionFunctions::GetFactionChangesSize},\
    {"GetFactionChangesAction",  FactionFunctions::GetFactionChangesAction},\
    \
    {"GetFactionId",             FactionFunctions::GetFactionId},\
    {"GetFactionRank",           FactionFunctions::GetFactionRank},\
    {"GetFactionExpulsionState", FactionFunctions::GetFactionExpulsionState},\
    {"GetFactionReputation",     FactionFunctions::GetFactionReputation},\
    \
    {"SetFactionChangesAction",  FactionFunctions::SetFactionChangesAction},\
    {"SetFactionId",             FactionFunctions::SetFactionId},\
    {"SetFactionRank",           FactionFunctions::SetFactionRank},\
    {"SetFactionExpulsionState", FactionFunctions::SetFactionExpulsionState},\
    {"SetFactionReputation",     FactionFunctions::SetFactionReputation},\
    \
    {"AddFaction",               FactionFunctions::AddFaction},\
    \
    {"SendFactionChanges",       FactionFunctions::SendFactionChanges}

class FactionFunctions
{
public:

    /**
    * \brief Clear the last recorded faction changes for a player.
    *
    * This is used to initialize the sending of new PlayerFaction packets.
    *
    * \param pid The player ID whose faction changes should be used.
    * \return void
    */
    static void InitializeFactionChanges(unsigned short pid) noexcept;

    /**
    * \brief Get the number of indexes in a player's latest faction changes.
    *
    * \param pid The player ID whose faction changes should be used.
    * \return The number of indexes.
    */
    static unsigned int GetFactionChangesSize(unsigned short pid) noexcept;

    /**
    * \brief Get the action type used in a player's latest faction changes.
    *
    * \param pid The player ID whose faction changes should be used.
    * \return The action type (0 for RANK, 1 for EXPULSION, 2 for REPUTATION).
    */
    static unsigned char GetFactionChangesAction(unsigned short pid) noexcept;

    /**
    * \brief Get the factionId at a certain index in a player's latest faction changes.
    *
    * \param pid The player ID whose faction changes should be used.
    * \param i The index of the faction.
    * \return The factionId.
    */
    static const char *GetFactionId(unsigned short pid, unsigned int i) noexcept;

    /**
    * \brief Get the rank at a certain index in a player's latest faction changes.
    *
    * \param pid The player ID whose faction changes should be used.
    * \param i The index of the faction.
    * \return The rank.
    */
    static int GetFactionRank(unsigned short pid, unsigned int i) noexcept;

    /**
    * \brief Get the expulsion state at a certain index in a player's latest faction changes.
    *
    * \param pid The player ID whose faction changes should be used.
    * \param i The index of the faction.
    * \return The expulsion state.
    */
    static bool GetFactionExpulsionState(unsigned short pid, unsigned int i) noexcept;

    /**
    * \brief Get the reputation at a certain index in a player's latest faction changes.
    *
    * \param pid The player ID whose faction changes should be used.
    * \param i The index of the faction.
    * \return The reputation.
    */
    static int GetFactionReputation(unsigned short pid, unsigned int i) noexcept;

    /**
    * \brief Set the action type in a player's faction changes.
    *
    * \param pid The player ID whose faction changes should be used.
    * \param action The action (0 for RANK, 1 for EXPULSION, 2 for REPUTATION).
    * \return void
    */
    static void SetFactionChangesAction(unsigned short pid, unsigned char action) noexcept;

    /**
    * \brief Set the factionId of the temporary faction stored on the server.
    *
    * \param factionId The factionId.
    * \return void
    */
    static void SetFactionId(const char* factionId) noexcept;

    /**
    * \brief Set the rank of the temporary faction stored on the server.
    *
    * \param rank The rank.
    * \return void
    */
    static void SetFactionRank(unsigned int rank) noexcept;

    /**
    * \brief Set the expulsion state of the temporary faction stored on the server.
    *
    * \param expulsionState The expulsion state.
    * \return void
    */
    static void SetFactionExpulsionState(bool expulsionState) noexcept;

    /**
    * \brief Set the reputation of the temporary faction stored on the server.
    *
    * \param reputation The reputation.
    * \return void
    */
    static void SetFactionReputation(int reputation) noexcept;

    /**
    * \brief Add the server's temporary faction to the faction changes for a player.
    *
    * In the process, the server's temporary faction will automatically be cleared so a new one
    * can be set up.
    *
    * \param pid The player ID whose faction changes should be used.
    * \return void
    */
    static void AddFaction(unsigned short pid) noexcept;

    /**
    * \brief Send a PlayerFaction packet with a player's recorded faction changes.
    *
    * \param pid The player ID whose faction changes should be used.
    * \param toOthers Whether this packet should be sent only to other players or
    *                 only to the player it is about.
    * \return void
    */
    static void SendFactionChanges(unsigned short pid, bool toOthers = false) noexcept;
private:

};

#endif //OPENMW_FACTIONAPI_HPP
