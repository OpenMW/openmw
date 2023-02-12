#include <components/esm/fourcc.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm3/player.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <random>

namespace ESM
{
    namespace
    {
        using namespace ::testing;

        constexpr std::array formats = {
            MaxLimitedSizeStringsFormatVersion,
            CurrentSaveGameFormatVersion,
        };

        constexpr std::uint32_t fakeRecordId = fourCC("FAKE");

        template <class T>
        void save(const T& record, ESMWriter& writer)
        {
            record.save(writer);
        }

        void save(const CellRef& record, ESMWriter& writer)
        {
            record.save(writer, true);
        }

        template <typename T>
        std::unique_ptr<std::istream> makeEsmStream(const T& record, FormatVersion formatVersion)
        {
            ESMWriter writer;
            auto stream = std::make_unique<std::stringstream>();
            writer.setFormatVersion(formatVersion);
            writer.save(*stream);
            writer.startRecord(fakeRecordId);
            save(record, writer);
            writer.endRecord(fakeRecordId);
            return stream;
        }

        template <class T>
        void load(ESMReader& reader, T& record)
        {
            record.load(reader);
        }

        void load(ESMReader& reader, CellRef& record)
        {
            bool deleted = false;
            record.load(reader, deleted, true);
        }

        template <typename T>
        void saveAndLoadRecord(const T& record, FormatVersion formatVersion, T& result)
        {
            ESMReader reader;
            reader.open(makeEsmStream(record, formatVersion), "stream");
            ASSERT_TRUE(reader.hasMoreRecs());
            ASSERT_EQ(reader.getRecName().toInt(), fakeRecordId);
            reader.getRecHeader();
            load(reader, result);
        }

        struct Esm3SaveLoadRecordTest : public TestWithParam<FormatVersion>
        {
            std::minstd_rand mRandom;
            std::uniform_int_distribution<short> mRefIdDistribution{ 'a', 'z' };

            std::string generateRandomString(std::size_t size)
            {
                std::string value;
                while (value.size() < size)
                    value.push_back(static_cast<char>(mRefIdDistribution(mRandom)));
                return value;
            }

            RefId generateRandomRefId(std::size_t size = 33) { return RefId::stringRefId(generateRandomString(size)); }
        };

        TEST_F(Esm3SaveLoadRecordTest, headerShouldNotChange)
        {
            const std::string author = generateRandomString(33);
            const std::string description = generateRandomString(257);

            auto stream = std::make_unique<std::stringstream>();

            ESMWriter writer;
            writer.setAuthor(author);
            writer.setDescription(description);
            writer.setFormatVersion(CurrentSaveGameFormatVersion);
            writer.save(*stream);
            writer.close();

            ESMReader reader;
            reader.open(std::move(stream), "stream");
            EXPECT_EQ(reader.getAuthor(), author);
            EXPECT_EQ(reader.getDesc(), description);
        }

        TEST_P(Esm3SaveLoadRecordTest, playerShouldNotChange)
        {
            std::minstd_rand random;
            Player record{};
            record.mObject.blank();
            record.mBirthsign = generateRandomRefId();
            record.mObject.mRef.mRefID = generateRandomRefId();
            std::generate_n(std::inserter(record.mPreviousItems, record.mPreviousItems.end()), 2,
                [&] { return std::make_pair(generateRandomRefId(), generateRandomRefId()); });
            Player result;
            saveAndLoadRecord(record, GetParam(), result);
            EXPECT_EQ(record.mBirthsign, result.mBirthsign);
            EXPECT_EQ(record.mPreviousItems, result.mPreviousItems);
        }

        TEST_P(Esm3SaveLoadRecordTest, cellRefShouldNotChange)
        {
            CellRef record;
            record.blank();
            record.mRefID = generateRandomRefId();
            record.mOwner = generateRandomRefId();
            record.mSoul = generateRandomRefId();
            record.mFaction = generateRandomRefId();
            record.mKey = generateRandomRefId();
            CellRef result;
            saveAndLoadRecord(record, GetParam(), result);
            EXPECT_EQ(record.mRefID, result.mRefID);
            EXPECT_EQ(record.mOwner, result.mOwner);
            EXPECT_EQ(record.mSoul, result.mSoul);
            EXPECT_EQ(record.mFaction, result.mFaction);
            EXPECT_EQ(record.mKey, result.mKey);
        }

        TEST_P(Esm3SaveLoadRecordTest, creatureStatsShouldNotChange)
        {
            CreatureStats record;
            record.blank();
            record.mLastHitAttemptObject = generateRandomRefId();
            record.mLastHitObject = generateRandomRefId();
            CreatureStats result;
            saveAndLoadRecord(record, GetParam(), result);
            EXPECT_EQ(record.mLastHitAttemptObject, result.mLastHitAttemptObject);
            EXPECT_EQ(record.mLastHitObject, result.mLastHitObject);
        }

        INSTANTIATE_TEST_SUITE_P(FormatVersions, Esm3SaveLoadRecordTest, ValuesIn(formats));
    }
}
