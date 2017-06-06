//
// Created by koncord on 16.04.17.
//

#ifndef OPENMW_PROCESSORGUIMESSAGEBOX_HPP
#define OPENMW_PROCESSORGUIMESSAGEBOX_HPP


#include "apps/openmw/mwmp/PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorGUIMessageBox : public PlayerProcessor
    {
    public:
        ProcessorGUIMessageBox()
        {
            BPP_INIT(ID_GUI_MESSAGEBOX)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (isLocal())
            {
                LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "ID_GUI_MESSAGEBOX, Type %d, MSG %s", player->guiMessageBox.type,
                                   player->guiMessageBox.label.c_str());

                switch(player->guiMessageBox.type)
                {
                    case BasePlayer::GUIMessageBox::MessageBox:
                        Main::get().getGUIController()->showMessageBox(player->guiMessageBox);
                        break;
                    case BasePlayer::GUIMessageBox::CustomMessageBox:
                        Main::get().getGUIController()->showCustomMessageBox(player->guiMessageBox);
                        break;
                    case BasePlayer::GUIMessageBox::InputDialog:
                    case BasePlayer::GUIMessageBox::PasswordDialog:
                        Main::get().getGUIController()->showInputBox(player->guiMessageBox);
                        break;
                    case BasePlayer::GUIMessageBox::ListBox:
                        Main::get().getGUIController()->showDialogList(player->guiMessageBox);
                        break;
                }
            }
        }
    };
}


#endif //OPENMW_PROCESSORGUIMESSAGEBOX_HPP
