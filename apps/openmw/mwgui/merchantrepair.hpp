#ifndef OPENMW_MWGUI_MERCHANTREPAIR_H
#define OPENMW_MWGUI_MERCHANTREPAIR_H

#include "../mwworld/ptr.hpp"
#include "windowbase.hpp"

namespace MWGui
{

    class MerchantRepair : public WindowBase
    {
    public:
        MerchantRepair();

        void onOpen() override;

        void setPtr(const MWWorld::Ptr& actor) override;

        std::string_view getWindowIdForLua() const override { return "MerchantRepair"; }

    private:
        MyGUI::ScrollView* mList;
        MyGUI::Button* mOkButton;
        MyGUI::TextBox* mGoldLabel;
        /// List of enabled/repairable items and their index in the full list.
        std::vector<std::pair<MyGUI::Button*, size_t>> mButtons;

        MWWorld::Ptr mActor;

        size_t mControllerFocus;

    protected:
        void onMouseWheel(MyGUI::Widget* _sender, int _rel);
        void onRepairButtonClick(MyGUI::Widget* sender);
        void onOkButtonClick(MyGUI::Widget* sender);
        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
    };

}

#endif
