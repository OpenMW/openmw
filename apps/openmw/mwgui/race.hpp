#ifndef MWGUI_RACE_H
#define MWGUI_RACE_H

#include <components/esm_store/store.hpp>

#include "window_base.hpp"

#include <boost/array.hpp>

namespace MWGui
{
    class WindowManager;
}

/*
  This file contains the dialog for choosing a race.
  Layout is defined by resources/mygui/openmw_chargen_race.layout.
 */

namespace MWGui
{
    using namespace MyGUI;

    class RaceDialog : public WindowBase
    {
    public:
        RaceDialog(WindowManager& parWindowManager);

        enum Gender
        {
            GM_Male,
            GM_Female
        };

        const std::string &getRaceId() const { return mCurrentRaceId; }
        Gender getGender() const { return mGenderIndex == 0 ? GM_Male : GM_Female; }
        // getFace()
        // getHair()

        void setRaceId(const std::string &raceId);
        void setGender(Gender gender) { mGenderIndex = gender == GM_Male ? 0 : 1; }
        // setFace()
        // setHair()

        void setNextButtonShow(bool shown);
        void open();

        // Events
        typedef delegates::CMultiDelegate0 EventHandle_Void;

        /** Event : Back button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventBack;

    protected:
        void onHeadRotate(MyGUI::ScrollBar* _sender, size_t _position);

        void onSelectPreviousGender(MyGUI::Widget* _sender);
        void onSelectNextGender(MyGUI::Widget* _sender);

        void onSelectPreviousFace(MyGUI::Widget* _sender);
        void onSelectNextFace(MyGUI::Widget* _sender);

        void onSelectPreviousHair(MyGUI::Widget* _sender);
        void onSelectNextHair(MyGUI::Widget* _sender);

        void onSelectRace(MyGUI::ListBox* _sender, size_t _index);

        void onOkClicked(MyGUI::Widget* _sender);
        void onBackClicked(MyGUI::Widget* _sender);

    private:
        void updateRaces();
        void updateSkills();
        void updateSpellPowers();

        MyGUI::CanvasPtr  mAppearanceBox;
        MyGUI::ListBox*   mRaceList;
        MyGUI::ScrollBar* mHeadRotate;

        MyGUI::WidgetPtr mSkillList;
        std::vector<MyGUI::WidgetPtr> mSkillItems;

        MyGUI::WidgetPtr mSpellPowerList;
        std::vector<MyGUI::WidgetPtr> mSpellPowerItems;

        int mGenderIndex, mFaceIndex, mHairIndex;
        int mFaceCount, mHairCount;

        std::string mCurrentRaceId;
    };
}
#endif
