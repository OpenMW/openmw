#include "alchemy.hpp"

#include "../luamanager.hpp"
#include "../util.hpp"

#include "../../mwbase/environment.hpp"
#include "../../mwbase/world.hpp"

#include "../../mwmechanics/actorutil.hpp"
#include "../../mwmechanics/npcstats.hpp"

#include "../../mwworld/class.hpp"
#include "../../mwworld/esmstore.hpp"

namespace MWLua
{
    void bindTES3Faction()
    {
        // Get our lua state.
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        // Binding for ESM::RankData
        {
            // Start our usertype. We must finish this with state.set_usertype.
            auto usertypeDefinition = state.create_simple_usertype<ESM::RankData>();
            usertypeDefinition.set("new", sol::no_constructor);

            // Basic property binding.
            usertypeDefinition.set("reputation", &ESM::RankData::mFactReaction);

            // Indirect bindings to unions and arrays.
            usertypeDefinition.set("attributes", sol::readonly_property([](ESM::RankData& self)
            {
                auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
                sol::state& state = stateHandle.state;

                sol::table result = state.create_table();
                result[1] = self.mAttribute1;
                result[2] = self.mAttribute2;

                return result;
            }));
            usertypeDefinition.set("skills", sol::readonly_property([](ESM::RankData& self)
            {
                auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
                sol::state& state = stateHandle.state;

                sol::table result = state.create_table();
                result[1] = self.mSkill1;
                result[2] = self.mSkill2;

                return result;
            }));

            // Finish up our usertype.
            state.set_usertype("tes3factionRank", usertypeDefinition);
        }

        // Binding for ESM::Faction
        {
            // Start our usertype. We must finish this with state.set_usertype.
            auto usertypeDefinition = state.create_simple_usertype<ESM::Faction>();

            usertypeDefinition.set("new", sol::no_constructor);

            // Basic property binding.
            usertypeDefinition.set("id", &ESM::Faction::mId);
            usertypeDefinition.set("name", &ESM::Faction::mName);
            usertypeDefinition.set("hidden", sol::property(
                [](ESM::Faction& self) { return self.mData.mIsHidden; },
                [](ESM::Faction& self, bool value) { self.mData.mIsHidden = value; }
            ));

            // Indirect bindings to unions and arrays.
            usertypeDefinition.set("attributes", sol::property([](ESM::Faction& self) { return std::ref(self.mData.mAttribute); }));
            usertypeDefinition.set("ranks", sol::readonly_property([](ESM::Faction& self) { return std::ref(self.mData.mRankData); }));
            usertypeDefinition.set("rankNames", sol::readonly_property([](ESM::Faction& self) { return std::ref(self.mRanks); }));
            usertypeDefinition.set("skills", sol::property([](ESM::Faction& self) { return std::ref(self.mData.mSkills); }));
            usertypeDefinition.set("reactions", sol::property([](ESM::Faction& self) { return std::ref(self.mReactions); }));

            // Functions exposed as properties.
            usertypeDefinition.set("playerReputation", sol::property(
                [](ESM::Faction& self)
                {
                    MWWorld::Ptr player = MWMechanics::getPlayer();
                    MWMechanics::NpcStats &stats = player.getClass().getNpcStats(player);
                    return stats.getFactionReputation(self.mId);
                },
                [](ESM::Faction& self, bool value)
                {
                    MWWorld::Ptr player = MWMechanics::getPlayer();
                    MWMechanics::NpcStats &stats = player.getClass().getNpcStats(player);
                    stats.setFactionReputation(self.mId, value);
                }
            ));
            usertypeDefinition.set("playerJoined", sol::property(
                [](ESM::Faction& self)
                {
                    MWWorld::Ptr player = MWMechanics::getPlayer();
                    MWMechanics::NpcStats &stats = player.getClass().getNpcStats(player);
                    return stats.isInFaction(self.mId);
                },
                [](ESM::Faction& self, bool value)
                {
                    MWWorld::Ptr player = MWMechanics::getPlayer();
                    MWMechanics::NpcStats &stats = player.getClass().getNpcStats(player);
                    if (value)
                        stats.joinFaction(self.mId);
                    else
                    {
                        int currentRank = stats.getFactionRank(self.mId);
                        if (currentRank < 0) return;
                        for (int i = 0; i <= currentRank; i++)
                            stats.lowerRank(self.mId);
                    }
                }
            ));
            usertypeDefinition.set("playerExpelled", sol::property(
                [](ESM::Faction& self)
                {
                    MWWorld::Ptr player = MWMechanics::getPlayer();
                    MWMechanics::NpcStats &stats = player.getClass().getNpcStats(player);
                    return stats.getExpelled(self.mId);
                },
                [](ESM::Faction& self, bool value)
                {
                    MWWorld::Ptr player = MWMechanics::getPlayer();
                    MWMechanics::NpcStats &stats = player.getClass().getNpcStats(player);
                    if (value)
                        stats.expell(self.mId);
                    else
                        stats.clearExpelled(self.mId);
                }
            ));
            usertypeDefinition.set("playerRank", sol::property(
                [](ESM::Faction& self)
                {
                    MWWorld::Ptr player = MWMechanics::getPlayer();
                    MWMechanics::NpcStats &stats = player.getClass().getNpcStats(player);
                    return stats.getFactionRank(self.mId);
                },
                [](ESM::Faction& self, int value)
                {
                    MWWorld::Ptr player = MWMechanics::getPlayer();
                    MWMechanics::NpcStats &stats = player.getClass().getNpcStats(player);
                    int currentRank = stats.getFactionRank(self.mId);
                    if (value < 0 && currentRank < 0) return;
                    else if (value >=0 && currentRank < 0)
                    {
                        stats.joinFaction(self.mId);
                        for (int i = 0; i < value; i++)
                            stats.raiseRank(self.mId);
                    }
                    else if (value < 0 && currentRank >= 0)
                    {
                        for (int i = 0; i <= currentRank; i++)
                            stats.lowerRank(self.mId);
                    }
                    else //if (value >=0 && currentRank >0 0)
                    {
                        int diff = value - currentRank;
                        if (diff >= 0)
                        {
                            for (int i = 0; i < diff; i++)
                                stats.raiseRank(self.mId);
                        }
                        else
                        {
                            diff = abs(diff);
                            for (int i = 0; i < diff; i++)
                                stats.lowerRank(self.mId);
                        }
                    }
                }
            ));

            state.set_usertype("tes3faction", usertypeDefinition);
        }
    }
}
