#ifndef MWGUI_LEVELUPDIALOG_H
#define MWGUI_LEVELUPDIALOG_H

#include "window_base.hpp"

namespace MWGui
{

    class LevelupDialog : public WindowBase
    {
    public:
        LevelupDialog(MWBase::WindowManager& parWindowManager);

        virtual void open();

    private:
        MyGUI::Button* mOkButton;
        MyGUI::ImageBox* mClassImage;
        MyGUI::TextBox* mLevelText;

        std::vector<MyGUI::TextBox*> mAttributeValues;
        std::vector<MyGUI::TextBox*> mAttributeMultipliers;

        void onOkButtonClicked (MyGUI::Widget* sender);
        void onAttributeClicked (MyGUI::Widget* sender);
    };

}

#endif
