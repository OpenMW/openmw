#include "apps/openmw/mwbase/environment.hpp"
#include "apps/openmw/mwclass/esm4base.hpp"
#include "apps/openmw/mwclass/npc.hpp"
#include "apps/openmw/mwworld/cell.hpp"
#include "apps/openmw/mwworld/cellstore.hpp"
#include "apps/openmw/mwworld/esmstore.hpp"
#include "apps/openmw/mwworld/livecellref.hpp"
#include "apps/openmw/mwworld/ptr.hpp"
#include "apps/openmw/mwworld/worldmodel.hpp"

#include <components/esm3/loadnpc.hpp>
#include <components/esm3/readerscache.hpp>
#include <components/esm4/loadcell.hpp>
#include <components/esm4/loadrefr.hpp>
#include <components/esm4/loadstat.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace MWWorld
{
    namespace
    {
        using namespace testing;

        TEST(MWWorldPtrTest, toStringShouldReturnHumanReadableTextRepresentationOfPtrWithNullRef)
        {
            Ptr ptr;
            EXPECT_EQ(ptr.toString(), "null object");
        }

        TEST(MWWorldPtrTest, toStringShouldReturnHumanReadableTextRepresentationOfPtrWithDeletedRef)
        {
            MWClass::Npc::registerSelf();
            ESM::NPC npc;
            npc.blank();
            npc.mId = ESM::RefId::stringRefId("Player");
            ESMStore store;
            store.insert(npc);
            ESM::CellRef cellRef;
            cellRef.blank();
            cellRef.mRefID = npc.mId;
            cellRef.mRefNum = ESM::RefNum{ .mIndex = 0x2a, .mContentFile = 0xd };
            LiveCellRef<ESM::NPC> liveCellRef(cellRef, &npc);
            liveCellRef.mData.setDeletedByContentFile(true);
            Ptr ptr(&liveCellRef);
            EXPECT_THAT(ptr.toString(), StrCaseEq("deleted object0xd00002a (NPC, \"player\")"));
        }

        TEST(MWWorldPtrTest, toStringShouldReturnHumanReadableTextRepresentationOfPtr)
        {
            MWClass::Npc::registerSelf();
            ESM::NPC npc;
            npc.blank();
            npc.mId = ESM::RefId::stringRefId("Player");
            ESMStore store;
            store.insert(npc);
            ESM::CellRef cellRef;
            cellRef.blank();
            cellRef.mRefID = npc.mId;
            cellRef.mRefNum = ESM::RefNum{ .mIndex = 0x2a, .mContentFile = 0xd };
            LiveCellRef<ESM::NPC> liveCellRef(cellRef, &npc);
            Ptr ptr(&liveCellRef);
            EXPECT_THAT(ptr.toString(), StrCaseEq("object0xd00002a (NPC, \"player\")"));
        }

        TEST(MWWorldPtrTest, underlyingLiveCellRefShouldBeDeregisteredOnDestruction)
        {
            MWClass::Npc::registerSelf();
            ESM::NPC npc;
            npc.blank();
            npc.mId = ESM::RefId::stringRefId("Player");
            ESMStore store;
            store.insert(npc);
            ESM::ReadersCache readersCache;
            WorldModel worldModel(store, readersCache);
            ESM::CellRef cellRef;
            cellRef.blank();
            cellRef.mRefID = npc.mId;
            cellRef.mRefNum = ESM::FormId{ .mIndex = 0x2a, .mContentFile = 0xd };
            {
                LiveCellRef<ESM::NPC> liveCellRef(cellRef, &npc);
                Ptr ptr(&liveCellRef);
                worldModel.registerPtr(ptr);
                ASSERT_EQ(worldModel.getPtr(cellRef.mRefNum), ptr);
            }
            EXPECT_EQ(worldModel.getPtr(cellRef.mRefNum), Ptr());
        }

        // Regression test for #9224: refs that never reach the scene must still be resolvable by FormId
        TEST(MWWorldPtrTest, esm4RefDisabledAtLoadShouldBeResolvableByFormId)
        {
            MWClass::ESM4Static::registerSelf();

            ESMStore store;

            ESM4::Static static4{};
            static4.mId = ESM::FormId{ .mIndex = 0x1, .mContentFile = 0 };
            store.getWritable<ESM4::Static>().insertStatic(static4);

            ESM4::Cell cell4{};
            cell4.mId = ESM::FormId{ .mIndex = 0x100, .mContentFile = 0 };
            cell4.mCellFlags = ESM4::CELL_Interior;
            store.getWritable<ESM4::Cell>().insertStatic(cell4);

            const ESM::FormId parentFormId{ .mIndex = 0x2a, .mContentFile = 0 };
            const ESM::FormId childFormId{ .mIndex = 0x2b, .mContentFile = 0 };
            const ESM::FormId deletedFormId{ .mIndex = 0x2c, .mContentFile = 0 };

            // Enable-parent that is itself disabled: refs linked to it load disabled
            ESM4::Reference parentRef{};
            parentRef.mId = parentFormId;
            parentRef.mParent = cell4.mId;
            parentRef.mBaseObj = static4.mId;
            parentRef.mFlags = ESM4::Rec_Disabled;
            store.getWritable<ESM4::Reference>().insertStatic(parentRef);

            ESM4::Reference childRef{};
            childRef.mId = childFormId;
            childRef.mParent = cell4.mId;
            childRef.mBaseObj = static4.mId;
            childRef.mEsp.parent = parentFormId;
            store.getWritable<ESM4::Reference>().insertStatic(childRef);

            ESM4::Reference deletedRef{};
            deletedRef.mId = deletedFormId;
            deletedRef.mParent = cell4.mId;
            deletedRef.mBaseObj = static4.mId;
            deletedRef.mFlags = ESM4::Rec_Deleted;
            store.getWritable<ESM4::Reference>().insertStatic(deletedRef);

            store.rebuildIdsIndex();
            store.getWritable<ESM4::Reference>().preprocessReferences(store.get<ESM4::Cell>());

            ESM::ReadersCache readersCache;
            WorldModel worldModel(store, readersCache);
            MWBase::Environment environment;
            environment.setWorldModel(worldModel);
            environment.setESMStore(store);

            CellStore cellStore(Cell(cell4), store, readersCache);
            cellStore.load();

            Ptr childPtr = worldModel.getPtr(childFormId);
            ASSERT_FALSE(childPtr.isEmpty());
            EXPECT_FALSE(childPtr.getRefData().isEnabled());

            EXPECT_FALSE(worldModel.getPtr(parentFormId).isEmpty());

            // Content-deleted refs stay unregistered
            EXPECT_TRUE(worldModel.getPtr(deletedFormId).isEmpty());

            childPtr.getRefData().enable();
            EXPECT_TRUE(childPtr.getRefData().isEnabled());
            EXPECT_EQ(worldModel.getPtr(childFormId), childPtr);
        }
    }
}
