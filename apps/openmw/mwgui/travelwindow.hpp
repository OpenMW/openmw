#ifndef MWGUI_TravelWINDOW_H
#define MWGUI_TravelWINDOW_H


#include "windowbase.hpp"
#include "referenceinterface.hpp"

namespace MyGUI
{
  class Gui;
  class Widget;
}

namespace MWGui
{
    class TravelWindow : public ReferenceInterface, public WindowBase
    {
        public:
            TravelWindow();

            void setPtr (const MWWorld::Ptr& actor) override;

        protected:
            MyGUI::Button* mCancelButton;
            MyGUI::TextBox* mPlayerGold;
            MyGUI::TextBox* mDestinations;
            MyGUI::TextBox* mSelect;

            MyGUI::ScrollView* mDestinationsView;

            void onCancelButtonClicked(MyGUI::Widget* _sender);
            void onTravelButtonClick(MyGUI::Widget* _sender);
            void onMouseWheel(MyGUI::Widget* _sender, int _rel);
            void addDestination(const std::string& name, ESM::Position pos, bool interior);
            void clearDestinations();
            int mCurrentY;

            void updateLabels();

            void onReferenceUnavailable() override;
    };
}

#endif
