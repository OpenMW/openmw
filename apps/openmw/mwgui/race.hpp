#ifndef MWGUI_RACE_H
#define MWGUI_RACE_H

#include "../mwrender/characterpreview.hpp"

#include "windowbase.hpp"


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
    class RaceDialog : public WindowModal
    {
    public:
        RaceDialog();

        enum Gender
        {
            GM_Male,
            GM_Female
        };

        const ESM::NPC &getResult() const { return mPreview->getPrototype(); }
        const std::string &getRaceId() const { return mCurrentRaceId; }
        Gender getGender() const { return mGenderIndex == 0 ? GM_Male : GM_Female; }
        // getFace()
        // getHair()

        void setRaceId(const std::string &raceId);
        void setGender(Gender gender) { mGenderIndex = gender == GM_Male ? 0 : 1; }
        // setFace()
        // setHair()

        void setNextButtonShow(bool shown);
        virtual void open();
        virtual void close();

        // Events
        typedef MyGUI::delegates::CMultiDelegate0 EventHandle_Void;

        /** Event : Back button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventBack;

        void doRenderUpdate();

    protected:
        void onHeadRotate(MyGUI::ScrollBar* _sender, size_t _position);

        void onSelectPreviousGender(MyGUI::Widget* _sender);
        void onSelectNextGender(MyGUI::Widget* _sender);

        void onSelectPreviousFace(MyGUI::Widget* _sender);
        void onSelectNextFace(MyGUI::Widget* _sender);

        void onSelectPreviousHair(MyGUI::Widget* _sender);
        void onSelectNextHair(MyGUI::Widget* _sender);

        void onSelectRace(MyGUI::ListBox* _sender, size_t _index);
        void onAccept(MyGUI::ListBox* _sender, size_t _index);

        void onOkClicked(MyGUI::Widget* _sender);
        void onBackClicked(MyGUI::Widget* _sender);

    private:
        void updateRaces();
        void updateSkills();
        void updateSpellPowers();
        void updatePreview();
        void recountParts();

        void getBodyParts (int part, std::vector<std::string>& out);

        std::vector<std::string> mAvailableHeads;
        std::vector<std::string> mAvailableHairs;

        MyGUI::ImageBox*  mPreviewImage;
        MyGUI::ListBox*   mRaceList;
        MyGUI::ScrollBar* mHeadRotate;

        MyGUI::Widget* mSkillList;
        std::vector<MyGUI::Widget*> mSkillItems;

        MyGUI::Widget* mSpellPowerList;
        std::vector<MyGUI::Widget*> mSpellPowerItems;

        int mGenderIndex, mFaceIndex, mHairIndex;

        std::string mCurrentRaceId;

        float mCurrentAngle;

        MWRender::RaceSelectionPreview* mPreview;

        bool mPreviewDirty;
    };
}
#endif
