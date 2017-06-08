#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>
#include "PacketContainer.hpp"

using namespace mwmp;

PacketContainer::PacketContainer(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_CONTAINER;
    hasCellData = true;
}

void PacketContainer::Packet(RakNet::BitStream *bs, bool send)
{
    if (!PacketHeader(bs, send))
        return;

    RW(event->action, send);

    WorldObject worldObject;
    for (unsigned int i = 0; i < event->worldObjectCount; i++)
    {
        if (send)
        {
            worldObject = event->worldObjects.at(i);
            worldObject.containerItemCount = (unsigned int) (worldObject.containerItems.size());
        }
        else
            worldObject.containerItems.clear();

        Object(worldObject, send);

        RW(worldObject.containerItemCount, send);

        if (worldObject.containerItemCount > maxObjects || worldObject.refId.empty() || (worldObject.refNumIndex != 0 && worldObject.mpNum != 0))
        {
            event->isValid = false;
            return;
        }

        ContainerItem containerItem;

        for (unsigned int j = 0; j < worldObject.containerItemCount; j++)
        {
            if (send)
                containerItem = worldObject.containerItems.at(j);

            RW(containerItem.refId, send);
            RW(containerItem.count, send);
            RW(containerItem.charge, send);
            RW(containerItem.actionCount, send);

            if (!send)
                worldObject.containerItems.push_back(containerItem);
        }
        if (!send)
            event->worldObjects.push_back(worldObject);
    }
}
