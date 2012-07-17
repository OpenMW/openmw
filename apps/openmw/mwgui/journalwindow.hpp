#ifndef MWGUI_JOURNAL_H
#define MWGUI_JOURNAL_H

#include <sstream>
#include <set>
#include <string>
#include <utility>

#include "window_base.hpp"

namespace MWGui
{
    class WindowManager;

    class JournalWindow : public WindowBase
    {
        public:
            JournalWindow(WindowManager& parWindowManager);
            void open();

            virtual void setVisible(bool visible); // only used to play close sound

        private:
            void displayLeftText(std::string text);
            void displayRightText(std::string text);


            /**
            *Called when next/prev button is used.
            */
            void notifyNextPage(MyGUI::WidgetPtr _sender);
            void notifyPrevPage(MyGUI::WidgetPtr _sender);

            static const int sLineHeight;

            MyGUI::WidgetPtr mSkillAreaWidget, mSkillClientWidget;
            MyGUI::ScrollBar* mSkillScrollerWidget;
            int mLastPos, mClientHeight;
            MyGUI::EditPtr mLeftTextWidget;
            MyGUI::EditPtr mRightTextWidget;
            MyGUI::ButtonPtr mPrevBtn;
            MyGUI::ButtonPtr mNextBtn;
            std::vector<std::string> mLeftPages;
            std::vector<std::string> mRightPages;
            int mPageNumber; //store the number of the current left page
            bool mVisible;
    };

}

#endif
