#ifndef OPENMW_CELLAPI_HPP
#define OPENMW_CELLAPI_HPP

#include "../Types.hpp"

#define CELLAPI \
    {"InitializeMapChanges",    CellFunctions::InitializeMapChanges},\
    \
    {"GetCellStateChangesSize", CellFunctions::GetCellStateChangesSize},\
    \
    {"GetCellStateType",        CellFunctions::GetCellStateType},\
    {"GetCellStateDescription", CellFunctions::GetCellStateDescription},\
    \
    {"GetCell",                 CellFunctions::GetCell},\
    {"GetExteriorX",            CellFunctions::GetExteriorX},\
    {"GetExteriorY",            CellFunctions::GetExteriorY},\
    {"IsInExterior",            CellFunctions::IsInExterior},\
    \
    {"GetRegion",               CellFunctions::GetRegion},\
    {"IsChangingRegion",        CellFunctions::IsChangingRegion},\
    \
    {"SetCell",                 CellFunctions::SetCell},\
    {"SetExteriorCell",         CellFunctions::SetExteriorCell},\
    \
    {"AddCellExplored",         CellFunctions::AddCellExplored},\
    \
    {"SendCell",                CellFunctions::SendCell},\
    {"SendMapChanges",          CellFunctions::SendMapChanges}


class CellFunctions
{
public:

    /**
    * \brief Clear the last recorded map changes for a player.
    *
    * This is used to initialize the sending of new PlayerMap packets.
    *
    * \param pid The player ID whose map changes should be used.
    * \return void
    */
    static void InitializeMapChanges(unsigned short pid) noexcept;

    /**
    * \brief Get the number of indexes in a player's latest cell state changes.
    *
    * \param pid The player ID whose cell state changes should be used.
    * \return The number of indexes.
    */
    static unsigned int GetCellStateChangesSize(unsigned short pid) noexcept;

    /**
    * \brief Get the cell state type at a certain index in a player's latest cell state changes.
    *
    * \param pid The player ID whose cell state changes should be used.
    * \param i The index of the cell state.
    * \return The cell state type (0 for LOAD, 1 for UNLOAD).
    */
    static unsigned int GetCellStateType(unsigned short pid, unsigned int i) noexcept;

    /**
    * \brief Get the cell description at a certain index in a player's latest cell state changes.
    *
    * \param pid The player ID whose cell state changes should be used.
    * \param i The index of the cell state.
    * \return The cell description.
    */
    static const char *GetCellStateDescription(unsigned short pid, unsigned int i) noexcept;

    /**
    * \brief Get the cell description of a player's cell.
    *
    * \param pid The player ID.
    * \return The cell description.
    */
    static const char *GetCell(unsigned short pid) noexcept;

    /**
    * \brief Get the X coordinate of the player's exterior cell.
    *
    * \param pid The player ID.
    * \return The X coordinate of the cell.
    */
    static int GetExteriorX(unsigned short pid) noexcept;

    /**
    * \brief Get the Y coordinate of the player's exterior cell.
    *
    * \param pid The player ID.
    * \return The Y coordinate of the cell.
    */
    static int GetExteriorY(unsigned short pid) noexcept;

    /**
    * \brief Check whether the player is in an exterior cell or not.
    *
    * \param pid The player ID.
    * \return Whether the player is in an exterior cell.
    */
    static bool IsInExterior(unsigned short pid) noexcept;

    /**
    * \brief Get the region of the player's exterior cell.
    *
    * A blank value will be returned if the player is in an interior.
    *
    * \param pid The player ID.
    * \return The region.
    */
    static const char *GetRegion(unsigned short pid) noexcept;

    /**
    * \brief Check whether the player's last cell change has involved a region change.
    *
    * \param pid The player ID.
    * \return Whether the player has changed their region.
    */
    static bool IsChangingRegion(unsigned short pid) noexcept;

    /**
    * \brief Set the cell of a player.
    *
    * This changes the cell recorded for that player in the server memory, but does not by itself
    * send a packet.
    *
    * The cell is determined to be an exterior cell if it fits the pattern of a number followed
    * by a comma followed by another number.
    *
    * \param pid The player ID.
    * \param cellDescription The cell description.
    * \return void
    */
    static void SetCell(unsigned short pid, const char *cellDescription) noexcept;

    /**
    * \brief Set the cell of a player to an exterior cell.
    *
    * This changes the cell recorded for that player in the server memory, but does not by itself
    * send a packet.
    *
    * \param pid The player ID.
    * \param x The X coordinate of the cell.
    * \param y The Y coordinate of the cell.
    * \return void
    */
    static void SetExteriorCell(unsigned short pid, int x, int y) noexcept;

    /**
    * \brief Add a new explored cell to the map changes for a player.
    *
    * \param pid The player ID whose map changes should be used.
    * \param cellDescription The cell description of the explored cell.
    * \return void
    */
    static void AddCellExplored(unsigned short pid, const char* cellDescription) noexcept;

    /**
    * \brief Send a PlayerCellChange packet about a player.
    *
    * It is only sent to the affected player.
    *
    * \param pid The player ID.
    * \return void
    */
    static void SendCell(unsigned short pid) noexcept;

    /**
    * \brief Send a PlayerMap packet with a player's recorded map changes.
    *
    * \param pid The player ID whose map changes should be used.
    * \param toOthers Whether this packet should be sent only to other players or
    *                 only to the player it is about.
    * \return void
    */
    static void SendMapChanges(unsigned short pid, bool toOthers = false) noexcept;
};

#endif //OPENMW_CELLAPI_HPP
