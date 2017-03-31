//
// Created by koncord on 01.04.17.
//

#ifndef OPENMW_PROCESSORGUIMESSAGEBOX_HPP
#define OPENMW_PROCESSORGUIMESSAGEBOX_HPP


#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorGUIMessageBox : public PlayerProcessor
    {
    public:
        ProcessorGUIMessageBox()
        {
            BPP_INIT(ID_GUI_MESSAGEBOX)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            packet.setPlayer(&player);
            packet.Read();

            Script::Call<Script::CallbackIdentity("OnGUIAction")>(player.getId(), (int)player.guiMessageBox.id,
                                                                  player.guiMessageBox.data.c_str());
        }
    };
}



#endif //OPENMW_PROCESSORGUIMESSAGEBOX_HPP
