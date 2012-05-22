#include "settingswindow.hpp"

#include <OgreRoot.h>
#include <OgreRenderSystem.h>

#include <components/settings/settings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwworld/world.hpp"

#include "window_manager.hpp"

namespace MWGui
{
    SettingsWindow::SettingsWindow(WindowManager& parWindowManager) :
        WindowBase("openmw_settings_window_layout.xml", parWindowManager)
    {
        getWidget(mOkButton, "OkButton");
        getWidget(mResolutionList, "ResolutionList");
        getWidget(mMenuTransparencySlider, "MenuTransparencySlider");
        getWidget(mViewDistanceSlider, "ViewDistanceSlider");

        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SettingsWindow::onOkButtonClicked);
        mMenuTransparencySlider->eventScrollChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onSliderChangePosition);
        mViewDistanceSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &SettingsWindow::onSliderChangePosition);

        center();

        int okSize = mOkButton->getTextSize().width + 24;
        mOkButton->setCoord(mMainWidget->getWidth()-16-okSize, mOkButton->getTop(),
                            okSize, mOkButton->getHeight());

        // fill resolution list
        Ogre::RenderSystem* rs = Ogre::Root::getSingleton().getRenderSystem();
        const Ogre::StringVector& videoModes = rs->getConfigOptions()["Video Mode"].possibleValues;
        for (Ogre::StringVector::const_iterator it=videoModes.begin();
            it!=videoModes.end(); ++it)
        {
            mResolutionList->addItem(*it);
        }

        // read settings
        int menu_transparency = (mMenuTransparencySlider->getScrollRange()-1) * Settings::Manager::getFloat("menu transparency", "GUI");
        mMenuTransparencySlider->setScrollPosition(menu_transparency);
        onSliderChangePosition(mMenuTransparencySlider, menu_transparency);
    }

    void SettingsWindow::onOkButtonClicked(MyGUI::Widget* _sender)
    {
        mWindowManager.setGuiMode(GM_Game);
    }

    void SettingsWindow::onSliderChangePosition(MyGUI::ScrollBar* scroller, size_t pos)
    {
        float val = pos / float(scroller->getScrollRange()-1);
        if (scroller == mMenuTransparencySlider)
        {
            Settings::Manager::setFloat("menu transparency", "GUI", val);
        }

        MWBase::Environment::get().getWorld()->processChangedSettings(Settings::Manager::apply());
    }
}
