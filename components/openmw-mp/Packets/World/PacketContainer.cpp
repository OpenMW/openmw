#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketContainer.hpp"

using namespace mwmp;

PacketContainer::PacketContainer(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_CONTAINER;
}

void PacketContainer::Packet(RakNet::BitStream *bs, BaseEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);

    RW(event->action, send);

    if (send)
    {
        event->objectChanges.count = (unsigned int)(event->objectChanges.objects.size());
    }
    else
    {
        event->objectChanges.objects.clear();
    }

    RW(event->objectChanges.count, send);

    RW(event->cell.mData.mFlags, send);
    RW(event->cell.mData.mX, send);
    RW(event->cell.mData.mY, send);
    RW(event->cell.mName, send);

    WorldObject worldObject;

    for (unsigned int i = 0; i < event->objectChanges.count; i++)
    {
        if (send)
        {
            worldObject = event->objectChanges.objects.at(i);
            worldObject.containerChanges.count = (unsigned int)(worldObject.containerChanges.items.size());
        }
        else
        {
            worldObject.containerChanges.items.clear();
        }

        RW(worldObject.refId, send);
        RW(worldObject.refNumIndex, send);
        RW(worldObject.containerChanges.count, send);

        ContainerItem containerItem;

        for (unsigned int i = 0; i < worldObject.containerChanges.count; i++)
        {
            if (send)
            {
                containerItem = worldObject.containerChanges.items.at(i);
            }

            RW(containerItem.refId, send);
            RW(containerItem.count, send);
            RW(containerItem.charge, send);
            RW(containerItem.actionCount, send);

            if (!send)
            {
                worldObject.containerChanges.items.push_back(containerItem);
            }
        }

        if (!send)
        {
            event->objectChanges.objects.push_back(worldObject);
        }
    }
}
