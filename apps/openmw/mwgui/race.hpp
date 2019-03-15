#ifndef MWGUI_RACE_H
#define MWGUI_RACE_H

#include <memory>

#include "windowbase.hpp"
#include <MyGUI_RenderManager.h>


namespace MWGui
{
    class WindowManager;
}

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

        const ESM::NPC &getResult() const;
        const std::string &getRaceId() const { return mCurrentRaceId; }
        Gender getGender() const { return mGenderIndex == 0 ? GM_Male : GM_Female; }

        void setRaceId(const std::string &raceId);
        void setGender(Gender gender) { mGenderIndex = gender == GM_Male ? 0 : 1; }

        void setNextButtonShow(bool shown);
        virtual void onOpen();
        virtual void onClose();

        bool exit() { return false; }

        // Events
        typedef MyGUI::delegates::CMultiDelegate0 EventHandle_Void;

        /** Event : Back button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventBack;

        /** Event : Dialog finished, OK button clicked.\n
            signature : void method()\n
        */
        EventHandle_WindowBase eventDone;

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

        osg::Group* mParent;
        Resource::ResourceSystem* mResourceSystem;

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

        std::unique_ptr<MWRender::RaceSelectionPreview> mPreview;
        std::unique_ptr<MyGUI::ITexture> mPreviewTexture;

        bool mPreviewDirty;
    };
}
#endif
