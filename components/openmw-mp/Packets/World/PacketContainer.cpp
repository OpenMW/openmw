#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>
#include "PacketContainer.hpp"

using namespace mwmp;

PacketContainer::PacketContainer(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_CONTAINER;
}

void PacketContainer::Packet(RakNet::BitStream *bs, bool send)
{
    WorldPacket::Packet(bs, send);

    RW(event->action, send);

    if (send)
    {
        event->worldObjectCount = (unsigned int)(event->worldObjects.size());
    }
    else
    {
        event->worldObjects.clear();
    }

    RW(event->worldObjectCount, send);

    RW(event->cell.mData.mFlags, send);
    RW(event->cell.mData.mX, send);
    RW(event->cell.mData.mY, send);
    RW(event->cell.mName, send);

    WorldObject worldObject;

    for (unsigned int i = 0; i < event->worldObjectCount; i++)
    {
        if (send)
        {
            worldObject = event->worldObjects.at(i);
            worldObject.containerItemCount = (unsigned int)(worldObject.containerItems.size());
        }
        else
        {
            worldObject.containerItems.clear();
        }

        RW(worldObject.refId, send);
        RW(worldObject.refNumIndex, send);
        RW(worldObject.mpNum, send);
        RW(worldObject.containerItemCount, send);

        if (worldObject.containerItemCount > 2000 || worldObject.refId.empty() || (worldObject.refNumIndex != 0 && worldObject.mpNum != 0))
        {
            event->isValid = false;
            return;
        }

        ContainerItem containerItem;

        for (unsigned int j = 0; j < worldObject.containerItemCount; j++)
        {
            if (send)
            {
                containerItem = worldObject.containerItems.at(j);
            }

            RW(containerItem.refId, send);
            RW(containerItem.count, send);
            RW(containerItem.charge, send);
            RW(containerItem.actionCount, send);

            if (!send)
            {
                worldObject.containerItems.push_back(containerItem);
            }
        }

        if (!send)
        {
            event->worldObjects.push_back(worldObject);
        }
    }
}
