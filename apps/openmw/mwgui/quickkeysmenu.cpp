#include "quickkeysmenu.hpp"

#include <boost/lexical_cast.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/player.hpp"

#include "windowmanagerimp.hpp"
#include "itemselection.hpp"

namespace MWGui
{

    QuickKeysMenu::QuickKeysMenu(MWBase::WindowManager& parWindowManager)
        : WindowBase("openmw_quickkeys_menu.layout", parWindowManager)
        , mAssignDialog(0)
        , mItemSelectionDialog(0)
        , mMagicSelectionDialog(0)
    {
        getWidget(mOkButton, "OKButton");
        getWidget(mInstructionLabel, "InstructionLabel");

        mMainWidget->setSize(mMainWidget->getWidth(),
                             mMainWidget->getHeight() + (mInstructionLabel->getTextSize().height - mInstructionLabel->getHeight()));

        int okButtonWidth = mOkButton->getTextSize ().width + 24;
        mOkButton->setCoord(mOkButton->getLeft() - (okButtonWidth - mOkButton->getWidth()),
                            mOkButton->getTop(),
                            okButtonWidth,
                            mOkButton->getHeight());
        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &QuickKeysMenu::onOkButtonClicked);

        center();


        for (int i = 0; i < 10; ++i)
        {
            MyGUI::Button* button;
            getWidget(button, "QuickKey" + boost::lexical_cast<std::string>(i+1));

            if (i != 9) // 10th quick key is always set to hand-to-hand
                button->eventMouseButtonClick += MyGUI::newDelegate(this, &QuickKeysMenu::onQuickKeyButtonClicked);

            unassign(button, i);

            mQuickKeyButtons.push_back(button);

        }
    }

    QuickKeysMenu::~QuickKeysMenu()
    {
        delete mAssignDialog;
    }

    void QuickKeysMenu::unassign(MyGUI::Widget* key, int index)
    {
        while (key->getChildCount ())
            MyGUI::Gui::getInstance ().destroyWidget (key->getChildAt(0));

        MyGUI::TextBox* textBox = key->createWidgetReal<MyGUI::TextBox>("SandText", MyGUI::FloatCoord(0,0,1,1), MyGUI::Align::Default);
        textBox->setTextAlign (MyGUI::Align::Center);
        textBox->setCaption (boost::lexical_cast<std::string>(index+1));
        textBox->setNeedMouseFocus (false);
    }

    void QuickKeysMenu::onQuickKeyButtonClicked(MyGUI::Widget* sender)
    {
        int index = -1;
        for (int i = 0; i < 10; ++i)
        {
            if (sender == mQuickKeyButtons[i] || sender->getParent () == mQuickKeyButtons[i])
            {
                index = i;
                break;
            }
        }
        assert(index != -1);
        mSelectedIndex = index;

        {
            // open assign dialog
            if (!mAssignDialog)
                mAssignDialog = new QuickKeysMenuAssign(mWindowManager, this);
            mAssignDialog->setVisible (true);
        }
    }

    void QuickKeysMenu::onOkButtonClicked (MyGUI::Widget *sender)
    {
        mWindowManager.removeGuiMode(GM_QuickKeysMenu);
    }


    void QuickKeysMenu::onItemButtonClicked(MyGUI::Widget* sender)
    {
        if (!mItemSelectionDialog )
        {
            mItemSelectionDialog = new ItemSelectionDialog("#{sQuickMenu6}", ContainerBase::Filter_All, mWindowManager);
            mItemSelectionDialog->eventItemSelected += MyGUI::newDelegate(this, &QuickKeysMenu::onAssignItem);
            mItemSelectionDialog->eventDialogCanceled += MyGUI::newDelegate(this, &QuickKeysMenu::onAssignItemCancel);
        }
        mItemSelectionDialog->setVisible(true);
        mItemSelectionDialog->openContainer(MWBase::Environment::get().getWorld()->getPlayer().getPlayer());
        mItemSelectionDialog->drawItems ();

        mAssignDialog->setVisible (false);
    }

    void QuickKeysMenu::onMagicButtonClicked(MyGUI::Widget* sender)
    {
        if (!mMagicSelectionDialog )
        {
            mMagicSelectionDialog = new MagicSelectionDialog(mWindowManager, this);
        }
        mMagicSelectionDialog->setVisible(true);

        mAssignDialog->setVisible (false);
    }

    void QuickKeysMenu::onUnassignButtonClicked(MyGUI::Widget* sender)
    {
        unassign(mQuickKeyButtons[mSelectedIndex], mSelectedIndex);
        mAssignDialog->setVisible (false);
    }

    void QuickKeysMenu::onCancelButtonClicked(MyGUI::Widget* sender)
    {
        mAssignDialog->setVisible (false);
    }

    void QuickKeysMenu::onAssignItem(MWWorld::Ptr item)
    {
        MyGUI::Button* button = mQuickKeyButtons[mSelectedIndex];
        while (button->getChildCount ())
            MyGUI::Gui::getInstance ().destroyWidget (button->getChildAt(0));

        MyGUI::ImageBox* image = button->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(9, 8, 42, 42), MyGUI::Align::Default);
        image->setUserString ("ToolTipType", "ItemPtr");
        image->setUserData(item);
        std::string path = std::string("icons\\");
        path += MWWorld::Class::get(item).getInventoryIcon(item);
        int pos = path.rfind(".");
        path.erase(pos);
        path.append(".dds");
        image->setImageTexture (path);
        image->eventMouseButtonClick += MyGUI::newDelegate(this, &QuickKeysMenu::onQuickKeyButtonClicked);

        mItemSelectionDialog->setVisible(false);
    }

    void QuickKeysMenu::onAssignItemCancel()
    {
        mItemSelectionDialog->setVisible(false);
    }

    void QuickKeysMenu::onAssignMagicItem (MWWorld::Ptr item)
    {

    }

    void QuickKeysMenu::onAssignMagic (const std::string& spellId)
    {

    }

    void QuickKeysMenu::onAssignMagicCancel ()
    {
        mMagicSelectionDialog->setVisible(false);
    }

    // ---------------------------------------------------------------------------------------------------------

    QuickKeysMenuAssign::QuickKeysMenuAssign (MWBase::WindowManager &parWindowManager, QuickKeysMenu* parent)
        : WindowModal("openmw_quickkeys_menu_assign.layout", parWindowManager)
        , mParent(parent)
    {
        getWidget(mLabel, "Label");
        getWidget(mItemButton, "ItemButton");
        getWidget(mMagicButton, "MagicButton");
        getWidget(mUnassignButton, "UnassignButton");
        getWidget(mCancelButton, "CancelButton");

        mItemButton->eventMouseButtonClick += MyGUI::newDelegate(mParent, &QuickKeysMenu::onItemButtonClicked);
        mMagicButton->eventMouseButtonClick += MyGUI::newDelegate(mParent, &QuickKeysMenu::onMagicButtonClicked);
        mUnassignButton->eventMouseButtonClick += MyGUI::newDelegate(mParent, &QuickKeysMenu::onUnassignButtonClicked);
        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(mParent, &QuickKeysMenu::onCancelButtonClicked);


        int maxWidth = mItemButton->getTextSize ().width + 24;
        maxWidth = std::max(maxWidth, mMagicButton->getTextSize ().width + 24);
        maxWidth = std::max(maxWidth, mUnassignButton->getTextSize ().width + 24);
        maxWidth = std::max(maxWidth, mCancelButton->getTextSize ().width + 24);

        mMainWidget->setSize(maxWidth + 24, mMainWidget->getHeight());
        mLabel->setSize(maxWidth, mLabel->getHeight());

        mItemButton->setCoord((maxWidth - mItemButton->getTextSize().width-24)/2 + 8,
                              mItemButton->getTop(),
                              mItemButton->getTextSize().width + 24,
                              mItemButton->getHeight());
        mMagicButton->setCoord((maxWidth - mMagicButton->getTextSize().width-24)/2 + 8,
                              mMagicButton->getTop(),
                              mMagicButton->getTextSize().width + 24,
                              mMagicButton->getHeight());
        mUnassignButton->setCoord((maxWidth - mUnassignButton->getTextSize().width-24)/2 + 8,
                              mUnassignButton->getTop(),
                              mUnassignButton->getTextSize().width + 24,
                              mUnassignButton->getHeight());
        mCancelButton->setCoord((maxWidth - mCancelButton->getTextSize().width-24)/2 + 8,
                              mCancelButton->getTop(),
                              mCancelButton->getTextSize().width + 24,
                              mCancelButton->getHeight());

        center();
    }


    // ---------------------------------------------------------------------------------------------------------

    MagicSelectionDialog::MagicSelectionDialog(MWBase::WindowManager &parWindowManager, QuickKeysMenu* parent)
        : WindowModal("openmw_magicselection_dialog.layout", parWindowManager)
        , mParent(parent)
    {
        getWidget(mCancelButton, "CancelButton");
        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &MagicSelectionDialog::onCancelButtonClicked);

        center();
    }

    void MagicSelectionDialog::onCancelButtonClicked (MyGUI::Widget *sender)
    {
        mParent->onAssignMagicCancel ();
    }

}
