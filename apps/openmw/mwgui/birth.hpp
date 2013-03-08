#ifndef MWGUI_BIRTH_H
#define MWGUI_BIRTH_H

#include "window_base.hpp"

/*
  This file contains the dialog for choosing a birth sign.
  Layout is defined by resources/mygui/openmw_chargen_race.layout.
 */

namespace MWGui
{
    class BirthDialog : public WindowModal
    {
    public:
        BirthDialog(MWBase::WindowManager& parWindowManager);

        enum Gender
        {
            GM_Male,
            GM_Female
        };

        const std::string &getBirthId() const { return mCurrentBirthId; }
        void setBirthId(const std::string &raceId);

        void setNextButtonShow(bool shown);
        virtual void open();

        // Events
        typedef MyGUI::delegates::CMultiDelegate0 EventHandle_Void;

        /** Event : Back button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventBack;

    protected:
        void onSelectBirth(MyGUI::ListBox* _sender, size_t _index);

        void onOkClicked(MyGUI::Widget* _sender);
        void onBackClicked(MyGUI::Widget* _sender);

    private:
        void updateBirths();
        void updateSpells();

        MyGUI::ListBox* mBirthList;
        MyGUI::Widget*  mSpellArea;
        MyGUI::ImageBox* mBirthImage;
        std::vector<MyGUI::Widget*> mSpellItems;

        std::string mCurrentBirthId;
    };
}
#endif
