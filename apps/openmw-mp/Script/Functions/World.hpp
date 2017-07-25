#ifndef OPENMW_WORLDAPI_HPP
#define OPENMW_WORLDAPI_HPP

#define WORLDAPI \
    {"ReadLastEvent",               WorldFunctions::ReadLastEvent},\
    {"InitializeEvent",             WorldFunctions::InitializeEvent},\
    \
    {"GetObjectChangesSize",        WorldFunctions::GetObjectChangesSize},\
    {"GetEventAction",              WorldFunctions::GetEventAction},\
    \
    {"GetObjectRefId",              WorldFunctions::GetObjectRefId},\
    {"GetObjectRefNumIndex",        WorldFunctions::GetObjectRefNumIndex},\
    {"GetObjectMpNum",              WorldFunctions::GetObjectMpNum},\
    {"GetObjectCount",              WorldFunctions::GetObjectCount},\
    {"GetObjectCharge",             WorldFunctions::GetObjectCharge},\
    {"GetObjectGoldValue",          WorldFunctions::GetObjectGoldValue},\
    {"GetObjectScale",              WorldFunctions::GetObjectScale},\
    {"GetObjectState",              WorldFunctions::GetObjectState},\
    {"GetObjectDoorState",          WorldFunctions::GetObjectDoorState},\
    {"GetObjectLockLevel",          WorldFunctions::GetObjectLockLevel},\
    {"GetObjectPosX",               WorldFunctions::GetObjectPosX},\
    {"GetObjectPosY",               WorldFunctions::GetObjectPosY},\
    {"GetObjectPosZ",               WorldFunctions::GetObjectPosZ},\
    {"GetObjectRotX",               WorldFunctions::GetObjectRotX},\
    {"GetObjectRotY",               WorldFunctions::GetObjectRotY},\
    {"GetObjectRotZ",               WorldFunctions::GetObjectRotZ},\
    \
    {"GetContainerChangesSize",     WorldFunctions::GetContainerChangesSize},\
    {"GetContainerItemRefId",       WorldFunctions::GetContainerItemRefId},\
    {"GetContainerItemCount",       WorldFunctions::GetContainerItemCount},\
    {"GetContainerItemCharge",      WorldFunctions::GetContainerItemCharge},\
    {"GetContainerItemActionCount", WorldFunctions::GetContainerItemActionCount},\
    \
    {"SetEventCell",                WorldFunctions::SetEventCell},\
    {"SetEventAction",              WorldFunctions::SetEventAction},\
    \
    {"SetObjectRefId",              WorldFunctions::SetObjectRefId},\
    {"SetObjectRefNumIndex",        WorldFunctions::SetObjectRefNumIndex},\
    {"SetObjectMpNum",              WorldFunctions::SetObjectMpNum},\
    {"SetObjectCount",              WorldFunctions::SetObjectCount},\
    {"SetObjectCharge",             WorldFunctions::SetObjectCharge},\
    {"SetObjectGoldValue",          WorldFunctions::SetObjectGoldValue},\
    {"SetObjectScale",              WorldFunctions::SetObjectScale},\
    {"SetObjectState",              WorldFunctions::SetObjectState},\
    {"SetObjectDoorState",          WorldFunctions::SetObjectDoorState},\
    {"SetObjectLockLevel",          WorldFunctions::SetObjectLockLevel},\
    {"SetObjectDisarmState",        WorldFunctions::SetObjectDisarmState},\
    {"SetObjectMasterState",        WorldFunctions::SetObjectMasterState},\
    {"SetObjectPosition",           WorldFunctions::SetObjectPosition},\
    {"SetObjectRotation",           WorldFunctions::SetObjectRotation},\
    \
    {"SetContainerItemRefId",       WorldFunctions::SetContainerItemRefId},\
    {"SetContainerItemCount",       WorldFunctions::SetContainerItemCount},\
    {"SetContainerItemCharge",      WorldFunctions::SetContainerItemCharge},\
    \
    {"AddWorldObject",              WorldFunctions::AddWorldObject},\
    {"AddContainerItem",            WorldFunctions::AddContainerItem},\
    \
    {"SendObjectPlace",             WorldFunctions::SendObjectPlace},\
    {"SendObjectSpawn",             WorldFunctions::SendObjectSpawn},\
    {"SendObjectDelete",            WorldFunctions::SendObjectDelete},\
    {"SendObjectLock",              WorldFunctions::SendObjectLock},\
    {"SendObjectTrap",              WorldFunctions::SendObjectTrap},\
    {"SendObjectScale",             WorldFunctions::SendObjectScale},\
    {"SendObjectState",             WorldFunctions::SendObjectState},\
    {"SendDoorState",               WorldFunctions::SendDoorState},\
    {"SendContainer",               WorldFunctions::SendContainer},\
    \
    {"SetHour",                     WorldFunctions::SetHour},\
    {"SetMonth",                    WorldFunctions::SetMonth},\
    {"SetDay",                      WorldFunctions::SetDay}

class WorldFunctions
{
public:

    /**
    * \brief Use the last event received by the server as the one being read.
    *
    * \return void
    */
    static void ReadLastEvent() noexcept;

    /**
    * \brief Clear the data from the last event sent by the server.
    *
    * This is used to initialize the sending of new Object packets.
    *
    * \param pid The player ID to whom the event should be attached.
    * \return void
    */
    static void InitializeEvent(unsigned short pid) noexcept;

    /**
    * \brief Get the number of indexes in the read event's object changes.
    *
    * \return The number of indexes.
    */
    static unsigned int GetObjectChangesSize() noexcept;

    /**
    * \brief Get the action type used in the read event.
    *
    * \return The action type (0 for SET, 1 for ADD, 2 for REMOVE, 3 for REQUEST).
    */
    static unsigned char GetEventAction() noexcept;

    /**
    * \brief Get the refId of the object at a certain index in the read event's object changes.
    *
    * \return The refId.
    */
    static const char *GetObjectRefId(unsigned int i) noexcept;

    /**
    * \brief Get the refNumIndex of the object at a certain index in the read event's object
    * changes.
    *
    * \param i The index of the object.
    * \return The refNumIndex.
    */
    static int GetObjectRefNumIndex(unsigned int i) noexcept;

    /**
    * \brief Get the mpNum of the object at a certain index in the read event's object changes.
    *
    * \param i The index of the object.
    * \return The mpNum.
    */
    static int GetObjectMpNum(unsigned int i) noexcept;

    /**
    * \brief Get the count of the object at a certain index in the read event's object changes.
    *
    * \param i The index of the object.
    * \return The object count.
    */
    static int GetObjectCount(unsigned int i) noexcept;

    /**
    * \brief Get the charge of the object at a certain index in the read event's object changes.
    *
    * \param i The index of the object.
    * \return The charge.
    */
    static int GetObjectCharge(unsigned int i) noexcept;

    /**
    * \brief Get the gold value of the object at a certain index in the read event's object
    * changes.
    *
    * This is used solely to get the gold value of gold. It is not used for other objects.
    *
    * \param i The index of the object.
    * \return The gold value.
    */
    static int GetObjectGoldValue(unsigned int i) noexcept;

    /**
    * \brief Get the object scale of the object at a certain index in the read event's object
    * changes.
    *
    * \param i The index of the object.
    * \return The object scale.
    */
    static double GetObjectScale(unsigned int i) noexcept;

    /**
    * \brief Get the object state of the object at a certain index in the read event's object
    * changes.
    *
    * \param i The index of the object.
    * \return The object state.
    */
    static bool GetObjectState(unsigned int i) noexcept;

    /**
    * \brief Get the door state of the object at a certain index in the read event's object
    * changes.
    *
    * \param i The index of the object.
    * \return The door state.
    */
    static int GetObjectDoorState(unsigned int i) noexcept;

    /**
    * \brief Get the lock level of the object at a certain index in the read event's object
    * changes.
    *
    * \param i The index of the object.
    * \return The lock level.
    */
    static int GetObjectLockLevel(unsigned int i) noexcept;

    /**
    * \brief Get the X position of the object at a certain index in the read event's object
    * changes.
    *
    * \param i The index of the object.
    * \return The X position.
    */
    static double GetObjectPosX(unsigned int i) noexcept;

    /**
    * \brief Get the Y position of the object at a certain index in the read event's object
    * changes.
    *
    * \param i The index of the object.
    * \return The Y position.
    */
    static double GetObjectPosY(unsigned int i) noexcept;

    /**
    * \brief Get the Z position at a certain index in the read event's object changes.
    *
    * \param i The index of the object.
    * \return The Z position.
    */
    static double GetObjectPosZ(unsigned int i) noexcept;

    /**
    * \brief Get the X rotation of the object at a certain index in the read event's object
    * changes.
    *
    * \param i The index of the object.
    * \return The X rotation.
    */
    static double GetObjectRotX(unsigned int i) noexcept;

    /**
    * \brief Get the Y rotation of the object at a certain index in the read event's object
    * changes.
    *
    * \param i The index of the object.
    * \return The Y rotation.
    */
    static double GetObjectRotY(unsigned int i) noexcept;

    /**
    * \brief Get the Z rotation of the object at a certain index in the read event's object
    * changes.
    *
    * \param i The index of the object.
    * \return The Z rotation.
    */
    static double GetObjectRotZ(unsigned int i) noexcept;

    /**
    * \brief Get the number of container item indexes of the object at a certain index in the
    * read event's object changes.
    *
    * \param i The index of the object.
    * \return The number of container item indexes.
    */
    static unsigned int GetContainerChangesSize(unsigned int objectIndex) noexcept;

    /**
    * \brief Get the refId of the container item at a certain itemIndex in the container changes
    * of the object at a certain objectIndex in the read event's object changes.
    *
    * \param objectIndex The index of the object.
    * \param itemIndex The index of the container item.
    * \return The refId.
    */
    static const char *GetContainerItemRefId(unsigned int objectIndex, unsigned int itemIndex) noexcept;

    /**
    * \brief Get the item count of the container item at a certain itemIndex in the container
    * changes of the object at a certain objectIndex in the read event's object changes.
    *
    * \param objectIndex The index of the object.
    * \param itemIndex The index of the container item.
    * \return The item count.
    */
    static int GetContainerItemCount(unsigned int objectIndex, unsigned int itemIndex) noexcept;

    /**
    * \brief Get the charge of the container item at a certain itemIndex in the container changes
    * of the object at a certain objectIndex in the read event's object changes.
    *
    * \param objectIndex The index of the object.
    * \param itemIndex The index of the container item.
    * \return The charge.
    */
    static int GetContainerItemCharge(unsigned int objectIndex, unsigned int itemIndex) noexcept;

    /**
    * \brief Get the action count of the container item at a certain itemIndex in the container
    * changes of the object at a certain objectIndex in the read event's object changes.
    *
    * \param objectIndex The index of the object.
    * \param itemIndex The index of the container item.
    * \return The action count.
    */
    static int GetContainerItemActionCount(unsigned int objectIndex, unsigned int itemIndex) noexcept;

    /**
    * \brief Set the cell of the temporary event stored on the server.
    *
    * The cell is determined to be an exterior cell if it fits the pattern of a number followed
    * by a comma followed by another number.
    *
    * \param cellDescription The description of the cell.
    * \return void
    */
    static void SetEventCell(const char* cellDescription) noexcept;

    /**
    * \brief Set the action type of the temporary event stored on the server.
    *
    * \param action The action type (0 for SET, 1 for ADD, 2 for REMOVE, 3 for REQUEST).
    * \return void
    */
    static void SetEventAction(unsigned char action) noexcept;

    /**
    * \brief Set the refId of the temporary world object stored on the server.
    *
    * \param refId The refId.
    * \return void
    */
    static void SetObjectRefId(const char* refId) noexcept;

    /**
    * \brief Set the refNumIndex of the temporary world object stored on the server.
    *
    * Every object loaded from .ESM and .ESP data files has a unique refNumIndex which needs to be
    * retained to refer to it in packets.
    * 
    * On the other hand, objects placed or spawned via the server should always have a refNumIndex
    * of 0.
    *
    * \param refNumIndex The refNumIndex.
    * \return void
    */
    static void SetObjectRefNumIndex(int refNumIndex) noexcept;

    /**
    * \brief Set the mpNum of the temporary world object stored on the server.
    *
    * Every object placed or spawned via the server is assigned an mpNum by incrementing the last
    * mpNum stored on the server. Scripts should take care to ensure that mpNums are kept unique
    * for these objects.
    * 
    * Objects loaded from .ESM and .ESP data files should always have an mpNum of 0, because they
    * have unique refNumIndexes instead.
    *
    * \param mpNum The mpNum.
    * \return void
    */
    static void SetObjectMpNum(int mpNum) noexcept;

    /**
    * \brief Set the object count of the temporary world object stored on the server.
    *
    * This determines the quantity of an object, with the exception of gold.
    *
    * \param count The object count.
    * \return void
    */
    static void SetObjectCount(int count) noexcept;

    /**
    * \brief Set the charge of the temporary world object stored on the server.
    *
    * Object durabilities are set through this value.
    *
    * \param charge The charge.
    * \return void
    */
    static void SetObjectCharge(int charge) noexcept;

    /**
    * \brief Set the gold value of the temporary world object stored on the server.
    *
    * This is used solely to set the gold value for gold. It has no effect on other objects.
    *
    * \param goldValue The gold value.
    * \return void
    */
    static void SetObjectGoldValue(int goldValue) noexcept;

    /**
    * \brief Set the scale of the temporary world object stored on the server.
    *
    * Objects are smaller or larger than their default size based on their scale.
    *
    * \param scale The scale.
    * \return void
    */
    static void SetObjectScale(double scale) noexcept;

    /**
    * \brief Set the object state of the temporary world object stored on the server.
    *
    * Objects are enabled or disabled based on their object state.
    *
    * \param objectState The object state.
    * \return void
    */
    static void SetObjectState(bool objectState) noexcept;

    /**
    * \brief Set the door state of the temporary world object stored on the server.
    *
    * Doors are open or closed based on their door state.
    *
    * \param doorState The door state.
    * \return void
    */
    static void SetObjectDoorState(int doorState) noexcept;

    /**
    * \brief Set the lock level of the temporary world object stored on the server.
    *
    * \param lockLevel The lock level.
    * \return void
    */
    static void SetObjectLockLevel(int lockLevel) noexcept;

    /**
    * \brief Set the disarm state of the temporary world object stored on the server.
    *
    * \param disarmState The disarmState.
    * \return void
    */
    static void SetObjectDisarmState(bool disarmState) noexcept;

    /**
    * \brief Set the master state of the temporary world object stored on the server.
    *
    * This only affects living actors and determines whether they are followers of another
    * living actor.
    *
    * \param masterState The master state.
    * \return void
    */
    static void SetObjectMasterState(bool masterState) noexcept;

    /**
    * \brief Set the position of the temporary world object stored on the server.
    *
    * \param x The X position.
    * \param y The Y position.
    * \param z The Z position.
    * \return void
    */
    static void SetObjectPosition(double x, double y, double z) noexcept;

    /**
    * \brief Set the rotation of the temporary world object stored on the server.
    *
    * \param x The X rotation.
    * \param y The Y rotation.
    * \param z The Z rotation.
    * \return void
    */
    static void SetObjectRotation(double x, double y, double z) noexcept;

    /**
    * \brief Set the refId of the temporary container item stored on the server.
    *
    * \param refId The refId.
    * \return void
    */
    static void SetContainerItemRefId(const char* refId) noexcept;

    /**
    * \brief Set the item count of the temporary container item stored on the server.
    *
    * \param count The item count.
    * \return void
    */
    static void SetContainerItemCount(int count) noexcept;

    /**
    * \brief Set the charge of the temporary container item stored on the server.
    *
    * \param charge The charge.
    * \return void
    */
    static void SetContainerItemCharge(int charge) noexcept;

    /**
    * \brief Add a copy of the server's temporary world object to the server's temporary event.
    *
    * In the process, the server's temporary world object will automatically be cleared so a new
    * one can be set up.
    *
    * \return void
    */
    static void AddWorldObject() noexcept;

    /**
    * \brief Add a copy of the server's temporary container item to the container changes of the
    * server's temporary world object.
    *
    * In the process, the server's temporary container item will automatically be cleared so a new
    * one can be set up.
    *
    * \return void
    */
    static void AddContainerItem() noexcept;

    /**
    * \brief Send an ObjectPlace packet.
    *
    * It is sent only to the player for whom the current event was initialized.
    *
    * \return void
    */
    static void SendObjectPlace() noexcept;

    /**
    * \brief Send an ObjectSpawn packet.
    *
    * It is sent only to the player for whom the current event was initialized.
    *
    * \return void
    */
    static void SendObjectSpawn() noexcept;

    /**
    * \brief Send an ObjectDelete packet.
    *
    * It is sent only to the player for whom the current event was initialized.
    *
    * \return void
    */
    static void SendObjectDelete() noexcept;

    /**
    * \brief Send an ObjectLock packet.
    *
    * It is sent only to the player for whom the current event was initialized.
    *
    * \return void
    */
    static void SendObjectLock() noexcept;

    /**
    * \brief Send an ObjectTrap packet.
    *
    * It is sent only to the player for whom the current event was initialized.
    *
    * \return void
    */
    static void SendObjectTrap() noexcept;

    /**
    * \brief Send an ObjectScale packet.
    *
    * It is sent only to the player for whom the current event was initialized.
    *
    * \return void
    */
    static void SendObjectScale() noexcept;

    /**
    * \brief Send an ObjectState packet.
    *
    * It is sent only to the player for whom the current event was initialized.
    *
    * \return void
    */
    static void SendObjectState() noexcept;

    /**
    * \brief Send a DoorState packet.
    *
    * It is sent only to the player for whom the current event was initialized.
    *
    * \return void
    */
    static void SendDoorState() noexcept;

    /**
    * \brief Send a Container packet.
    *
    * It is sent only to the player for whom the current event was initialized.
    *
    * \return void
    */
    static void SendContainer() noexcept;

    /**
    * \brief Set the game hour for a player and send a GameTime packet to that player.
    *
    * \param pid The player ID.
    * \param hour The hour.
    * \return void
    */
    static void SetHour(unsigned short pid, double hour) noexcept;

    /**
    * \brief Set the game month for a player and send a GameTime packet to that player.
    *
    * \param pid The player ID.
    * \param month The month.
    * \return void
    */
    static void SetMonth(unsigned short pid, int month) noexcept;

    /**
    * \brief Set the game day for a player and send a GameTime packet to that player.
    *
    * \param pid The player ID.
    * \param day The day.
    * \return void
    */
    static void SetDay(unsigned short pid, int day) noexcept;
};


#endif //OPENMW_WORLDAPI_HPP
