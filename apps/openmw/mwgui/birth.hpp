#ifndef MWGUI_BIRTH_H
#define MWGUI_BIRTH_H

#include <openengine/gui/layout.hpp>

namespace MWWorld
{
    class Environment;
}

/*
  This file contains the dialog for choosing a birth sign.
  Layout is defined by resources/mygui/openmw_chargen_race_layout.xml.
 */

namespace MWGui
{
    using namespace MyGUI;

    class BirthDialog : public OEngine::GUI::Layout
    {
    public:
        BirthDialog(MWWorld::Environment& environment);

        enum Gender
        {
            GM_Male,
            GM_Female
        };

        const std::string &getBirthId() const { return currentBirthId; }
        void setBirthId(const std::string &raceId);

        void setNextButtonShow(bool shown);
        void open();

        // Events
        typedef delegates::CDelegate0 EventHandle_Void;

        /** Event : Back button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventBack;

        /** Event : Dialog finished, OK button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventDone;

    protected:
        void onSelectBirth(MyGUI::List* _sender, size_t _index);

        void onOkClicked(MyGUI::Widget* _sender);
        void onBackClicked(MyGUI::Widget* _sender);

    private:
        void updateBirths();
        void updateSpells();

        MWWorld::Environment& environment;

        MyGUI::ListPtr    birthList;
        MyGUI::WidgetPtr  spellArea;
        MyGUI::StaticImagePtr birthImage;
        std::vector<MyGUI::WidgetPtr> spellItems;

        std::string currentBirthId;
    };
}
#endif
