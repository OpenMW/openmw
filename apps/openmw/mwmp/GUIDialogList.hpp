//
// Created by koncord on 03.11.16.
//

#ifndef OPENMW_GUIDIALOGLIST_HPP
#define OPENMW_GUIDIALOGLIST_HPP


#include "../mwgui/windowbase.hpp"

namespace mwmp
{
    class GUIDialogList : public MWGui::WindowModal
    {
    public:
        GUIDialogList(const std::string &message, const std::vector<std::string> &list);
        void mousePressed(MyGUI::Widget *_widget);
        void onFrame(float frameDuration);
    private:
        bool mMarkedToDelete;
        MyGUI::EditBox *mMessage;
        MyGUI::ListBox *mListBox;
        MyGUI::Button *mButton;
    };
}

#endif //OPENMW_GUIDIALOGLIST_HPP
