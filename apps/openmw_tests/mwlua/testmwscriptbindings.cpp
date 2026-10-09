#include "apps/openmw/mwbase/environment.hpp"
#include "apps/openmw/mwclass/npc.hpp"
#include "apps/openmw/mwdialogue/dialoguemanagerimp.hpp"
#include "apps/openmw/mwlua/context.hpp"
#include "apps/openmw/mwlua/dialoguebindings.hpp"
#include "apps/openmw/mwlua/luamanagerimp.hpp"
#include "apps/openmw/mwlua/mwscriptbindings.hpp"
#include "apps/openmw/mwscript/globalscripts.hpp"
#include "apps/openmw/mwscript/scriptmanagerimp.hpp"
#include "apps/openmw/mwworld/esmstore.hpp"
#include "apps/openmw/mwworld/livecellref.hpp"
#include "apps/openmw/mwworld/worldimp.hpp"
#include "apps/openmw/mwworld/worldmodel.hpp"

#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm3/loaddial.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadscpt.hpp>
#include <components/lua/luastate.hpp>
#include <components/translation/translation.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <sstream>

#include "../mwscript/testutils.hpp"

namespace MWLua
{
    namespace
    {
        class MWLuaMWScriptBindingsTest : public testing::Test
        {
        protected:
            MWBase::Environment mEnvironment;
            MWWorld::World mWorld{ nullptr, -1, {}, {} };
            MWWorld::ESMStore& mStore = mWorld.getStore();
            MWWorld::WorldModel& mWorldModel = mWorld.getWorldModel();
            TestCompilerContext mCompilerContext;
            MWScript::ScriptManager mScriptManager{ mStore, mCompilerContext, 0 };
            Compiler::Extensions mExtensions;
            Translation::Storage mTranslations;
            MWDialogue::DialogueManager mDialogueManager{ mExtensions, mTranslations };
            LuaManager mLuaManager{ nullptr, {} };
            LuaUtil::LuaState mLua{ nullptr, nullptr };
            const ESM::RefId mScriptId = ESM::RefId::stringRefId("result_locals");
            const ESM::RefId mActorId = ESM::RefId::stringRefId("result_actor");

            void SetUp() override
            {
                mEnvironment.setWorld(mWorld);
                mEnvironment.setESMStore(mStore);
                mEnvironment.setWorldModel(mWorldModel);
                mEnvironment.setScriptManager(mScriptManager);
                mEnvironment.setDialogueManager(mDialogueManager);
                mEnvironment.setLuaManager(mLuaManager);
                MWClass::Npc::registerSelf();
                Compiler::registerExtensions(mExtensions);

                ESM::Script script;
                script.blank();
                script.mId = mScriptId;
                script.mScriptText = "begin result_locals\nshort runs\nend\n";
                mStore.insertStatic(script);

                ESM::NPC actor;
                actor.blank();
                actor.mId = mActorId;
                actor.mScript = mScriptId;
                mStore.insertStatic(actor);

                ESM::Dialogue dialogue;
                dialogue.blank();
                dialogue.mId = ESM::RefId::stringRefId("result_topic");
                dialogue.mStringId = "result_topic";
                dialogue.mType = ESM::Dialogue::Topic;
                ESM::DialInfo info;
                info.blank();
                info.mId = ESM::RefId::stringRefId("result_info");
                info.mResultScript = "set runs to ( runs + 1 )";
                dialogue.mInfo.push_back(info);
                info.mId = ESM::RefId::stringRefId("stop_info");
                info.mResultScript = "StopScript result_locals";
                dialogue.mInfo.push_back(info);
                info.mId = ESM::RefId::stringRefId("empty_info");
                info.mResultScript.clear();
                dialogue.mInfo.push_back(info);
                auto stream = std::make_unique<std::stringstream>();
                ESM::ESMWriter writer;
                writer.setFormatVersion(ESM::CurrentSaveGameFormatVersion);
                writer.save(*stream);
                writer.startRecord(ESM::REC_DIAL);
                dialogue.save(writer);
                writer.endRecord(ESM::REC_DIAL);
                for (const ESM::DialInfo& resultInfo : dialogue.mInfo)
                {
                    writer.startRecord(ESM::REC_INFO);
                    resultInfo.save(writer);
                    writer.endRecord(ESM::REC_INFO);
                }
                ESM::ESMReader reader;
                reader.open(std::move(stream), "result-script-dialogue");
                ESM::Dialogue* loadedDialogue = nullptr;
                mStore.load(reader, nullptr, loadedDialogue);
                mStore.getWritable<ESM::Dialogue>().setUp();

                Context context;
                context.mType = Context::Global;
                context.mLuaManager = &mLuaManager;
                context.mLua = &mLua;
                sol::state_view lua = mLua.unsafeState();
                lua["dialogue"] = initCoreDialogueBindings(context);
                lua["mwscript"] = initMWScriptBindings(context);
                lua.script(
                    "function infoById(id)\n"
                    "  for _, info in ipairs(dialogue.topic.records['result_topic'].infos) do\n"
                    "    if info.id == id then return info end\n"
                    "  end\n"
                    "end");
            }

            MWWorld::LiveCellRef<ESM::NPC> makeActor()
            {
                ESM::CellRef ref;
                ref.blank();
                ref.mRefID = mActorId;
                return MWWorld::LiveCellRef<ESM::NPC>(ref, mStore.get<ESM::NPC>().find(mActorId));
            }
        };

        TEST_F(MWLuaMWScriptBindingsTest, shouldInitializeResultScriptLocalsOnlyWhenApplyingDelayedActions)
        {
            auto actor = makeActor();
            const MWWorld::Ptr ptr(&actor);
            mWorldModel.registerPtr(ptr);
            actor.mData.setLocalScriptEnabled(false);
            sol::state_view lua = mLua.unsafeState();
            lua["actor"] = GObject(ptr);

            const sol::protected_function_result result
                = lua.safe_script("return mwscript.runResultScript(infoById('result_info'), actor)");
            ASSERT_TRUE(result.valid());
            ASSERT_TRUE(result.get<bool>());
            EXPECT_TRUE(actor.mData.getLocals().getScriptId().empty());
            EXPECT_TRUE(actor.mData.getLocals().mShorts.empty());

            mLuaManager.applyDelayedActions();

            EXPECT_EQ(actor.mData.getLocals().getScriptId(), mScriptId);
            ASSERT_EQ(actor.mData.getLocals().mShorts.size(), 1);
            EXPECT_EQ(actor.mData.getLocals().mShorts[0], 1);
            EXPECT_FALSE(actor.mData.isLocalScriptEnabled());
        }

        TEST_F(MWLuaMWScriptBindingsTest, shouldPreserveExistingLocalsAcrossQueuedResultScripts)
        {
            auto actor = makeActor();
            const MWWorld::Ptr ptr(&actor);
            mWorldModel.registerPtr(ptr);
            actor.mData.setLocals(*mStore.get<ESM::Script>().find(mScriptId));
            ASSERT_EQ(actor.mData.getLocals().mShorts.size(), 1);
            actor.mData.getLocals().mShorts[0] = 7;
            sol::state_view lua = mLua.unsafeState();
            lua["actor"] = GObject(ptr);

            const sol::protected_function_result result = lua.safe_script(
                "local info = infoById('result_info')\n"
                "assert(mwscript.runResultScript(info, actor))\n"
                "return mwscript.runResultScript(info, actor)");
            ASSERT_TRUE(result.valid());
            ASSERT_TRUE(result.get<bool>());
            EXPECT_EQ(actor.mData.getLocals().mShorts[0], 7);

            mLuaManager.applyDelayedActions();

            EXPECT_EQ(actor.mData.getLocals().mShorts[0], 9);
        }

        TEST_F(MWLuaMWScriptBindingsTest, shouldRunResultScriptWithoutAnAttachedScript)
        {
            ESM::NPC record;
            record.blank();
            record.mId = ESM::RefId::stringRefId("unscripted_actor");
            mStore.insertStatic(record);
            ESM::CellRef ref;
            ref.blank();
            ref.mRefID = record.mId;
            MWWorld::LiveCellRef<ESM::NPC> actor(ref, mStore.get<ESM::NPC>().find(record.mId));
            const MWWorld::Ptr ptr(&actor);
            mWorldModel.registerPtr(ptr);
            auto& globalScripts = mScriptManager.getGlobalScripts();
            globalScripts.addScript(mScriptId);
            sol::state_view lua = mLua.unsafeState();
            lua["actor"] = GObject(ptr);

            const sol::protected_function_result result
                = lua.safe_script("return mwscript.runResultScript(infoById('stop_info'), actor)");
            ASSERT_TRUE(result.valid());
            ASSERT_TRUE(result.get<bool>());
            EXPECT_TRUE(globalScripts.isRunning(mScriptId));

            mLuaManager.applyDelayedActions();

            EXPECT_FALSE(globalScripts.isRunning(mScriptId));
            EXPECT_TRUE(actor.mData.getLocals().getScriptId().empty());
            EXPECT_TRUE(actor.mData.getLocals().mShorts.empty());
        }

        TEST_F(MWLuaMWScriptBindingsTest, shouldRejectResultScriptWithoutCode)
        {
            auto actor = makeActor();
            const MWWorld::Ptr ptr(&actor);
            mWorldModel.registerPtr(ptr);
            sol::state_view lua = mLua.unsafeState();
            lua["actor"] = GObject(ptr);

            const sol::protected_function_result result
                = lua.safe_script("return mwscript.runResultScript(infoById('empty_info'), actor)");
            ASSERT_TRUE(result.valid());
            EXPECT_FALSE(result.get<bool>());
            mLuaManager.applyDelayedActions();
            EXPECT_TRUE(actor.mData.getLocals().getScriptId().empty());
        }

        TEST_F(MWLuaMWScriptBindingsTest, shouldRejectDialogueAndInfoIdsAsResultArguments)
        {
            auto actor = makeActor();
            const MWWorld::Ptr ptr(&actor);
            mWorldModel.registerPtr(ptr);
            sol::state_view lua = mLua.unsafeState();
            lua["actor"] = GObject(ptr);

            const sol::protected_function_result result
                = lua.safe_script("return pcall(mwscript.runResultScript, 'result_topic', 'result_info', actor)");
            ASSERT_TRUE(result.valid());
            EXPECT_FALSE(result.get<bool>());
            mLuaManager.applyDelayedActions();
            EXPECT_TRUE(actor.mData.getLocals().getScriptId().empty());
        }

        TEST_F(MWLuaMWScriptBindingsTest, shouldReadMWScriptVariablesRegardlessOfNameCase)
        {
            auto actor = makeActor();
            const MWWorld::Ptr ptr(&actor);
            mWorldModel.registerPtr(ptr);
            actor.mData.setLocals(*mStore.get<ESM::Script>().find(mScriptId));
            sol::state_view lua = mLua.unsafeState();
            lua["actor"] = GObject(ptr);

            const sol::protected_function_result result = lua.safe_script(
                "local variables = mwscript.getLocalScript(actor).variables\n"
                "variables.Runs = 7\n"
                "assert(variables.runs == 7)\n"
                "return variables.Runs");

            ASSERT_TRUE(result.valid());
            EXPECT_EQ(result.get<double>(), 7);
            EXPECT_EQ(actor.mData.getLocals().mShorts[0], 7);
        }
    }
}
