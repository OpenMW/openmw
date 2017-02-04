#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketContainer.hpp"

using namespace mwmp;

PacketContainer::PacketContainer(RakNet::RakPeerInterface *peer) : WorldPacket(peer)
{
    packetID = ID_CONTAINER;
}

void PacketContainer::Packet(RakNet::BitStream *bs, WorldEvent *event, bool send)
{
    WorldPacket::Packet(bs, event, send);

    if (!send)
        event->objectChanges.objects.clear();
    else
    {
        event->objectChanges.count = (unsigned int)(event->objectChanges.objects.size());
        event->containerChanges.count = (unsigned int)(event->containerChanges.items.size());
    }

    RW(event->objectChanges.count, send);
    RW(event->containerChanges.count, send);

    RW(event->cell.mData.mFlags, send);
    RW(event->cell.mData.mX, send);
    RW(event->cell.mData.mY, send);
    RW(event->cell.mName, send);

    WorldObject worldObject;

    for (unsigned int i = 0; i < event->objectChanges.count; i++)
    {
        if (send)
        {
            worldObject = event->objectChanges.objects[i];
        }

        RW(worldObject.refId, send);
        RW(worldObject.refNumIndex, send);

        if (!send)
        {
            event->objectChanges.objects.push_back(worldObject);
        }
    }

    ContainerItem containerItem;

    for (unsigned int i = 0; i < event->containerChanges.count; i++)
    {
        if (send)
        {
            containerItem = event->containerChanges.items[i];
        }

        RW(containerItem.refId, send);
        RW(containerItem.count, send);

        if (!send)
        {
            event->containerChanges.items.push_back(containerItem);
        }
    }

}
