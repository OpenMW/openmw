#include "apps/openmw/mwbase/environment.hpp"
#include "apps/openmw/mwclass/misc.hpp"
#include "apps/openmw/mwscript/interpretercontext.hpp"
#include "apps/openmw/mwscript/scriptmanagerimp.hpp"
#include "apps/openmw/mwworld/esmstore.hpp"
#include "apps/openmw/mwworld/livecellref.hpp"
#include "apps/openmw/mwworld/localscripts.hpp"

#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm3/loadmisc.hpp>
#include <components/esm3/loadscpt.hpp>
#include <components/esm3/objectstate.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <sstream>

#include "../mwscript/testutils.hpp"

namespace MWWorld
{
    namespace
    {
        class MWWorldLocalScriptsTest : public testing::Test
        {
        protected:
            MWBase::Environment mEnvironment;
            ESMStore mStore;
            TestCompilerContext mCompilerContext;
            MWScript::ScriptManager mScriptManager{ mStore, mCompilerContext, 0 };
            LocalScripts mLocalScripts{ mStore };
            const ESM::RefId mScriptId = ESM::RefId::stringRefId("local_counter");
            const ESM::RefId mRecordId = ESM::RefId::stringRefId("scripted_item");

            void SetUp() override
            {
                mEnvironment.setESMStore(mStore);
                mEnvironment.setScriptManager(mScriptManager);
                MWClass::Miscellaneous::registerSelf();

                ESM::Script script;
                script.mId = mScriptId;
                script.blank();
                script.mScriptText = "begin local_counter\nshort runs\nset runs to ( runs + 1 )\nend\n";
                mStore.insertStatic(script);

                ESM::Miscellaneous item;
                item.blank();
                item.mId = mRecordId;
                item.mScript = mScriptId;
                mStore.insertStatic(item);
            }

            LiveCellRef<ESM::Miscellaneous> makeReference()
            {
                ESM::CellRef ref;
                ref.blank();
                ref.mRefID = mRecordId;
                return LiveCellRef<ESM::Miscellaneous>(ref, mStore.get<ESM::Miscellaneous>().find(mRecordId));
            }

            void runFrame()
            {
                mLocalScripts.startIteration();
                std::pair<ESM::RefId, Ptr> script;
                while (mLocalScripts.getNext(script))
                {
                    MWScript::InterpreterContext context(&script.second.getRefData().getLocals(), script.second);
                    ASSERT_TRUE(mScriptManager.run(script.first, context));
                }
            }

            double runCount(RefData& data) { return data.getLocals().getVarAsDouble(mScriptId, "runs"); }
        };

        TEST_F(MWWorldLocalScriptsTest, shouldPauseOneInstanceAndAllowOneExecutionWithoutResuming)
        {
            auto first = makeReference();
            auto second = makeReference();
            const Ptr firstPtr(&first);
            const Ptr secondPtr(&second);
            mLocalScripts.add(mScriptId, firstPtr);
            mLocalScripts.add(mScriptId, secondPtr);
            runFrame();

            first.mData.setLocalScriptEnabled(false);
            EXPECT_FALSE(mLocalScripts.isRunning(mScriptId, firstPtr));
            EXPECT_TRUE(mLocalScripts.isRunning(mScriptId, secondPtr));
            runFrame();
            EXPECT_EQ(runCount(first.mData), 1);
            EXPECT_EQ(runCount(second.mData), 2);

            MWScript::InterpreterContext context(&first.mData.getLocals(), firstPtr);
            ASSERT_TRUE(mScriptManager.run(mScriptId, context));
            runFrame();
            EXPECT_EQ(runCount(first.mData), 2);
            EXPECT_EQ(runCount(second.mData), 3);
            EXPECT_FALSE(mLocalScripts.isRunning(mScriptId, firstPtr));

            first.mData.setLocalScriptEnabled(true);
            first.mData.setLocalScriptEnabled(true);
            runFrame();
            EXPECT_EQ(runCount(first.mData), 3);
            EXPECT_TRUE(mLocalScripts.isRunning(mScriptId, firstPtr));
        }

        TEST_F(MWWorldLocalScriptsTest, shouldKeepPauseWhenReregisteredAndNotScheduleInactiveObjectsOnResume)
        {
            auto original = makeReference();
            const Ptr originalPtr(&original);
            mLocalScripts.add(mScriptId, originalPtr);
            runFrame();
            original.mData.setLocalScriptEnabled(false);
            mLocalScripts.remove(originalPtr);

            auto moved = original;
            const Ptr movedPtr(&moved);
            mLocalScripts.add(mScriptId, movedPtr);
            runFrame();
            EXPECT_FALSE(mLocalScripts.isRunning(mScriptId, movedPtr));
            EXPECT_EQ(runCount(moved.mData), 1);

            mLocalScripts.remove(movedPtr);
            moved.mData.setLocalScriptEnabled(true);
            runFrame();
            EXPECT_FALSE(mLocalScripts.isRunning(mScriptId, movedPtr));
            EXPECT_EQ(runCount(moved.mData), 1);

            mLocalScripts.add(mScriptId, movedPtr);
            runFrame();
            EXPECT_TRUE(mLocalScripts.isRunning(mScriptId, movedPtr));
            EXPECT_EQ(runCount(moved.mData), 2);
        }

        TEST_F(MWWorldLocalScriptsTest, shouldPreserveActivationInterceptionWhilePaused)
        {
            auto item = makeReference();
            EXPECT_FALSE(item.mData.onActivate());
            item.mData.setLocalScriptEnabled(false);

            EXPECT_FALSE(item.mData.activate());
            EXPECT_FALSE(item.mData.activate());
            EXPECT_TRUE(item.mData.onActivate());
            EXPECT_FALSE(item.mData.onActivate());
            EXPECT_TRUE(item.mData.activateByScript());
            EXPECT_TRUE(item.mData.activate());
            EXPECT_FALSE(item.mData.isLocalScriptEnabled());

            EXPECT_FALSE(item.mData.onActivate());
            item.mData.setLocalScriptEnabled(true);
            EXPECT_FALSE(item.mData.activate());
            EXPECT_TRUE(item.mData.onActivate());
        }

        TEST_F(MWWorldLocalScriptsTest, shouldKeepPauseButResetActivationWhenCopied)
        {
            auto original = makeReference();
            EXPECT_FALSE(original.mData.onActivate());
            original.mData.setLocalScriptEnabled(false);
            EXPECT_FALSE(original.mData.activate());

            auto copied = original;
            EXPECT_FALSE(copied.mData.isLocalScriptEnabled());
            EXPECT_TRUE(copied.mData.activate());
            EXPECT_FALSE(copied.mData.onActivate());
            EXPECT_FALSE(copied.mData.activateByScript());
            EXPECT_TRUE(original.mData.onActivate());
        }

        TEST_F(MWWorldLocalScriptsTest, shouldSavePausedStateAndLocalVariablesForWorldAndInventoryObjects)
        {
            auto item = makeReference();
            mLocalScripts.add(mScriptId, Ptr(&item));
            runFrame();
            item.mData.setChanged(false);
            item.mData.setLocalScriptEnabled(false);
            ASSERT_TRUE(item.mData.hasChanged());
            EXPECT_FALSE(item.mData.onActivate());
            EXPECT_FALSE(item.mData.activate());

            for (bool inInventory : { false, true })
            {
                SCOPED_TRACE(inInventory);
                ESM::ObjectState state;
                state.blank();
                state.mRef.mRefID = mRecordId;
                item.mData.write(state, mScriptId);

                auto stream = std::make_unique<std::stringstream>();
                ESM::ESMWriter writer;
                writer.setFormatVersion(ESM::CurrentSaveGameFormatVersion);
                writer.save(*stream);
                writer.startRecord(ESM::REC_MISC);
                state.save(writer, inInventory);
                writer.endRecord(ESM::REC_MISC);

                ESM::ESMReader reader;
                reader.open(std::move(stream), "local-script-state");
                ASSERT_EQ(reader.getRecName().toInt(), ESM::REC_MISC);
                reader.getRecHeader();
                ESM::ObjectState loaded;
                loaded.mRef.loadId(reader, true);
                loaded.load(reader);

                RefData restored(loaded, false);
                restored.setLocals(*mStore.get<ESM::Script>().find(mScriptId));
                restored.getLocals().read(loaded.mLocals, mScriptId);
                EXPECT_FALSE(restored.isLocalScriptEnabled());
                EXPECT_TRUE(restored.activate());
                EXPECT_EQ(runCount(restored), 1);
                restored.setLocalScriptEnabled(true);
                EXPECT_TRUE(restored.isLocalScriptEnabled());
                EXPECT_EQ(runCount(restored), 1);
            }
        }
    }
}
