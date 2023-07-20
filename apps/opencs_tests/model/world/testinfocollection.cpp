#include "apps/opencs/model/world/infocollection.hpp"

#include "components/esm3/esmreader.hpp"
#include "components/esm3/esmwriter.hpp"
#include "components/esm3/formatversion.hpp"
#include "components/esm3/loaddial.hpp"
#include "components/esm3/loadinfo.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <sstream>
#include <vector>

namespace CSMWorld
{
    inline std::ostream& operator<<(std::ostream& stream, const Record<Info>* value)
    {
        return stream << "&Record{.mState=" << value->mState << ", .mId=" << value->get().mId << "}";
    }

    namespace
    {
        using namespace ::testing;

        struct DialInfoData
        {
            ESM::DialInfo mValue;
            bool mDeleted = false;

            void save(ESM::ESMWriter& writer) const { mValue.save(writer, mDeleted); }
        };

        template <class T>
        struct DialogueData
        {
            ESM::Dialogue mDialogue;
            std::vector<T> mInfos;
        };

        DialogueData<ESM::DialInfo> generateDialogueWithInfos(
            std::size_t infoCount, std::string_view dialogueId = "dialogue")
        {
            DialogueData<ESM::DialInfo> result;

            result.mDialogue.blank();
            result.mDialogue.mId = ESM::RefId::stringRefId(dialogueId);
            result.mDialogue.mStringId = dialogueId;

            for (std::size_t i = 0; i < infoCount; ++i)
            {
                ESM::DialInfo& info = result.mInfos.emplace_back();
                info.blank();
                info.mId = ESM::RefId::stringRefId("info" + std::to_string(i));
            }

            if (infoCount >= 2)
            {
                result.mInfos[0].mNext = result.mInfos[1].mId;
                result.mInfos[infoCount - 1].mPrev = result.mInfos[infoCount - 2].mId;
            }

            for (std::size_t i = 1; i < infoCount - 1; ++i)
            {
                result.mInfos[i].mPrev = result.mInfos[i - 1].mId;
                result.mInfos[i].mNext = result.mInfos[i + 1].mId;
            }

            return result;
        }

        template <class Infos>
        std::unique_ptr<std::stringstream> saveDialogueWithInfos(const ESM::Dialogue& dialogue, Infos&& infos)
        {
            auto stream = std::make_unique<std::stringstream>();

            ESM::ESMWriter writer;
            writer.setFormatVersion(ESM::CurrentSaveGameFormatVersion);
            writer.save(*stream);

            writer.startRecord(ESM::REC_DIAL);
            dialogue.save(writer);
            writer.endRecord(ESM::REC_DIAL);

            for (const auto& info : infos)
            {
                writer.startRecord(ESM::REC_INFO);
                info.save(writer);
                writer.endRecord(ESM::REC_INFO);
            }

            return stream;
        }

        void loadDialogueWithInfos(bool base, std::unique_ptr<std::stringstream> stream, InfoCollection& infoCollection,
            InfoOrderByTopic& infoOrder)
        {
            ESM::ESMReader reader;
            reader.open(std::move(stream), "test");

            ASSERT_TRUE(reader.hasMoreRecs());
            ASSERT_EQ(reader.getRecName().toInt(), ESM::REC_DIAL);
            reader.getRecHeader();
            bool isDeleted;
            ESM::Dialogue dialogue;
            dialogue.load(reader, isDeleted);

            while (reader.hasMoreRecs())
            {
                ASSERT_EQ(reader.getRecName().toInt(), ESM::REC_INFO);
                reader.getRecHeader();
                infoCollection.load(reader, base, dialogue, infoOrder);
            }
        }

        template <class Infos>
        void saveAndLoadDialogueWithInfos(const ESM::Dialogue& dialogue, Infos&& infos, bool base,
            InfoCollection& infoCollection, InfoOrderByTopic& infoOrder)
        {
            loadDialogueWithInfos(base, saveDialogueWithInfos(dialogue, infos), infoCollection, infoOrder);
        }

        template <class T>
        void saveAndLoadDialogueWithInfos(
            const DialogueData<T>& data, bool base, InfoCollection& infoCollection, InfoOrderByTopic& infoOrder)
        {
            saveAndLoadDialogueWithInfos(data.mDialogue, data.mInfos, base, infoCollection, infoOrder);
        }

        MATCHER_P(InfoId, v, "")
        {
            return arg.mId == v;
        }

        TEST(CSMWorldInfoCollectionTest, loadShouldAddRecord)
        {
            ESM::Dialogue dialogue;
            dialogue.blank();
            dialogue.mId = ESM::RefId::stringRefId("dialogue");
            dialogue.mStringId = "Dialogue";

            ESM::DialInfo info;
            info.blank();
            info.mId = ESM::RefId::stringRefId("info0");

            const bool base = true;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;
            saveAndLoadDialogueWithInfos(dialogue, std::array{ info }, base, collection, infoOrder);

            EXPECT_EQ(collection.getSize(), 1);
            ASSERT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info0")), 0);
            const Record<Info>& record = collection.getRecord(0);
            ASSERT_EQ(record.mState, RecordBase::State_BaseOnly);
            EXPECT_EQ(record.mBase.mTopicId, dialogue.mId);
            EXPECT_EQ(record.mBase.mOriginalId, info.mId);
            EXPECT_EQ(record.mBase.mId, ESM::RefId::stringRefId("dialogue#info0"));

            ASSERT_THAT(infoOrder, ElementsAre(Key(dialogue.mId)));
            EXPECT_THAT(infoOrder.find(dialogue.mId)->second.getOrderedInfo(), ElementsAre(InfoId(info.mId)));
        }

        TEST(CSMWorldInfoCollectionTest, loadShouldAddRecordAndMarkModifiedOnlyWhenNotBase)
        {
            ESM::Dialogue dialogue;
            dialogue.blank();
            dialogue.mId = ESM::RefId::stringRefId("dialogue");
            dialogue.mStringId = "Dialogue";

            ESM::DialInfo info;
            info.blank();
            info.mId = ESM::RefId::stringRefId("info0");

            const bool base = false;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;
            saveAndLoadDialogueWithInfos(dialogue, std::array{ info }, base, collection, infoOrder);

            EXPECT_EQ(collection.getSize(), 1);
            ASSERT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info0")), 0);
            const Record<Info>& record = collection.getRecord(0);
            ASSERT_EQ(record.mState, RecordBase::State_ModifiedOnly);
            EXPECT_EQ(record.mModified.mTopicId, dialogue.mId);
            EXPECT_EQ(record.mModified.mOriginalId, info.mId);
            EXPECT_EQ(record.mModified.mId, ESM::RefId::stringRefId("dialogue#info0"));

            ASSERT_THAT(infoOrder, ElementsAre(Key(dialogue.mId)));
            EXPECT_THAT(infoOrder.find(dialogue.mId)->second.getOrderedInfo(), ElementsAre(InfoId(info.mId)));
        }

        TEST(CSMWorldInfoCollectionTest, loadShouldUpdateRecord)
        {
            ESM::Dialogue dialogue;
            dialogue.blank();
            dialogue.mId = ESM::RefId::stringRefId("dialogue");
            dialogue.mStringId = "Dialogue";

            ESM::DialInfo info;
            info.blank();
            info.mId = ESM::RefId::stringRefId("info0");

            const bool base = true;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;
            saveAndLoadDialogueWithInfos(dialogue, std::array{ info }, base, collection, infoOrder);

            ESM::DialInfo updatedInfo = info;
            updatedInfo.mActor = ESM::RefId::stringRefId("newActor");

            saveAndLoadDialogueWithInfos(dialogue, std::array{ updatedInfo }, base, collection, infoOrder);

            ASSERT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info0")), 0);
            const Record<Info>& record = collection.getRecord(0);
            ASSERT_EQ(record.mState, RecordBase::State_BaseOnly);
            EXPECT_EQ(record.mBase.mActor, ESM::RefId::stringRefId("newActor"));

            ASSERT_THAT(infoOrder, ElementsAre(Key(dialogue.mId)));
            EXPECT_THAT(infoOrder.find(dialogue.mId)->second.getOrderedInfo(), ElementsAre(InfoId(info.mId)));
        }

        TEST(CSMWorldInfoCollectionTest, loadShouldUpdateRecordAndMarkModifiedWhenNotBase)
        {
            ESM::Dialogue dialogue;
            dialogue.blank();
            dialogue.mId = ESM::RefId::stringRefId("dialogue");
            dialogue.mStringId = "Dialogue";

            ESM::DialInfo info;
            info.blank();
            info.mId = ESM::RefId::stringRefId("info0");

            const bool base = true;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;
            saveAndLoadDialogueWithInfos(dialogue, std::array{ info }, base, collection, infoOrder);

            ESM::DialInfo updatedInfo = info;
            updatedInfo.mActor = ESM::RefId::stringRefId("newActor");

            saveAndLoadDialogueWithInfos(dialogue, std::array{ updatedInfo }, false, collection, infoOrder);

            ASSERT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info0")), 0);
            const Record<Info>& record = collection.getRecord(0);
            ASSERT_EQ(record.mState, RecordBase::State_Modified);
            EXPECT_EQ(record.mModified.mActor, ESM::RefId::stringRefId("newActor"));

            ASSERT_THAT(infoOrder, ElementsAre(Key(dialogue.mId)));
            EXPECT_THAT(infoOrder.find(dialogue.mId)->second.getOrderedInfo(), ElementsAre(InfoId(info.mId)));
        }

        TEST(CSMWorldInfoCollectionTest, loadShouldSkipAbsentDeletedRecord)
        {
            ESM::Dialogue dialogue;
            dialogue.blank();
            dialogue.mId = ESM::RefId::stringRefId("dialogue");
            dialogue.mStringId = "Dialogue";

            DialInfoData info;
            info.mValue.blank();
            info.mValue.mId = ESM::RefId::stringRefId("info0");
            info.mDeleted = true;

            const bool base = true;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;
            saveAndLoadDialogueWithInfos(dialogue, std::array{ info }, base, collection, infoOrder);

            EXPECT_EQ(collection.getSize(), 0);

            ASSERT_THAT(infoOrder, ElementsAre());
        }

        TEST(CSMWorldInfoCollectionTest, loadShouldRemovePresentDeletedBaseRecord)
        {
            ESM::Dialogue dialogue;
            dialogue.blank();
            dialogue.mId = ESM::RefId::stringRefId("dialogue");
            dialogue.mStringId = "Dialogue";

            DialInfoData info;
            info.mValue.blank();
            info.mValue.mId = ESM::RefId::stringRefId("info0");

            const bool base = true;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;

            saveAndLoadDialogueWithInfos(dialogue, std::array{ info }, base, collection, infoOrder);

            info.mDeleted = true;

            saveAndLoadDialogueWithInfos(dialogue, std::array{ info }, base, collection, infoOrder);

            EXPECT_EQ(collection.getSize(), 0);

            ASSERT_THAT(infoOrder, ElementsAre(Key(dialogue.mId)));
            EXPECT_THAT(infoOrder.find(dialogue.mId)->second.getOrderedInfo(), ElementsAre());
        }

        TEST(CSMWorldInfoCollectionTest, loadShouldMarkAsDeletedNotBaseRecord)
        {
            ESM::Dialogue dialogue;
            dialogue.blank();
            dialogue.mId = ESM::RefId::stringRefId("dialogue");
            dialogue.mStringId = "Dialogue";

            DialInfoData info;
            info.mValue.blank();
            info.mValue.mId = ESM::RefId::stringRefId("info0");

            const bool base = true;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;

            saveAndLoadDialogueWithInfos(dialogue, std::array{ info }, base, collection, infoOrder);

            info.mDeleted = true;

            saveAndLoadDialogueWithInfos(dialogue, std::array{ info }, false, collection, infoOrder);

            EXPECT_EQ(collection.getSize(), 1);
            EXPECT_EQ(
                collection.getRecord(ESM::RefId::stringRefId("dialogue#info0")).mState, RecordBase::State_Deleted);

            ASSERT_THAT(infoOrder, ElementsAre(Key(dialogue.mId)));
            EXPECT_THAT(infoOrder.find(dialogue.mId)->second.getOrderedInfo(), ElementsAre(InfoId(info.mValue.mId)));
        }

        TEST(CSMWorldInfoCollectionTest, sortShouldOrderRecordsBasedOnPrev)
        {
            const DialogueData<ESM::DialInfo> data = generateDialogueWithInfos(3);

            const bool base = true;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;
            saveAndLoadDialogueWithInfos(data, base, collection, infoOrder);

            collection.sort(infoOrder);

            EXPECT_EQ(collection.getSize(), 3);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info0")), 0);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info1")), 1);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info2")), 2);
        }

        TEST(CSMWorldInfoCollectionTest, sortShouldOrderRecordsBasedOnPrevWhenReversed)
        {
            DialogueData<ESM::DialInfo> data = generateDialogueWithInfos(3);

            std::reverse(data.mInfos.begin(), data.mInfos.end());

            const bool base = true;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;
            saveAndLoadDialogueWithInfos(data, base, collection, infoOrder);

            collection.sort(infoOrder);

            EXPECT_EQ(collection.getSize(), 3);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info0")), 0);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info2")), 1);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info1")), 2);
        }

        TEST(CSMWorldInfoCollectionTest, sortShouldInsertNewRecordBasedOnPrev)
        {
            const bool base = true;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;

            const DialogueData<ESM::DialInfo> data = generateDialogueWithInfos(3);

            saveAndLoadDialogueWithInfos(data, base, collection, infoOrder);

            ESM::DialInfo newInfo;
            newInfo.blank();
            newInfo.mId = ESM::RefId::stringRefId("newInfo");
            newInfo.mPrev = data.mInfos[1].mId;
            newInfo.mNext = ESM::RefId::stringRefId("invalid");

            saveAndLoadDialogueWithInfos(data.mDialogue, std::array{ newInfo }, base, collection, infoOrder);

            collection.sort(infoOrder);

            EXPECT_EQ(collection.getSize(), 4);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info0")), 0);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info1")), 1);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#newInfo")), 2);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info2")), 3);
        }

        TEST(CSMWorldInfoCollectionTest, sortShouldInsertNewRecordToFrontWhenPrevIsEmpty)
        {
            const bool base = true;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;

            const DialogueData<ESM::DialInfo> data = generateDialogueWithInfos(3);

            saveAndLoadDialogueWithInfos(data, base, collection, infoOrder);

            ESM::DialInfo newInfo;
            newInfo.blank();
            newInfo.mId = ESM::RefId::stringRefId("newInfo");
            newInfo.mNext = ESM::RefId::stringRefId("invalid");

            saveAndLoadDialogueWithInfos(data.mDialogue, std::array{ newInfo }, base, collection, infoOrder);

            collection.sort(infoOrder);

            EXPECT_EQ(collection.getSize(), 4);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#newInfo")), 0);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info0")), 1);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info1")), 2);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info2")), 3);
        }

        TEST(CSMWorldInfoCollectionTest, sortShouldInsertNewRecordToBackWhenPrevIsNotFound)
        {
            const bool base = true;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;

            const DialogueData<ESM::DialInfo> data = generateDialogueWithInfos(3);

            saveAndLoadDialogueWithInfos(data, base, collection, infoOrder);

            ESM::DialInfo newInfo;
            newInfo.blank();
            newInfo.mId = ESM::RefId::stringRefId("newInfo");
            newInfo.mPrev = ESM::RefId::stringRefId("invalid");

            saveAndLoadDialogueWithInfos(data.mDialogue, std::array{ newInfo }, base, collection, infoOrder);

            collection.sort(infoOrder);

            EXPECT_EQ(collection.getSize(), 4);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info0")), 0);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info1")), 1);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info2")), 2);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#newInfo")), 3);
        }

        TEST(CSMWorldInfoCollectionTest, sortShouldMoveBackwardUpdatedRecordBasedOnPrev)
        {
            const bool base = true;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;

            const DialogueData<ESM::DialInfo> data = generateDialogueWithInfos(3);

            saveAndLoadDialogueWithInfos(data, base, collection, infoOrder);

            ESM::DialInfo updatedInfo = data.mInfos[2];
            updatedInfo.mPrev = data.mInfos[0].mId;
            updatedInfo.mNext = ESM::RefId::stringRefId("invalid");

            saveAndLoadDialogueWithInfos(data.mDialogue, std::array{ updatedInfo }, base, collection, infoOrder);

            collection.sort(infoOrder);

            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info0")), 0);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info1")), 2);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info2")), 1);
        }

        TEST(CSMWorldInfoCollectionTest, sortShouldMoveForwardUpdatedRecordBasedOnPrev)
        {
            const bool base = true;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;

            const DialogueData<ESM::DialInfo> data = generateDialogueWithInfos(3);

            saveAndLoadDialogueWithInfos(data, base, collection, infoOrder);

            ESM::DialInfo updatedInfo = data.mInfos[0];
            updatedInfo.mPrev = data.mInfos[1].mId;
            updatedInfo.mNext = ESM::RefId::stringRefId("invalid");

            saveAndLoadDialogueWithInfos(data.mDialogue, std::array{ updatedInfo }, base, collection, infoOrder);

            collection.sort(infoOrder);

            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info0")), 1);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info1")), 0);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info2")), 2);
        }

        TEST(CSMWorldInfoCollectionTest, sortShouldMoveToFrontUpdatedRecordWhenPrevIsEmpty)
        {
            const bool base = true;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;

            const DialogueData<ESM::DialInfo> data = generateDialogueWithInfos(3);

            saveAndLoadDialogueWithInfos(data, base, collection, infoOrder);

            ESM::DialInfo updatedInfo = data.mInfos[2];
            updatedInfo.mPrev = ESM::RefId();
            updatedInfo.mNext = ESM::RefId::stringRefId("invalid");

            saveAndLoadDialogueWithInfos(data.mDialogue, std::array{ updatedInfo }, base, collection, infoOrder);

            collection.sort(infoOrder);

            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info0")), 1);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info1")), 2);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info2")), 0);
        }

        TEST(CSMWorldInfoCollectionTest, sortShouldMoveToBackUpdatedRecordWhenPrevIsNotFound)
        {
            const bool base = true;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;

            const DialogueData<ESM::DialInfo> data = generateDialogueWithInfos(3);

            saveAndLoadDialogueWithInfos(data, base, collection, infoOrder);

            ESM::DialInfo updatedInfo = data.mInfos[0];
            updatedInfo.mPrev = ESM::RefId::stringRefId("invalid");
            updatedInfo.mNext = ESM::RefId();

            saveAndLoadDialogueWithInfos(data.mDialogue, std::array{ updatedInfo }, base, collection, infoOrder);

            collection.sort(infoOrder);

            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info0")), 2);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info1")), 0);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue#info2")), 1);
        }

        TEST(CSMWorldInfoCollectionTest, sortShouldProvideStableOrderByTopic)
        {
            const bool base = true;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;

            saveAndLoadDialogueWithInfos(generateDialogueWithInfos(2, "dialogue2"), base, collection, infoOrder);
            saveAndLoadDialogueWithInfos(generateDialogueWithInfos(2, "dialogue0"), base, collection, infoOrder);
            saveAndLoadDialogueWithInfos(generateDialogueWithInfos(2, "dialogue1"), base, collection, infoOrder);

            collection.sort(infoOrder);

            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue0#info0")), 0);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue0#info1")), 1);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue1#info0")), 2);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue1#info1")), 3);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue2#info0")), 4);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue2#info1")), 5);
        }

        TEST(CSMWorldInfoCollectionTest, getAppendIndexShouldReturnFirstIndexAfterInfoTopic)
        {
            const bool base = true;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;

            saveAndLoadDialogueWithInfos(generateDialogueWithInfos(2, "dialogue0"), base, collection, infoOrder);
            saveAndLoadDialogueWithInfos(generateDialogueWithInfos(2, "dialogue1"), base, collection, infoOrder);

            collection.sort(infoOrder);

            EXPECT_EQ(collection.getAppendIndex(ESM::RefId::stringRefId("dialogue0#info2")), 2);
        }

        TEST(CSMWorldInfoCollectionTest, reorderRowsShouldFailWhenOutOfBounds)
        {
            const bool base = true;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;

            saveAndLoadDialogueWithInfos(generateDialogueWithInfos(2, "dialogue0"), base, collection, infoOrder);
            saveAndLoadDialogueWithInfos(generateDialogueWithInfos(2, "dialogue1"), base, collection, infoOrder);

            EXPECT_FALSE(collection.reorderRows(5, {}));
        }

        TEST(CSMWorldInfoCollectionTest, reorderRowsShouldFailWhenAppliedToDifferentTopics)
        {
            const bool base = true;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;

            saveAndLoadDialogueWithInfos(generateDialogueWithInfos(2, "dialogue0"), base, collection, infoOrder);
            saveAndLoadDialogueWithInfos(generateDialogueWithInfos(2, "dialogue1"), base, collection, infoOrder);

            EXPECT_FALSE(collection.reorderRows(0, { 0, 1, 2 }));
        }

        TEST(CSMWorldInfoCollectionTest, reorderRowsShouldSucceedWhenAppliedToOneTopic)
        {
            const bool base = true;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;

            saveAndLoadDialogueWithInfos(generateDialogueWithInfos(3, "dialogue0"), base, collection, infoOrder);
            saveAndLoadDialogueWithInfos(generateDialogueWithInfos(3, "dialogue1"), base, collection, infoOrder);

            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue0#info0")), 0);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue0#info1")), 1);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue0#info2")), 2);

            EXPECT_TRUE(collection.reorderRows(1, { 1, 0 }));

            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue0#info0")), 0);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue0#info1")), 2);
            EXPECT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue0#info2")), 1);
        }

        MATCHER_P(RecordPtrIdIs, v, "")
        {
            return v == arg->get().mId;
        }

        TEST(CSMWorldInfoCollectionTest, getInfosByTopicShouldReturnRecordsGroupedByTopic)
        {
            const bool base = true;
            InfoOrderByTopic infoOrder;
            InfoCollection collection;

            saveAndLoadDialogueWithInfos(generateDialogueWithInfos(2, "d0"), base, collection, infoOrder);
            saveAndLoadDialogueWithInfos(generateDialogueWithInfos(2, "d1"), base, collection, infoOrder);

            collection.sort(infoOrder);

            EXPECT_THAT(collection.getInfosByTopic(),
                UnorderedElementsAre(Pair("d0", ElementsAre(RecordPtrIdIs("d0#info0"), RecordPtrIdIs("d0#info1"))),
                    Pair("d1", ElementsAre(RecordPtrIdIs("d1#info0"), RecordPtrIdIs("d1#info1")))));
        }
    }
}
