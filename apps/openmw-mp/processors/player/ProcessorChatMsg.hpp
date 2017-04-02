//
// Created by koncord on 01.04.17.
//

#ifndef OPENMW_PROCESSORPLAYERCHATMSG_HPP
#define OPENMW_PROCESSORPLAYERCHATMSG_HPP


#include "apps/openmw-mp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorChatMsg : public PlayerProcessor
    {
    public:
        ProcessorChatMsg()
        {
            BPP_INIT(ID_CHAT_MESSAGE)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            packet.Read();
            Script::CallBackReturn<Script::CallbackIdentity("OnPlayerSendMessage")> result = true;
            Script::Call<Script::CallbackIdentity("OnPlayerSendMessage")>(result, player.getId(), player.chatMessage.c_str());

            if (result)
            {
                player.chatMessage = player.npc.mName + " (" + std::to_string(player.getId()) + "): "
                                      + player.chatMessage + "\n";
                packet.Send(false);
                packet.Send(true);
            }
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERCHATMSG_HPP
