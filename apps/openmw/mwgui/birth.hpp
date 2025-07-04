#ifndef MWGUI_BIRTH_H
#define MWGUI_BIRTH_H

#include "windowbase.hpp"
#include <components/esm/refid.hpp>

namespace MWGui
{
    class BirthDialog : public WindowModal
    {
    public:
        BirthDialog();

        enum Gender
        {
            GM_Male,
            GM_Female
        };

        const ESM::RefId& getBirthId() const { return mCurrentBirthId; }
        void setBirthId(const ESM::RefId& raceId);

        void setNextButtonShow(bool shown);
        void onOpen() override;

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
        void onSelectBirth(MyGUI::ListBox* _sender, size_t _index);

        void onAccept(MyGUI::ListBox* _sender, size_t index);
        void onOkClicked(MyGUI::Widget* _sender);
        void onBackClicked(MyGUI::Widget* _sender);

    private:
        void updateBirths();
        void updateSpells();

        MyGUI::ListBox* mBirthList;
        MyGUI::ScrollView* mSpellArea;
        MyGUI::ImageBox* mBirthImage;
        std::vector<MyGUI::Widget*> mSpellItems;

        ESM::RefId mCurrentBirthId;
    };
}
#endif
