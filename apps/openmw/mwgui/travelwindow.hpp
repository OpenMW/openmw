#ifndef MWGUI_TravelWINDOW_H
#define MWGUI_TravelWINDOW_H

#include <string>

#include "referenceinterface.hpp"
#include "windowbase.hpp"

namespace ESM
{
    struct Position;
}

namespace MWWorld
{
    class Ptr;
}

namespace MyGUI
{
    class Button;
    class ScrollView;
    class TextBox;
    class Widget;
}

namespace MWGui
{
    class TravelWindow : public ReferenceInterface, public WindowBase
    {
    public:
        TravelWindow();

        void setPtr(const MWWorld::Ptr& actor) override;

    protected:
        MyGUI::Button* mCancelButton;
        MyGUI::TextBox* mPlayerGold;
        MyGUI::TextBox* mDestinations;
        MyGUI::TextBox* mSelect;

        MyGUI::ScrollView* mDestinationsView;

        void onCancelButtonClicked(MyGUI::Widget* _sender);
        void onTravelButtonClick(MyGUI::Widget* _sender);
        void onMouseWheel(MyGUI::Widget* _sender, int _rel);
        void addDestination(const ESM::RefId& name, const ESM::Position& pos, bool interior);
        void clearDestinations();
        int mCurrentY;

        void updateLabels();

        void onReferenceUnavailable() override;
    };
}

#endif
