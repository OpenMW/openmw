#include "levelupdialog.hpp"

#include <boost/lexical_cast.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"

namespace MWGui
{

    LevelupDialog::LevelupDialog(MWBase::WindowManager &parWindowManager)
        : WindowBase("openmw_levelup_dialog.layout", parWindowManager)
    {
        getWidget(mOkButton, "OkButton");
        getWidget(mClassImage, "ClassImage");
        getWidget(mLevelText, "LevelText");

        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &LevelupDialog::onOkButtonClicked);

        for (int i=1; i<9; ++i)
        {
            MyGUI::TextBox* t;
            getWidget(t, "AttribVal" + boost::lexical_cast<std::string>(i));

            MyGUI::Button* b;
            getWidget(b, "Attrib" + boost::lexical_cast<std::string>(i));
            b->setUserData (i-1);
            b->eventMouseButtonClick += MyGUI::newDelegate(this, &LevelupDialog::onAttributeClicked);

            mAttributeValues.push_back(t);

            getWidget(t, "AttribMultiplier" + boost::lexical_cast<std::string>(i));

            t->setCaption("x2");
            mAttributeMultipliers.push_back(t);
        }

        center();

        open();
    }

    void LevelupDialog::open()
    {
        center();

        mClassImage->setImageTexture ("textures\\levelup\\acrobat.dds");

        /// \todo replace this with INI-imported texts
        int level = 2;
        mLevelText->setCaptionWithReplacing("#{sLevelUpMenu1} " + boost::lexical_cast<std::string>(level));
    }

    void LevelupDialog::onOkButtonClicked (MyGUI::Widget* sender)
    {
        MWBase::Environment::get().getWindowManager ()->messageBox("#{sNotifyMessage36}", std::vector<std::string>());
    }

    void LevelupDialog::onAttributeClicked (MyGUI::Widget *sender)
    {
        int index = *sender->getUserData<int>();
    }
}
