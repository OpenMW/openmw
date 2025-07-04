#ifndef MWGUI_STATS_WINDOW_H
#define MWGUI_STATS_WINDOW_H

#include "statswatcher.hpp"
#include "windowpinnablebase.hpp"
#include <components/esm/attr.hpp>
#include <components/esm/refid.hpp>

namespace MWGui
{
    class StatsWindow : public WindowPinnableBase, public NoDrop, public StatsListener
    {
    public:
        typedef std::map<ESM::RefId, int> FactionList;

        StatsWindow(DragAndDrop* drag);

        /// automatically updates all the data in the stats window, but only if it has changed.
        void onFrame(float dt) override;

        void setBar(const std::string& name, const std::string& tname, int val, int max);
        void setPlayerName(const std::string& playerName);

        /// Set value for the given ID.
        void setAttribute(ESM::RefId id, const MWMechanics::AttributeValue& value) override;
        void setValue(std::string_view id, const MWMechanics::DynamicStat<float>& value) override;
        void setValue(std::string_view id, const std::string& value) override;
        void setValue(std::string_view id, int value) override;
        void setValue(ESM::RefId id, const MWMechanics::SkillValue& value) override;
        void configureSkills(const std::vector<ESM::RefId>& major, const std::vector<ESM::RefId>& minor) override;

        void setReputation(int reputation)
        {
            if (reputation != mReputation)
                mChanged = true;
            this->mReputation = reputation;
        }
        void setBounty(int bounty)
        {
            if (bounty != mBounty)
                mChanged = true;
            this->mBounty = bounty;
        }
        void updateSkillArea();

        void onOpen() override { onWindowResize(mMainWidget->castType<MyGUI::Window>()); }

        std::string_view getWindowIdForLua() const override { return "Stats"; }

    private:
        void addSkills(const std::vector<ESM::RefId>& skills, const std::string& titleId,
            const std::string& titleDefault, MyGUI::IntCoord& coord1, MyGUI::IntCoord& coord2);
        void addSeparator(MyGUI::IntCoord& coord1, MyGUI::IntCoord& coord2);
        void addGroup(std::string_view label, MyGUI::IntCoord& coord1, MyGUI::IntCoord& coord2);
        std::pair<MyGUI::TextBox*, MyGUI::TextBox*> addValueItem(std::string_view text, const std::string& value,
            const std::string& state, MyGUI::IntCoord& coord1, MyGUI::IntCoord& coord2);
        MyGUI::Widget* addItem(const std::string& text, MyGUI::IntCoord& coord1, MyGUI::IntCoord& coord2);

        void setFactions(const FactionList& factions);
        void setExpelled(const std::set<ESM::RefId>& expelled);
        void setBirthSign(const ESM::RefId& signId);

        void onWindowResize(MyGUI::Window* window);
        void onMouseWheel(MyGUI::Widget* _sender, int _rel);

        MyGUI::Widget* mLeftPane;
        MyGUI::Widget* mRightPane;

        MyGUI::ScrollView* mSkillView;

        std::vector<ESM::RefId> mMajorSkills, mMinorSkills, mMiscSkills;
        std::map<ESM::RefId, MWMechanics::SkillValue> mSkillValues;
        std::map<ESM::RefId, MyGUI::TextBox*> mAttributeWidgets;
        std::map<ESM::RefId, std::pair<MyGUI::TextBox*, MyGUI::TextBox*>> mSkillWidgetMap;
        std::map<std::string, MyGUI::Widget*> mFactionWidgetMap;
        FactionList mFactions; ///< Stores a list of factions and the current rank
        ESM::RefId mBirthSignId;
        int mReputation, mBounty;
        std::vector<MyGUI::Widget*> mSkillWidgets; //< Skills and other information
        std::set<ESM::RefId> mExpelled;

        bool mChanged;
        const int mMinFullWidth;

    protected:
        void onPinToggled() override;
        void onTitleDoubleClicked() override;
    };
}
#endif
