#include "apps/opencs/model/world/infocollection.hpp"

#include "components/esm3/esmreader.hpp"
#include "components/esm3/esmwriter.hpp"
#include "components/esm3/formatversion.hpp"
#include "components/esm3/loaddial.hpp"
#include "components/esm3/loadinfo.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <span>
#include <sstream>

namespace CSMWorld
{
    namespace
    {
        using namespace ::testing;

        std::unique_ptr<std::stringstream> saveDialogueWithInfos(
            const ESM::Dialogue& dialogue, std::span<const ESM::DialInfo> infos)
        {
            auto stream = std::make_unique<std::stringstream>();

            ESM::ESMWriter writer;
            writer.setFormatVersion(ESM::CurrentSaveGameFormatVersion);
            writer.save(*stream);

            writer.startRecord(ESM::REC_DIAL);
            dialogue.save(writer);
            writer.endRecord(ESM::REC_DIAL);

            for (const ESM::DialInfo& info : infos)
            {
                writer.startRecord(ESM::REC_INFO);
                info.save(writer);
                writer.endRecord(ESM::REC_INFO);
            }

            return stream;
        }

        void loadDialogueWithInfos(bool base, std::unique_ptr<std::stringstream> stream, InfoCollection& infoCollection,
            InfosByTopic& infosByTopic)
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
                infoCollection.load(reader, base, dialogue, infosByTopic);
            }
        }

        void saveAndLoadDialogueWithInfos(const ESM::Dialogue& dialogue, std::span<const ESM::DialInfo> infos,
            bool base, InfoCollection& infoCollection, InfosByTopic& infosByTopic)
        {
            loadDialogueWithInfos(base, saveDialogueWithInfos(dialogue, infos), infoCollection, infosByTopic);
        }

        TEST(CSMWorldInfoCollectionTest, loadShouldAddRecord)
        {
            ESM::Dialogue dialogue;
            dialogue.blank();
            dialogue.mId = ESM::RefId::stringRefId("dialogue1");

            ESM::DialInfo info;
            info.blank();
            info.mId = ESM::RefId::stringRefId("info1");

            const bool base = true;
            InfosByTopic infosByTopic;
            InfoCollection collection;
            saveAndLoadDialogueWithInfos(dialogue, std::array{ info }, base, collection, infosByTopic);

            EXPECT_EQ(collection.getSize(), 1);
            ASSERT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue1#info1")), 0);
            const Record<Info>& record = collection.getRecord(0);
            ASSERT_EQ(record.mState, RecordBase::State_BaseOnly);
            EXPECT_EQ(record.mBase.mTopicId, dialogue.mId);
            EXPECT_EQ(record.mBase.mOriginalId, info.mId);
            EXPECT_EQ(record.mBase.mId, ESM::RefId::stringRefId("dialogue1#info1"));
        }

        TEST(CSMWorldInfoCollectionTest, loadShouldUpdateRecord)
        {
            ESM::Dialogue dialogue;
            dialogue.blank();
            dialogue.mId = ESM::RefId::stringRefId("dialogue1");

            ESM::DialInfo info;
            info.blank();
            info.mId = ESM::RefId::stringRefId("info1");

            const bool base = true;
            InfosByTopic infosByTopic;
            InfoCollection collection;
            saveAndLoadDialogueWithInfos(dialogue, std::array{ info }, base, collection, infosByTopic);

            ESM::DialInfo updatedInfo = info;
            updatedInfo.mActor = ESM::RefId::stringRefId("newActor");

            saveAndLoadDialogueWithInfos(dialogue, std::array{ updatedInfo }, base, collection, infosByTopic);

            ASSERT_EQ(collection.searchId(ESM::RefId::stringRefId("dialogue1#info1")), 0);
            const Record<Info>& record = collection.getRecord(0);
            ASSERT_EQ(record.mState, RecordBase::State_BaseOnly);
            EXPECT_EQ(record.mBase.mActor, ESM::RefId::stringRefId("newActor"));
        }
    }
}
