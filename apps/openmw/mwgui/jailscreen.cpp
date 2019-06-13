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
            MWBase::Environment::get().getWindowManager()->fadeScreenOut(0.f); // override fade-in caused by cell transition

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

        MWBase::Environment::get().getMechanicsManager()->rest(mDays * 24, true);
        MWBase::Environment::get().getWorld()->advanceTime(mDays * 24);

        std::set<int> skills;
        for (int day=0; day<mDays; ++day)
        {
            int skill = Misc::Rng::rollDice(ESM::Skill::Length);
            skills.insert(skill);

            MWMechanics::SkillValue& value = player.getClass().getNpcStats(player).getSkill(skill);
            if (skill == ESM::Skill::Security || skill == ESM::Skill::Sneak)
                value.setBase(std::min(100, value.getBase()+1));
            else
                value.setBase(std::max(0, value.getBase()-1));
        }

        const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        std::string message;
        if (mDays == 1)
            message = gmst.find("sNotifyMessage42")->mValue.getString();
        else
            message = gmst.find("sNotifyMessage43")->mValue.getString();

        message = Misc::StringUtils::format(message, mDays);

        for (const int& skill : skills)
        {
            std::string skillName = gmst.find(ESM::Skill::sSkillNameIds[skill])->mValue.getString();
            int skillValue = player.getClass().getNpcStats(player).getSkill(skill).getBase();
            std::string skillMsg = gmst.find("sNotifyMessage44")->mValue.getString();
            if (skill == ESM::Skill::Sneak || skill == ESM::Skill::Security)
                skillMsg = gmst.find("sNotifyMessage39")->mValue.getString();

            skillMsg = Misc::StringUtils::format(skillMsg, skillName, skillValue);
            message += "\n" + skillMsg;
        }

        std::vector<std::string> buttons;
        buttons.push_back("#{sOk}");
        MWBase::Environment::get().getWindowManager()->interactiveMessageBox(message, buttons);
    }
}
