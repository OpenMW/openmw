#ifndef MWGUI_CLASS_H
#define MWGUI_CLASS_H

#include <components/esm/attr.hpp>
#include <components/esm/loadclas.hpp>
#include "widgets.hpp"
#include "windowbase.hpp"

namespace MWGui
{
    void setClassImage(MyGUI::ImageBox* imageBox, const std::string& classId);

    class InfoBoxDialog : public WindowModal
    {
    public:
        InfoBoxDialog();

        typedef std::vector<std::string> ButtonList;

        void setText(const std::string &str);
        std::string getText() const;
        void setButtons(ButtonList &buttons);

        void onOpen() override;

        bool exit() override { return false; }

        // Events
        typedef MyGUI::delegates::CMultiDelegate1<int> EventHandle_Int;

        /** Event : Button was clicked.\n
            signature : void method(int index)\n
        */
        EventHandle_Int eventButtonSelected;

    protected:
        void onButtonClicked(MyGUI::Widget* _sender);

    private:

        void fitToText(MyGUI::TextBox* widget);
        void layoutVertically(MyGUI::Widget* widget, int margin);
        MyGUI::Widget* mTextBox;
        MyGUI::TextBox* mText;
        MyGUI::Widget* mButtonBar;
        std::vector<MyGUI::Button*> mButtons;
    };

    // Lets the player choose between 3 ways of creating a class
    class ClassChoiceDialog : public InfoBoxDialog
    {
    public:
        // Corresponds to the buttons that can be clicked
        enum ClassChoice
        {
            Class_Generate = 0,
            Class_Pick = 1,
            Class_Create = 2,
            Class_Back = 3
        };
        ClassChoiceDialog();
    };

    class GenerateClassResultDialog : public WindowModal
    {
    public:
        GenerateClassResultDialog();

        std::string getClassId() const;
        void setClassId(const std::string &classId);

        bool exit() override { return false; }

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
        void onOkClicked(MyGUI::Widget* _sender);
        void onBackClicked(MyGUI::Widget* _sender);

    private:
        MyGUI::ImageBox* mClassImage;
        MyGUI::TextBox*  mClassName;

        std::string mCurrentClassId;
    };

    class PickClassDialog : public WindowModal
    {
    public:
        PickClassDialog();

        const std::string &getClassId() const { return mCurrentClassId; }
        void setClassId(const std::string &classId);

        void setNextButtonShow(bool shown);
        void onOpen() override;

        bool exit() override { return false; }

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
        void onSelectClass(MyGUI::ListBox* _sender, size_t _index);
        void onAccept(MyGUI::ListBox* _sender, size_t _index);

        void onOkClicked(MyGUI::Widget* _sender);
        void onBackClicked(MyGUI::Widget* _sender);

    private:
        void updateClasses();
        void updateStats();

        MyGUI::ImageBox* mClassImage;
        MyGUI::ListBox*  mClassList;
        MyGUI::TextBox*  mSpecializationName;
        Widgets::MWAttributePtr mFavoriteAttribute[2];
        Widgets::MWSkillPtr   mMajorSkill[5];
        Widgets::MWSkillPtr   mMinorSkill[5];

        std::string mCurrentClassId;
    };

    class SelectSpecializationDialog : public WindowModal
    {
    public:
        SelectSpecializationDialog();
        ~SelectSpecializationDialog();

        bool exit() override;

        ESM::Class::Specialization getSpecializationId() const { return mSpecializationId; }

        // Events
        typedef MyGUI::delegates::CMultiDelegate0 EventHandle_Void;

        /** Event : Cancel button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventCancel;

        /** Event : Dialog finished, specialization selected.\n
            signature : void method()\n
        */
        EventHandle_Void eventItemSelected;

    protected:
        void onSpecializationClicked(MyGUI::Widget* _sender);
        void onCancelClicked(MyGUI::Widget* _sender);

    private:
        MyGUI::TextBox *mSpecialization0, *mSpecialization1, *mSpecialization2;

        ESM::Class::Specialization mSpecializationId;
    };

    class SelectAttributeDialog : public WindowModal
    {
    public:
        SelectAttributeDialog();
        ~SelectAttributeDialog();

        bool exit() override;

        ESM::Attribute::AttributeID getAttributeId() const { return mAttributeId; }

        // Events
        typedef MyGUI::delegates::CMultiDelegate0 EventHandle_Void;

        /** Event : Cancel button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventCancel;

        /** Event : Dialog finished, attribute selected.\n
            signature : void method()\n
        */
        EventHandle_Void eventItemSelected;

    protected:
        void onAttributeClicked(Widgets::MWAttributePtr _sender);
        void onCancelClicked(MyGUI::Widget* _sender);

    private:
        ESM::Attribute::AttributeID mAttributeId;
    };

    class SelectSkillDialog : public WindowModal
    {
    public:
        SelectSkillDialog();
        ~SelectSkillDialog();

        bool exit() override;

        ESM::Skill::SkillEnum getSkillId() const { return mSkillId; }

        // Events
        typedef MyGUI::delegates::CMultiDelegate0 EventHandle_Void;

        /** Event : Cancel button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventCancel;

        /** Event : Dialog finished, skill selected.\n
            signature : void method()\n
        */
        EventHandle_Void eventItemSelected;

    protected:
        void onSkillClicked(Widgets::MWSkillPtr _sender);
        void onCancelClicked(MyGUI::Widget* _sender);

    private:
        Widgets::MWSkillPtr mCombatSkill[9];
        Widgets::MWSkillPtr mMagicSkill[9];
        Widgets::MWSkillPtr mStealthSkill[9];

        ESM::Skill::SkillEnum mSkillId;
    };

    class DescriptionDialog : public WindowModal
    {
    public:
        DescriptionDialog();
        ~DescriptionDialog();

        std::string getTextInput() const { return mTextEdit->getCaption(); }
        void setTextInput(const std::string &text) { mTextEdit->setCaption(text); }

        /** Event : Dialog finished, OK button clicked.\n
            signature : void method()\n
        */
        EventHandle_WindowBase eventDone;

    protected:
        void onOkClicked(MyGUI::Widget* _sender);

    private:
        MyGUI::EditBox* mTextEdit;
    };

    class CreateClassDialog : public WindowModal
    {
    public:
        CreateClassDialog();
        virtual ~CreateClassDialog();

        bool exit() override { return false; }

        std::string getName() const;
        std::string getDescription() const;
        ESM::Class::Specialization getSpecializationId() const;
        std::vector<int> getFavoriteAttributes() const;
        std::vector<ESM::Skill::SkillEnum> getMajorSkills() const;
        std::vector<ESM::Skill::SkillEnum> getMinorSkills() const;

        void setNextButtonShow(bool shown);

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
        void onOkClicked(MyGUI::Widget* _sender);
        void onBackClicked(MyGUI::Widget* _sender);

        void onSpecializationClicked(MyGUI::Widget* _sender);
        void onSpecializationSelected();
        void onAttributeClicked(Widgets::MWAttributePtr _sender);
        void onAttributeSelected();
        void onSkillClicked(Widgets::MWSkillPtr _sender);
        void onSkillSelected();
        void onDescriptionClicked(MyGUI::Widget* _sender);
        void onDescriptionEntered(WindowBase* parWindow);
        void onDialogCancel();

        void setSpecialization(int id);

        void update();

    private:
        MyGUI::EditBox*                   mEditName;
        MyGUI::TextBox*                  mSpecializationName;
        Widgets::MWAttributePtr          mFavoriteAttribute0, mFavoriteAttribute1;
        Widgets::MWSkillPtr              mMajorSkill[5];
        Widgets::MWSkillPtr              mMinorSkill[5];
        std::vector<Widgets::MWSkillPtr> mSkills;
        std::string                      mDescription;

        SelectSpecializationDialog       *mSpecDialog;
        SelectAttributeDialog            *mAttribDialog;
        SelectSkillDialog                *mSkillDialog;
        DescriptionDialog                *mDescDialog;

        ESM::Class::Specialization       mSpecializationId;

        Widgets::MWAttributePtr              mAffectedAttribute;
        Widgets::MWSkillPtr              mAffectedSkill;
    };
}
#endif
