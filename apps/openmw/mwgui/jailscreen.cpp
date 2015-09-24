#include <MyGUI_ScrollBar.h>

#include <components/misc/rng.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/store.hpp"
#include "../mwworld/class.hpp"

#include "jailscreen.hpp"

namespace MWGui
{
    JailScreen::JailScreen()
        : WindowBase("openmw_jail_screen.layout"),
          mDays(1),
          mFadeTimeRemaining(0),
          mTimeAdvancer(0.01f)
    {
        getWidget(mProgressBar, "ProgressBar");

        setVisible(false);

        mTimeAdvancer.eventProgressChanged += MyGUI::newDelegate(this, &JailScreen::onJailProgressChanged);
        mTimeAdvancer.eventFinished += MyGUI::newDelegate(this, &JailScreen::onJailFinished);

        center();
    }

    void JailScreen::goToJail(int days)
    {
        mDays = days;

        MWBase::Environment::get().getWindowManager()->fadeScreenOut(0.5);
        mFadeTimeRemaining = 0.5;

        setVisible(false);
        mProgressBar->setScrollRange(100+1);
        mProgressBar->setScrollPosition(0);
        mProgressBar->setTrackSize(0);
    }

    void JailScreen::onFrame(float dt)
    {
        mTimeAdvancer.onFrame(dt);

        if (mFadeTimeRemaining <= 0)
            return;

        mFadeTimeRemaining -= dt;

        if (mFadeTimeRemaining <= 0)
        {
            MWWorld::Ptr player = MWMechanics::getPlayer();
            MWBase::Environment::get().getWorld()->teleportToClosestMarker(player, "prisonmarker");

            setVisible(true);
            mTimeAdvancer.run(100);
        }
    }

    void JailScreen::onJailProgressChanged(int cur, int /*total*/)
    {
        mProgressBar->setScrollPosition(0);
        mProgressBar->setTrackSize(static_cast<int>(cur / (float)(mProgressBar->getScrollRange()) * mProgressBar->getLineSize()));
    }

    void JailScreen::onJailFinished()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Jail);
        MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.5);

        MWWorld::Ptr player = MWMechanics::getPlayer();

        MWBase::Environment::get().getWorld()->advanceTime(mDays * 24);
        for (int i=0; i<mDays*24; ++i)
            MWBase::Environment::get().getMechanicsManager()->rest(true);

        std::set<int> skills;
        for (int day=0; day<mDays; ++day)
        {
            int skill = Misc::Rng::rollDice(ESM::Skill::Length);
            skills.insert(skill);

            MWMechanics::SkillValue& value = player.getClass().getNpcStats(player).getSkill(skill);
            if (skill == ESM::Skill::Security || skill == ESM::Skill::Sneak)
                value.setBase(std::min(100, value.getBase()+1));
            else
                value.setBase(value.getBase()-1);
        }

        const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        std::string message;
        if (mDays == 1)
            message = gmst.find("sNotifyMessage42")->getString();
        else
            message = gmst.find("sNotifyMessage43")->getString();

        std::stringstream dayStr;
        dayStr << mDays;
        if (message.find("%d") != std::string::npos)
            message.replace(message.find("%d"), 2, dayStr.str());

        for (std::set<int>::iterator it = skills.begin(); it != skills.end(); ++it)
        {
            std::string skillName = gmst.find(ESM::Skill::sSkillNameIds[*it])->getString();
            std::stringstream skillValue;
            skillValue << player.getClass().getNpcStats(player).getSkill(*it).getBase();
            std::string skillMsg = gmst.find("sNotifyMessage44")->getString();
            if (*it == ESM::Skill::Sneak || *it == ESM::Skill::Security)
                skillMsg = gmst.find("sNotifyMessage39")->getString();

            if (skillMsg.find("%s") != std::string::npos)
                skillMsg.replace(skillMsg.find("%s"), 2, skillName);
            if (skillMsg.find("%d") != std::string::npos)
                skillMsg.replace(skillMsg.find("%d"), 2, skillValue.str());
            message += "\n" + skillMsg;
        }

        std::vector<std::string> buttons;
        buttons.push_back("#{sOk}");
        MWBase::Environment::get().getWindowManager()->interactiveMessageBox(message, buttons);
    }
}
