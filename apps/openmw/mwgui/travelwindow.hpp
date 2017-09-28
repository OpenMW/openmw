#ifndef MWGUI_TravelWINDOW_H
#define MWGUI_TravelWINDOW_H


#include "windowbase.hpp"
#include "referenceinterface.hpp"
#include "../mwbase/world.hpp"

namespace MyGUI
{
  class Gui;
  class Widget;
}

namespace MWGui
{
    class WindowManager;
}


namespace MWGui
{
    class TravelWindow : public ReferenceInterface, public WindowBase
    {
        public:
            TravelWindow();

            virtual void onFrame();

            virtual void exit();

            void startTravel(const MWWorld::Ptr& actor);

        protected:
            MyGUI::Button* mCancelButton;
            MyGUI::TextBox* mPlayerGold;
            MyGUI::TextBox* mDestinations;
            MyGUI::TextBox* mSelect;

            MyGUI::ScrollView* mDestinationsView;

            MWBase::World::TravelTarget mTravelTarget;

            void onCancelButtonClicked(MyGUI::Widget* _sender);
            void onTravelButtonClick(MyGUI::Widget* _sender);
            void travel();
            void onMouseWheel(MyGUI::Widget* _sender, int _rel);
            void addDestination(const std::string& name, ESM::Position pos, bool interior);
            void clearDestinations();
            int mCurrentY;

            static const int sLineHeight;

            void updateLabels();

            virtual void onReferenceUnavailable();
    };
}

#endif
