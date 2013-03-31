#ifndef MWGUI_SpellBuyingWINDOW_H
#define MWGUI_SpellBuyingWINDOW_H

#include "window_base.hpp"
#include "referenceinterface.hpp"

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
    class SpellBuyingWindow : public ReferenceInterface, public WindowBase
    {
        public:
            SpellBuyingWindow(MWBase::WindowManager& parWindowManager);

            void startSpellBuying(const MWWorld::Ptr& actor);

        protected:
            MyGUI::Button* mCancelButton;
            MyGUI::TextBox* mPlayerGold;

            MyGUI::ScrollView* mSpellsView;

            std::map<MyGUI::Widget*, std::string> mSpellsWidgetMap;

            void onCancelButtonClicked(MyGUI::Widget* _sender);
            void onSpellButtonClick(MyGUI::Widget* _sender);
            void onMouseWheel(MyGUI::Widget* _sender, int _rel);
            void addSpell(const std::string& spellID);
            void clearSpells();
            int mLastPos,mCurrentY;

            static const int sLineHeight;

            void updateLabels();

            virtual void onReferenceUnavailable();

            bool playerHasSpell (const std::string& id);
    };
}

#endif
