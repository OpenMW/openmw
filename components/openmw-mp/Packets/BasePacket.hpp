#ifndef OPENMW_BASEPACKET_HPP
#define OPENMW_BASEPACKET_HPP

#include <string>
#include <RakNetTypes.h>
#include <BitStream.h>
#include <PacketPriority.h>
#include <components/openmw-mp/Base/BasePlayer.hpp>


namespace mwmp
{
    class BasePacket
    {
    public:
        BasePacket(RakNet::RakPeerInterface *peer);

        ~BasePacket();

        void SetReadStream(RakNet::BitStream *bitStream);
        void SetSendStream(RakNet::BitStream *bitStream);
        void SetStreams(RakNet::BitStream *inStream, RakNet::BitStream *outStream);
        virtual void RequestData(RakNet::RakNetGUID guid);

        static size_t headerSize()
        {
            return (size_t)(1 + RakNet::RakNetGUID::size()); // packetID + RakNetGUID (uint64_t)
        }

        unsigned char GetPacketID()
        {
            return packetID;
        }

    protected:
        template<class templateType>
        void RW(templateType &data, unsigned int size, bool write)
        {
            if (write)
                bs->Write(data, size);
            else
                bs->Read(data, size);
        }

        template<class templateType>
        void RW(templateType &data, bool write)
        {
            if (write)
                bs->Write(data);
            else
                bs->Read(data);
        }

        void RW(bool &data, bool write)
        {
            char _data;
            if (write)
            {
                _data = data;
                bs->Write(_data);
            }
            else
            {
                bs->Read(_data);
                data = _data;
            }
        }

        void RW(std::string &str, bool write)
        {
            if (write)
            {
                RakNet::RakString rstr(str.c_str());
                bs->Write(rstr);
            }
            else
            {
                RakNet::RakString rstr;
                bs->Read(rstr);
                str = rstr.C_String();
            }
        }

    protected:
        unsigned char packetID;
        PacketReliability reliability;
        PacketPriority priority;
        RakNet::BitStream *bsRead, *bsSend, *bs;
        RakNet::RakPeerInterface *peer;
    };
}

#endif //OPENMW_BASEPACKET_HPP
