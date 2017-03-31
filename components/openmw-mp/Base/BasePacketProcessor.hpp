//
// Created by koncord on 31.03.17.
//

#ifndef OPENMW_BASEPACKETPROCESSOR_HPP
#define OPENMW_BASEPACKETPROCESSOR_HPP

#include <string>

#define BPP_INIT(packet_id) packetID = packet_id; strPacketID = #packet_id; className = typeid(this).name();

class BasePacketProcessor
{
public:
    unsigned char GetPacketID()
    {
        return packetID;
    }
    std::string GetNameOfID()
    {
        return strPacketID;
    }

    std::string GetClassName()
    {
        return className;
    }

protected:
    unsigned char packetID;
    std::string strPacketID;
    std::string className;
};


#endif //OPENMW_BASEPACKETPROCESSOR_HPP
