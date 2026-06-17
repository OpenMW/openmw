#ifndef MWGUI_RACE_H
#define MWGUI_RACE_H

#include "windowbase.hpp"
#include <components/esm/refid.hpp>
#include <memory>

namespace MWRender
{
    class RaceSelectionPreview;
}

namespace ESM
{
    struct NPC;
}

namespace osg
{
    class Group;
}

namespace Resource
{
    class ResourceSystem;
}

namespace MWGui
{
    class RaceDialog : public WindowModal
    {
    public:
        RaceDialog(osg::Group* parent, Resource::ResourceSystem* resourceSystem);

        enum Gender
        {
            GM_Male,
            GM_Female
        };

        const ESM::NPC& getResult() const;
        const ESM::RefId& getRaceId() const { return mCurrentRaceId; }
        Gender getGender() const { return mGenderIndex == 0 ? GM_Male : GM_Female; }

        void setRaceId(const ESM::RefId& raceId);
        void setGender(Gender gender) { mGenderIndex = gender == GM_Male ? 0 : 1; }

        void setNextButtonShow(bool shown);
        void onOpen() override;
        void onClose() override;

        bool exit() override { return false; }

        // Events
        typedef MyGUI::delegates::MultiDelegate<> EventHandle_Void;

        /** Event : Back button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventBack;

        /** Event : Dialog finished, OK button clicked.\n
            signature : void method()\n
        */
        EventHandle_WindowBase eventDone;

    protected:
        void onPreviewScroll(MyGUI::Widget* sender, int delta);
        void onHeadRotate(MyGUI::ScrollBar* sender, size_t position);

        void onSelectPreviousGender(MyGUI::Widget* sender);
        void onSelectNextGender(MyGUI::Widget* sender);

        void onSelectPreviousFace(MyGUI::Widget* sender);
        void onSelectNextFace(MyGUI::Widget* sender);

        void onSelectPreviousHair(MyGUI::Widget* sender);
        void onSelectNextHair(MyGUI::Widget* sender);

        void onSelectRace(MyGUI::ListBox* sender, size_t index);
        void onAccept(MyGUI::ListBox* sender, size_t index);

        void onOkClicked(MyGUI::Widget* sender);
        void onBackClicked(MyGUI::Widget* sender);

    private:
        void updateRaces();
        void updateSkills();
        void updateSpellPowers();
        void updatePreview();
        void recountParts();

        void getBodyParts(int part, std::vector<ESM::RefId>& out);

        osg::Group* mParent;
        Resource::ResourceSystem* mResourceSystem;

        std::vector<ESM::RefId> mAvailableHeads;
        std::vector<ESM::RefId> mAvailableHairs;

        MyGUI::ImageBox* mPreviewImage;
        MyGUI::ListBox* mRaceList;
        MyGUI::ScrollBar* mHeadRotate;
        MyGUI::Button* mBackButton;
        MyGUI::Button* mOkButton;

        MyGUI::Widget* mSkillList;
        std::vector<MyGUI::Widget*> mSkillItems;

        MyGUI::Widget* mSpellPowerList;
        std::vector<MyGUI::Widget*> mSpellPowerItems;

        size_t mGenderIndex, mFaceIndex, mHairIndex;

        ESM::RefId mCurrentRaceId;

        float mCurrentAngle;

        std::unique_ptr<MWRender::RaceSelectionPreview> mPreview;
        std::unique_ptr<MyGUI::ITexture> mPreviewTexture;

        bool mPreviewDirty;

        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
        bool onControllerThumbstickEvent(const SDL_ControllerAxisEvent& arg) override;
    };
}
#endif
