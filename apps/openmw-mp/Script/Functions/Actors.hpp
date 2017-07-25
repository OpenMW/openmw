#ifndef OPENMW_ACTORAPI_HPP
#define OPENMW_ACTORAPI_HPP

#define ACTORAPI \
    {"ReadLastActorList",           ActorFunctions::ReadLastActorList},\
    {"ReadCellActorList",           ActorFunctions::ReadCellActorList},\
    {"InitializeActorList",         ActorFunctions::InitializeActorList},\
    \
    {"GetActorListSize",            ActorFunctions::GetActorListSize},\
    {"GetActorListAction",          ActorFunctions::GetActorListAction},\
    \
    {"GetActorCell",                ActorFunctions::GetActorCell},\
    {"GetActorRefId",               ActorFunctions::GetActorRefId},\
    {"GetActorRefNumIndex",         ActorFunctions::GetActorRefNumIndex},\
    {"GetActorMpNum",               ActorFunctions::GetActorMpNum},\
    \
    {"GetActorPosX",                ActorFunctions::GetActorPosX},\
    {"GetActorPosY",                ActorFunctions::GetActorPosY},\
    {"GetActorPosZ",                ActorFunctions::GetActorPosZ},\
    {"GetActorRotX",                ActorFunctions::GetActorRotX},\
    {"GetActorRotY",                ActorFunctions::GetActorRotY},\
    {"GetActorRotZ",                ActorFunctions::GetActorRotZ},\
    \
    {"GetActorHealthBase",          ActorFunctions::GetActorHealthBase},\
    {"GetActorHealthCurrent",       ActorFunctions::GetActorHealthCurrent},\
    {"GetActorHealthModified",      ActorFunctions::GetActorHealthModified},\
    {"GetActorMagickaBase",         ActorFunctions::GetActorMagickaBase},\
    {"GetActorMagickaCurrent",      ActorFunctions::GetActorMagickaCurrent},\
    {"GetActorMagickaModified",     ActorFunctions::GetActorMagickaModified},\
    {"GetActorFatigueBase",         ActorFunctions::GetActorFatigueBase},\
    {"GetActorFatigueCurrent",      ActorFunctions::GetActorFatigueCurrent},\
    {"GetActorFatigueModified",     ActorFunctions::GetActorFatigueModified},\
    \
    {"GetActorEquipmentItemRefId",  ActorFunctions::GetActorEquipmentItemRefId},\
    {"GetActorEquipmentItemCount",  ActorFunctions::GetActorEquipmentItemCount},\
    {"GetActorEquipmentItemCharge", ActorFunctions::GetActorEquipmentItemCharge},\
    \
    {"DoesActorHavePosition",       ActorFunctions::DoesActorHavePosition},\
    {"DoesActorHaveStatsDynamic",   ActorFunctions::DoesActorHaveStatsDynamic},\
    \
    {"SetActorListCell",            ActorFunctions::SetActorListCell},\
    {"SetActorListAction",          ActorFunctions::SetActorListAction},\
    \
    {"SetActorCell",                ActorFunctions::SetActorCell},\
    {"SetActorRefId",               ActorFunctions::SetActorRefId},\
    {"SetActorRefNumIndex",         ActorFunctions::SetActorRefNumIndex},\
    {"SetActorMpNum",               ActorFunctions::SetActorMpNum},\
    \
    {"SetActorPosition",            ActorFunctions::SetActorPosition},\
    {"SetActorRotation",            ActorFunctions::SetActorRotation},\
    \
    {"SetActorHealthBase",          ActorFunctions::SetActorHealthBase},\
    {"SetActorHealthCurrent",       ActorFunctions::SetActorHealthCurrent},\
    {"SetActorHealthModified",      ActorFunctions::SetActorHealthModified},\
    {"SetActorMagickaBase",         ActorFunctions::SetActorMagickaBase},\
    {"SetActorMagickaCurrent",      ActorFunctions::SetActorMagickaCurrent},\
    {"SetActorMagickaModified",     ActorFunctions::SetActorMagickaModified},\
    {"SetActorFatigueBase",         ActorFunctions::SetActorFatigueBase},\
    {"SetActorFatigueCurrent",      ActorFunctions::SetActorFatigueCurrent},\
    {"SetActorFatigueModified",     ActorFunctions::SetActorFatigueModified},\
    \
    {"EquipActorItem",              ActorFunctions::EquipActorItem},\
    {"UnequipActorItem",            ActorFunctions::UnequipActorItem},\
    \
    {"AddActor",                    ActorFunctions::AddActor},\
    \
    {"SendActorList",               ActorFunctions::SendActorList},\
    {"SendActorAuthority",          ActorFunctions::SendActorAuthority},\
    {"SendActorPosition",           ActorFunctions::SendActorPosition},\
    {"SendActorStatsDynamic",       ActorFunctions::SendActorStatsDynamic},\
    {"SendActorEquipment",          ActorFunctions::SendActorEquipment},\
    {"SendActorCellChange",         ActorFunctions::SendActorCellChange}

class ActorFunctions
{
public:

    /**
    * \brief Use the last actor list received by the server as the one being read.
    *
    * \return void
    */
    static void ReadLastActorList() noexcept;

    /**
    * \brief Use the temporary actor list stored for a cell as the one being read.
    *
    * This type of actor list is used to store actor positions and dynamic stats and is deleted
    * when the cell is unloaded.
    *
    * \param cellDescription The description of the cell whose actor list should be read.
    * \return void
    */
    static void ReadCellActorList(const char* cellDescription) noexcept;

    /**
    * \brief Clear the data from the last actor list sent by the server.
    *
    * This is used to initialize the sending of new Actor packets.
    *
    * \param pid The player ID to whom the actor list should be attached.
    * \return void
    */
    static void InitializeActorList(unsigned short pid) noexcept;

    /**
    * \brief Get the number of indexes in the read actor list.
    *
    * \return The number of indexes.
    */
    static unsigned int GetActorListSize() noexcept;

    /**
    * \brief Get the action type used in the read actor list.
    *
    * \return The action type (0 for SET, 1 for ADD, 2 for REMOVE, 3 for REQUEST).
    */
    static unsigned char GetActorListAction() noexcept;

    /**
    * \brief Get the cell description of the actor at a certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \return The cell description.
    */
    static const char *GetActorCell(unsigned int i) noexcept;
    
    /**
    * \brief Get the refId of the actor at a certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \return The refId.
    */
    static const char *GetActorRefId(unsigned int i) noexcept;

    /**
    * \brief Get the refNumIndex of the actor at a certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \return The refNumIndex.
    */
    static int GetActorRefNumIndex(unsigned int i) noexcept;

    /**
    * \brief Get the mpNum of the actor at a certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \return The mpNum.
    */
    static int GetActorMpNum(unsigned int i) noexcept;

    /**
    * \brief Get the X position of the actor at a certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \return The X position.
    */
    static double GetActorPosX(unsigned int i) noexcept;

    /**
    * \brief Get the Y position of the actor at a certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \return The Y position.
    */
    static double GetActorPosY(unsigned int i) noexcept;

    /**
    * \brief Get the Z position of the actor at a certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \return The Z position.
    */
    static double GetActorPosZ(unsigned int i) noexcept;

    /**
    * \brief Get the X rotation of the actor at a certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \return The X rotation.
    */
    static double GetActorRotX(unsigned int i) noexcept;

    /**
    * \brief Get the Y rotation of the actor at a certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \return The Y rotation.
    */
    static double GetActorRotY(unsigned int i) noexcept;

    /**
    * \brief Get the Z rotation of the actor at a certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \return The Z rotation.
    */
    static double GetActorRotZ(unsigned int i) noexcept;

    /**
    * \brief Get the base health of the actor at a certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \return The base health.
    */
    static double GetActorHealthBase(unsigned int i) noexcept;

    /**
    * \brief Get the current health of the actor at a certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \return The current health.
    */
    static double GetActorHealthCurrent(unsigned int i) noexcept;

    /**
    * \brief Get the modified health of the actor at a certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \return The modified health.
    */
    static double GetActorHealthModified(unsigned int i) noexcept;

    /**
    * \brief Get the base magicka of the actor at a certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \return The base magicka.
    */
    static double GetActorMagickaBase(unsigned int i) noexcept;

    /**
    * \brief Get the current magicka of the actor at a certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \return The current magicka.
    */
    static double GetActorMagickaCurrent(unsigned int i) noexcept;

    /**
    * \brief Get the modified magicka of the actor at a certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \return The modified magicka.
    */
    static double GetActorMagickaModified(unsigned int i) noexcept;

    /**
    * \brief Get the base fatigue of the actor at a certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \return The base fatigue.
    */
    static double GetActorFatigueBase(unsigned int i) noexcept;

    /**
    * \brief Get the current fatigue of the actor at a certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \return The current fatigue.
    */
    static double GetActorFatigueCurrent(unsigned int i) noexcept;

    /**
    * \brief Get the modified fatigue of the actor at a certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \return The modified fatigue.
    */
    static double GetActorFatigueModified(unsigned int i) noexcept;

    /**
    * \brief Get the refId of the item in a certain slot of the equipment of the actor at a
    * certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \param slot The slot of the equipment item.
    * \return The refId.
    */
    static const char *GetActorEquipmentItemRefId(unsigned int i, unsigned short slot) noexcept;

    /**
    * \brief Get the count of the item in a certain slot of the equipment of the actor at a
    * certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \param slot The slot of the equipment item.
    * \return The item count.
    */
    static int GetActorEquipmentItemCount(unsigned int i, unsigned short slot) noexcept;

    /**
    * \brief Get the charge of the item in a certain slot of the equipment of the actor at a
    * certain index in the read actor list.
    *
    * \param i The index of the actor.
    * \param slot The slot of the equipment item.
    * \return The charge.
    */
    static int GetActorEquipmentItemCharge(unsigned int i, unsigned short slot) noexcept;

    /**
    * \brief Check whether there is any positional data for the actor at a certain index in
    * the read actor list.
    *
    * This is only useful when reading the actor list data recorded for a particular cell.
    *
    * \param i The index of the actor.
    * \return Whether the read actor list contains positional data.
    */
    static bool DoesActorHavePosition(unsigned int i) noexcept;

    /**
    * \brief Check whether there is any dynamic stats data for the actor at a certain index in
    * the read actor list.
    *
    * This is only useful when reading the actor list data recorded for a particular cell.
    *
    * \param i The index of the actor.
    * \return Whether the read actor list contains dynamic stats data.
    */
    static bool DoesActorHaveStatsDynamic(unsigned int i) noexcept;

    /**
    * \brief Set the cell of the temporary actor list stored on the server.
    *
    * The cell is determined to be an exterior cell if it fits the pattern of a number followed
    * by a comma followed by another number.
    *
    * \param cellDescription The description of the cell.
    * \return void
    */
    static void SetActorListCell(const char* cellDescription) noexcept;

    /**
    * \brief Set the action type of the temporary actor list stored on the server.
    *
    * \param action The action type (0 for SET, 1 for ADD, 2 for REMOVE, 3 for REQUEST).
    * \return void
    */
    static void SetActorListAction(unsigned char action) noexcept;

    /**
    * \brief Set the cell of the temporary actor stored on the server.
    *
    * Used for ActorCellChange packets, where a specific actor's cell now differs from that of the
    * actor list.
    *
    * The cell is determined to be an exterior cell if it fits the pattern of a number followed
    * by a comma followed by another number.
    *
    * \param cellDescription The description of the cell.
    * \return void
    */
    static void SetActorCell(const char* cellDescription) noexcept;

    /**
    * \brief Set the refId of the temporary actor stored on the server.
    *
    * \param refId The refId.
    * \return void
    */
    static void SetActorRefId(const char* refId) noexcept;

    /**
    * \brief Set the refNumIndex of the temporary actor stored on the server.
    *
    * \param refNumIndex The refNumIndex.
    * \return void
    */
    static void SetActorRefNumIndex(int refNumIndex) noexcept;

    /**
    * \brief Set the mpNum of the temporary actor stored on the server.
    *
    * \param mpNum The mpNum.
    * \return void
    */
    static void SetActorMpNum(int mpNum) noexcept;

    /**
    * \brief Set the position of the temporary actor stored on the server.
    *
    * \param x The X position.
    * \param y The Y position.
    * \param z The Z position.
    * \return void
    */
    static void SetActorPosition(double x, double y, double z) noexcept;

    /**
    * \brief Set the rotation of the temporary actor stored on the server.
    *
    * \param x The X rotation.
    * \param y The Y rotation.
    * \param z The Z rotation.
    * \return void
    */
    static void SetActorRotation(double x, double y, double z) noexcept;

    /**
    * \brief Set the base health of the temporary actor stored on the server.
    *
    * \param value The new value.
    * \return void
    */
    static void SetActorHealthBase(double value) noexcept;

    /**
    * \brief Set the current health of the temporary actor stored on the server.
    *
    * \param value The new value.
    * \return void
    */
    static void SetActorHealthCurrent(double value) noexcept;

    /**
    * \brief Set the modified health of the temporary actor stored on the server.
    *
    * \param value The new value.
    * \return void
    */
    static void SetActorHealthModified(double value) noexcept;

    /**
    * \brief Set the base magicka of the temporary actor stored on the server.
    *
    * \param value The new value.
    * \return void
    */
    static void SetActorMagickaBase(double value) noexcept;

    /**
    * \brief Set the current magicka of the temporary actor stored on the server.
    *
    * \param value The new value.
    * \return void
    */
    static void SetActorMagickaCurrent(double value) noexcept;

    /**
    * \brief Set the modified magicka of the temporary actor stored on the server.
    *
    * \param value The new value.
    * \return void
    */
    static void SetActorMagickaModified(double value) noexcept;

    /**
    * \brief Set the base fatigue of the temporary actor stored on the server.
    *
    * \param value The new value.
    * \return void
    */
    static void SetActorFatigueBase(double value) noexcept;

    /**
    * \brief Set the current fatigue of the temporary actor stored on the server.
    *
    * \param value The new value.
    * \return void
    */
    static void SetActorFatigueCurrent(double value) noexcept;

    /**
    * \brief Set the modified fatigue of the temporary actor stored on the server.
    *
    * \param value The new value.
    * \return void
    */
    static void SetActorFatigueModified(double value) noexcept;

    /**
    * \brief Equip an item in a certain slot of the equipment of the temporary actor stored
    * on the server.
    *
    * \param slot The equipment slot.
    * \param refId The refId of the item.
    * \param count The count of the item.
    * \param charge The charge of the item.
    * \return void
    */
    static void EquipActorItem(unsigned short slot, const char* refId, unsigned int count, int charge) noexcept;

    /**
    * \brief Unequip the item in a certain slot of the equipment of the temporary actor stored
    * on the server.
    *
    * \param slot The equipment slot.
    * \return void
    */
    static void UnequipActorItem(unsigned short slot) noexcept;

    /**
    * \brief Add a copy of the server's temporary actor to the server's temporary actor list.
    *
    * In the process, the server's temporary actor will automatically be cleared so a new
    * one can be set up.
    *
    * \return void
    */
    static void AddActor() noexcept;

    /**
    * \brief Send an ActorList packet.
    *
    * It is sent only to the player for whom the current actor list was initialized.
    *
    * \return void
    */
    static void SendActorList() noexcept;

    /**
    * \brief Send an ActorAuthority packet.
    *
    * The player for whom the current actor list was initialized is recorded in the server memory
    * as the new actor authority for the actor list's cell.
    *
    * The packet is sent to that player as well as all other players who have the cell loaded.
    *
    * \return void
    */
    static void SendActorAuthority() noexcept;

    /**
    * \brief Send an ActorPosition packet.
    *
    * It is sent only to the player for whom the current actor list was initialized.
    *
    * \return void
    */
    static void SendActorPosition() noexcept;

    /**
    * \brief Send an ActorStatsDynamic packet.
    *
    * It is sent only to the player for whom the current actor list was initialized.
    *
    * \return void
    */
    static void SendActorStatsDynamic() noexcept;

    /**
    * \brief Send an ActorEquipment packet.
    *
    * It is sent only to the player for whom the current actor list was initialized.
    *
    * \return void
    */
    static void SendActorEquipment() noexcept;

    /**
    * \brief Send an ActorCellChange packet.
    *
    * It is sent only to the player for whom the current actor list was initialized.
    *
    * \return void
    */
    static void SendActorCellChange() noexcept;
};


#endif //OPENMW_ACTORAPI_HPP
