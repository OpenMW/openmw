//
// Created by koncord on 31.03.17.
//

#ifndef OPENMW_BASEPACKETPROCESSOR_HPP
#define OPENMW_BASEPACKETPROCESSOR_HPP

#include <string>
#include <memory>
#include <unordered_map>

#define BPP_INIT(packet_id) packetID = packet_id; strPacketID = #packet_id; className = typeid(this).name(); avoidReading = false;

template<typename Proccessor>
class BasePacketProcessor
{
public:
    typedef std::unordered_map<unsigned char, std::unique_ptr<Proccessor>> processors_t;
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

    static void AddProcessor(Proccessor *processor)
    {
        for (auto &p : processors)
        {
            if (processor->packetID == p.first)
                throw std::logic_error("processor " + p.second->strPacketID + " already registered. Check " +
                                       processor->className + " and " + p.second->className);
        }
        processors.insert(typename processors_t::value_type(processor->GetPacketID(), processor));
    }
protected:
    unsigned char packetID;
    std::string strPacketID;
    std::string className;
    bool avoidReading;
    static processors_t processors;
};

#endif //OPENMW_BASEPACKETPROCESSOR_HPP
