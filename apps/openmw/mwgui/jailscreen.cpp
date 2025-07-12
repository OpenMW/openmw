#include <MyGUI_ScrollBar.h>

#include <components/misc/rng.hpp>
#include <components/misc/strings/format.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/store.hpp"

#include "jailscreen.hpp"

namespace MWGui
{
    JailScreen::JailScreen()
        : WindowBase("openmw_jail_screen.layout")
        , mDays(1)
        , mFadeTimeRemaining(0)
        , mTimeAdvancer()
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
        mProgressBar->setScrollRange(100 + 1);
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
            MWBase::Environment::get().getWorld()->teleportToClosestMarker(
                player, ESM::RefId::stringRefId("prisonmarker"));
            MWBase::Environment::get().getWindowManager()->fadeScreenOut(
                0.f); // override fade-in caused by cell transition

            setVisible(true);
            mTimeAdvancer.run(100);
        }
    }

    void JailScreen::onJailProgressChanged(int cur, int /*total*/)
    {
        mProgressBar->setScrollPosition(0);
        mProgressBar->setTrackSize(
            static_cast<int>(cur / (float)(mProgressBar->getScrollRange()) * mProgressBar->getLineSize()));
    }

    void JailScreen::onJailFinished()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Jail);
        MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.5);

        MWWorld::Ptr player = MWMechanics::getPlayer();

        MWBase::Environment::get().getMechanicsManager()->rest(mDays * 24, true);
        MWBase::Environment::get().getWorld()->advanceTime(mDays * 24);

        // We should not worsen corprus when in prison
        player.getClass().getCreatureStats(player).getActiveSpells().skipWorsenings(mDays * 24);

        const auto& skillStore = MWBase::Environment::get().getESMStore()->get<ESM::Skill>();
        std::set<const ESM::Skill*> skills;
        for (int day = 0; day < mDays; ++day)
        {
            auto& prng = MWBase::Environment::get().getWorld()->getPrng();
            const ESM::Skill* skill = skillStore.searchRandom({}, prng);
            skills.insert(skill);

            MWMechanics::SkillValue& value = player.getClass().getNpcStats(player).getSkill(skill->mId);
            if (skill->mId == ESM::Skill::Security || skill->mId == ESM::Skill::Sneak)
                value.setBase(std::min(100.f, value.getBase() + 1));
            else
                value.setBase(std::max(0.f, value.getBase() - 1));
        }

        const MWWorld::Store<ESM::GameSetting>& gmst
            = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();

        std::string message;
        if (mDays == 1)
            message = gmst.find("sNotifyMessage42")->mValue.getString();
        else
            message = gmst.find("sNotifyMessage43")->mValue.getString();

        message = Misc::StringUtils::format(message, mDays);

        for (const ESM::Skill* skill : skills)
        {
            int skillValue = player.getClass().getNpcStats(player).getSkill(skill->mId).getBase();
            std::string skillMsg = gmst.find("sNotifyMessage44")->mValue.getString();
            if (skill->mId == ESM::Skill::Sneak || skill->mId == ESM::Skill::Security)
                skillMsg = gmst.find("sNotifyMessage39")->mValue.getString();

            skillMsg = Misc::StringUtils::format(skillMsg, skill->mName, skillValue);
            message += "\n" + skillMsg;
        }

        std::vector<std::string> buttons;
        buttons.emplace_back("#{Interface:OK}");
        MWBase::Environment::get().getWindowManager()->interactiveMessageBox(message, buttons);
    }
}
