//
// Created by koncord on 20.07.16.
//

#ifndef OPENMW_GUICONTROLLER_HPP
#define OPENMW_GUICONTROLLER_HPP

#include <components/settings/settings.hpp>
#include <apps/openmw/mwgui/textinput.hpp>
#include <apps/openmw/mwgui/mode.hpp>
#include <components/openmw-mp/Base/BasePlayer.hpp>
#include "GUIChat.hpp"
#include "PlayerMarkerCollection.hpp"

namespace MWGui
{
    class LocalMapBase;
    class MapWindow;
}

namespace mwmp
{
    class GUIController
    {
    public:
        GUIController();
        ~GUIController();
        void cleanup();
        void setupChat(const Settings::Manager &manager);

        void PrintChatMessage(std::string &msg);
        void setChatVisible(bool chatVisible);

        void ShowMessageBox(const BasePlayer::GUIMessageBox &guiMessageBox);
        void ShowCustomMessageBox(const BasePlayer::GUIMessageBox &guiMessageBox);
        void ShowInputBox(const BasePlayer::GUIMessageBox &guiMessageBox);

        /// Return true if any tes3mp gui element in active state
        bool HaveFocusedElement();
        /// Returns 0 if there was no events
        bool pressedKey(int key);

        void update(float dt);

        void WM_UpdateVisible(MWGui::GuiMode mode);

        void updatePlayersMarkers(MWGui::LocalMapBase *localMapBase);
        void updateGlobalMapMarkerTooltips(MWGui::MapWindow *pWindow);

        ESM::CustomMarker CreateMarker(const RakNet::RakNetGUID &guid);
        PlayerMarkerCollection mPlayerMarkers;
    private:
        void setGlobalMapMarkerTooltip(MWGui::MapWindow *mapWindow ,MyGUI::Widget* markerWidget, int x, int y);

    private:
        GUIChat *mChat;
        int keySay;
        int keyChatMode;

        long id;
        bool calledMessageBox;
        MWGui::TextInputDialog *mInputBox;
        void OnInputBoxDone(MWGui::WindowBase* parWindow);
        //MyGUI::Widget *oldFocusWidget, *currentFocusWidget;
    };
}

#endif //OPENMW_GUICONTROLLER_HPP
