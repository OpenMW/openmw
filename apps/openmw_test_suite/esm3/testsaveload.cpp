#include <components/esm/fourcc.hpp>
#include <components/esm3/aisequence.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm3/loadcont.hpp>
#include <components/esm3/loaddial.hpp>
#include <components/esm3/loadregn.hpp>
#include <components/esm3/loadscpt.hpp>
#include <components/esm3/player.hpp>
#include <components/esm3/quickkeys.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <random>
#include <type_traits>

namespace ESM
{
    namespace
    {
        auto tie(const ContItem& value)
        {
            return std::tie(value.mCount, value.mItem);
        }

        auto tie(const ESM::Region::SoundRef& value)
        {
            return std::tie(value.mSound, value.mChance);
        }

        auto tie(const ESM::QuickKeys::QuickKey& value)
        {
            return std::tie(value.mType, value.mId);
        }
    }

    inline bool operator==(const ESM::ContItem& lhs, const ESM::ContItem& rhs)
    {
        return tie(lhs) == tie(rhs);
    }

    inline std::ostream& operator<<(std::ostream& stream, const ESM::ContItem& value)
    {
        return stream << "ESM::ContItem {.mCount = " << value.mCount << ", .mItem = '" << value.mItem << "'}";
    }

    inline bool operator==(const ESM::Region::SoundRef& lhs, const ESM::Region::SoundRef& rhs)
    {
        return tie(lhs) == tie(rhs);
    }

    inline std::ostream& operator<<(std::ostream& stream, const ESM::Region::SoundRef& value)
    {
        return stream << "ESM::Region::SoundRef {.mSound = '" << value.mSound << "', .mChance = " << value.mChance
                      << "}";
    }

    inline bool operator==(const ESM::QuickKeys::QuickKey& lhs, const ESM::QuickKeys::QuickKey& rhs)
    {
        return tie(lhs) == tie(rhs);
    }

    inline std::ostream& operator<<(std::ostream& stream, const ESM::QuickKeys::QuickKey& value)
    {
        return stream << "ESM::QuickKeys::QuickKey {.mType = '" << static_cast<std::uint32_t>(value.mType)
                      << "', .mId = " << value.mId << "}";
    }

    namespace
    {
        using namespace ::testing;

        std::vector<ESM::FormatVersion> getFormats()
        {
            std::vector<ESM::FormatVersion> result({
                CurrentContentFormatVersion,
                MaxLimitedSizeStringsFormatVersion,
                MaxStringRefIdFormatVersion,
            });
            for (ESM::FormatVersion v = result.back() + 1; v <= ESM::CurrentSaveGameFormatVersion; ++v)
                result.push_back(v);
            return result;
        }

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

        template <class T, class = std::void_t<>>
        struct HasLoad : std::false_type
        {
        };

        template <class T>
        struct HasLoad<T, std::void_t<decltype(std::declval<T>().load(std::declval<ESMReader&>()))>> : std::true_type
        {
        };

        template <class T>
        auto load(ESMReader& reader, T& record) -> std::enable_if_t<HasLoad<std::decay_t<T>>::value>
        {
            record.load(reader);
        }

        template <class T, class = std::void_t<>>
        struct HasLoadWithDelete : std::false_type
        {
        };

        template <class T>
        struct HasLoadWithDelete<T,
            std::void_t<decltype(std::declval<T>().load(std::declval<ESMReader&>(), std::declval<bool&>()))>>
            : std::true_type
        {
        };

        template <class T>
        auto load(ESMReader& reader, T& record) -> std::enable_if_t<HasLoadWithDelete<std::decay_t<T>>::value>
        {
            bool deleted = false;
            record.load(reader, deleted);
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

            template <class T, std::size_t n>
            void generateArray(T (&dst)[n])
            {
                for (auto& v : dst)
                    v = std::uniform_real_distribution<float>{ -1.0f, 1.0f }(mRandom);
            }
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

        TEST_F(Esm3SaveLoadRecordTest, containerContItemShouldSupportRefIdLongerThan32)
        {
            Container record;
            record.blank();
            record.mInventory.mList.push_back(ESM::ContItem{ .mCount = 42, .mItem = generateRandomRefId(33) });
            record.mInventory.mList.push_back(ESM::ContItem{ .mCount = 13, .mItem = generateRandomRefId(33) });
            Container result;
            saveAndLoadRecord(record, CurrentSaveGameFormatVersion, result);
            EXPECT_EQ(result.mInventory.mList, record.mInventory.mList);
        }

        TEST_F(Esm3SaveLoadRecordTest, regionSoundRefShouldSupportRefIdLongerThan32)
        {
            Region record;
            record.blank();
            record.mSoundList.push_back(ESM::Region::SoundRef{ .mSound = generateRandomRefId(33), .mChance = 42 });
            record.mSoundList.push_back(ESM::Region::SoundRef{ .mSound = generateRandomRefId(33), .mChance = 13 });
            Region result;
            saveAndLoadRecord(record, CurrentSaveGameFormatVersion, result);
            EXPECT_EQ(result.mSoundList, record.mSoundList);
        }

        TEST_F(Esm3SaveLoadRecordTest, scriptSoundRefShouldSupportRefIdLongerThan32)
        {
            Script record;
            record.blank();
            record.mId = generateRandomRefId(33);
            record.mData.mNumShorts = 42;
            Script result;
            saveAndLoadRecord(record, CurrentSaveGameFormatVersion, result);
            EXPECT_EQ(result.mId, record.mId);
            EXPECT_EQ(result.mData.mNumShorts, record.mData.mNumShorts);
        }

        TEST_P(Esm3SaveLoadRecordTest, playerShouldNotChange)
        {
            // Player state is not saved to vanilla ESM format.
            if (GetParam() == CurrentContentFormatVersion)
                return;
            std::minstd_rand random;
            Player record{};
            record.mObject.blank();
            record.mBirthsign = generateRandomRefId();
            record.mObject.mRef.mRefID = generateRandomRefId();
            std::generate_n(std::inserter(record.mPreviousItems, record.mPreviousItems.end()), 2,
                [&] { return std::make_pair(generateRandomRefId(), generateRandomRefId()); });
            record.mCellId = ESM::RefId::esm3ExteriorCell(0, 0);
            generateArray(record.mLastKnownExteriorPosition);
            record.mHasMark = true;
            record.mMarkedCell = ESM::RefId::esm3ExteriorCell(0, 0);
            generateArray(record.mMarkedPosition.pos);
            generateArray(record.mMarkedPosition.rot);
            record.mCurrentCrimeId = 42;
            record.mPaidCrimeId = 13;
            Player result;
            saveAndLoadRecord(record, GetParam(), result);
            EXPECT_EQ(record.mObject.mRef.mRefID, result.mObject.mRef.mRefID);
            EXPECT_EQ(record.mBirthsign, result.mBirthsign);
            EXPECT_EQ(record.mPreviousItems, result.mPreviousItems);
            EXPECT_EQ(record.mPreviousItems, result.mPreviousItems);
            EXPECT_EQ(record.mCellId, result.mCellId);
            EXPECT_THAT(record.mLastKnownExteriorPosition, ElementsAreArray(result.mLastKnownExteriorPosition));
            EXPECT_EQ(record.mHasMark, result.mHasMark);
            EXPECT_EQ(record.mMarkedCell, result.mMarkedCell);
            EXPECT_THAT(record.mMarkedPosition.pos, ElementsAreArray(result.mMarkedPosition.pos));
            EXPECT_THAT(record.mMarkedPosition.rot, ElementsAreArray(result.mMarkedPosition.rot));
            EXPECT_EQ(record.mCurrentCrimeId, result.mCurrentCrimeId);
            EXPECT_EQ(record.mPaidCrimeId, result.mPaidCrimeId);
        }

        TEST_P(Esm3SaveLoadRecordTest, cellRefShouldNotChange)
        {
            CellRef record;
            record.blank();
            record.mRefNum.mIndex = std::numeric_limits<unsigned>::max();
            record.mRefNum.mContentFile = std::numeric_limits<int>::max();
            record.mRefID = generateRandomRefId();
            record.mScale = 2;
            record.mOwner = generateRandomRefId();
            record.mGlobalVariable = generateRandomString(100);
            record.mSoul = generateRandomRefId();
            record.mFaction = generateRandomRefId();
            record.mFactionRank = std::numeric_limits<int>::max();
            record.mChargeInt = std::numeric_limits<int>::max();
            record.mEnchantmentCharge = std::numeric_limits<float>::max();
            record.mCount = std::numeric_limits<int>::max();
            record.mTeleport = true;
            generateArray(record.mDoorDest.pos);
            generateArray(record.mDoorDest.rot);
            record.mDestCell = generateRandomString(100);
            record.mLockLevel = 0;
            record.mIsLocked = true;
            record.mKey = generateRandomRefId();
            record.mTrap = generateRandomRefId();
            record.mReferenceBlocked = std::numeric_limits<signed char>::max();
            generateArray(record.mPos.pos);
            generateArray(record.mPos.rot);
            CellRef result;
            saveAndLoadRecord(record, GetParam(), result);
            EXPECT_EQ(record.mRefNum.mIndex, result.mRefNum.mIndex);
            EXPECT_EQ(record.mRefNum.mContentFile, result.mRefNum.mContentFile);
            EXPECT_EQ(record.mRefID, result.mRefID);
            EXPECT_EQ(record.mScale, result.mScale);
            EXPECT_EQ(record.mOwner, result.mOwner);
            EXPECT_EQ(record.mGlobalVariable, result.mGlobalVariable);
            EXPECT_EQ(record.mSoul, result.mSoul);
            EXPECT_EQ(record.mFaction, result.mFaction);
            EXPECT_EQ(record.mFactionRank, result.mFactionRank);
            EXPECT_EQ(record.mChargeInt, result.mChargeInt);
            EXPECT_EQ(record.mEnchantmentCharge, result.mEnchantmentCharge);
            EXPECT_EQ(record.mCount, result.mCount);
            EXPECT_EQ(record.mTeleport, result.mTeleport);
            EXPECT_EQ(record.mDoorDest, result.mDoorDest);
            EXPECT_EQ(record.mDestCell, result.mDestCell);
            EXPECT_EQ(record.mLockLevel, result.mLockLevel);
            EXPECT_EQ(record.mIsLocked, result.mIsLocked);
            EXPECT_EQ(record.mKey, result.mKey);
            EXPECT_EQ(record.mTrap, result.mTrap);
            EXPECT_EQ(record.mReferenceBlocked, result.mReferenceBlocked);
            EXPECT_EQ(record.mPos, result.mPos);
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

        TEST_P(Esm3SaveLoadRecordTest, containerShouldNotChange)
        {
            Container record;
            record.blank();
            record.mId = generateRandomRefId();
            record.mInventory.mList.push_back(ESM::ContItem{ .mCount = 42, .mItem = generateRandomRefId(32) });
            record.mInventory.mList.push_back(ESM::ContItem{ .mCount = 13, .mItem = generateRandomRefId(32) });
            Container result;
            saveAndLoadRecord(record, GetParam(), result);
            EXPECT_EQ(result.mId, record.mId);
            EXPECT_EQ(result.mInventory.mList, record.mInventory.mList);
        }

        TEST_P(Esm3SaveLoadRecordTest, regionShouldNotChange)
        {
            Region record;
            record.blank();
            record.mId = generateRandomRefId();
            record.mSoundList.push_back(ESM::Region::SoundRef{ .mSound = generateRandomRefId(32), .mChance = 42 });
            record.mSoundList.push_back(ESM::Region::SoundRef{ .mSound = generateRandomRefId(32), .mChance = 13 });
            Region result;
            saveAndLoadRecord(record, GetParam(), result);
            EXPECT_EQ(result.mId, record.mId);
            EXPECT_EQ(result.mSoundList, record.mSoundList);
        }

        TEST_P(Esm3SaveLoadRecordTest, scriptShouldNotChange)
        {
            Script record;
            record.blank();
            record.mId = generateRandomRefId(32);
            record.mData.mNumShorts = 42;
            Script result;
            saveAndLoadRecord(record, GetParam(), result);
            EXPECT_EQ(result.mId, record.mId);
            EXPECT_EQ(result.mData.mNumShorts, record.mData.mNumShorts);
        }

        TEST_P(Esm3SaveLoadRecordTest, quickKeysShouldNotChange)
        {
            const QuickKeys record {
                .mKeys = {
                    {
                        .mType = QuickKeys::Type::Magic,
                        .mId = generateRandomRefId(32),
                    },
                    {
                        .mType = QuickKeys::Type::MagicItem,
                        .mId = generateRandomRefId(32),
                    },
                },
            };
            QuickKeys result;
            saveAndLoadRecord(record, GetParam(), result);
            EXPECT_EQ(result.mKeys, record.mKeys);
        }

        TEST_P(Esm3SaveLoadRecordTest, dialogueShouldNotChange)
        {
            Dialogue record;
            record.blank();
            record.mStringId = generateRandomString(32);
            record.mId = ESM::RefId::stringRefId(record.mStringId);
            Dialogue result;
            saveAndLoadRecord(record, GetParam(), result);
            EXPECT_EQ(result.mId, record.mId);
            EXPECT_EQ(result.mStringId, record.mStringId);
        }

        TEST_P(Esm3SaveLoadRecordTest, aiSequenceAiWanderShouldNotChange)
        {
            AiSequence::AiWander record;
            record.mData.mDistance = 1;
            record.mData.mDuration = 2;
            record.mData.mTimeOfDay = 3;
            constexpr std::uint8_t idle[8] = { 4, 5, 6, 7, 8, 9, 10, 11 };
            static_assert(std::size(idle) == std::size(record.mData.mIdle));
            std::copy(std::begin(idle), std::end(idle), record.mData.mIdle);
            record.mData.mShouldRepeat = 12;
            record.mDurationData.mRemainingDuration = 13;
            record.mStoredInitialActorPosition = true;
            constexpr float initialActorPosition[3] = { 15, 16, 17 };
            static_assert(std::size(initialActorPosition) == std::size(record.mInitialActorPosition.mValues));
            std::copy(
                std::begin(initialActorPosition), std::end(initialActorPosition), record.mInitialActorPosition.mValues);

            AiSequence::AiWander result;
            saveAndLoadRecord(record, GetParam(), result);

            EXPECT_EQ(result.mData.mDistance, record.mData.mDistance);
            EXPECT_EQ(result.mData.mDuration, record.mData.mDuration);
            EXPECT_EQ(result.mData.mTimeOfDay, record.mData.mTimeOfDay);
            EXPECT_THAT(result.mData.mIdle, ElementsAreArray(record.mData.mIdle));
            EXPECT_EQ(result.mData.mShouldRepeat, record.mData.mShouldRepeat);
            EXPECT_EQ(result.mDurationData.mRemainingDuration, record.mDurationData.mRemainingDuration);
            EXPECT_EQ(result.mStoredInitialActorPosition, record.mStoredInitialActorPosition);
            EXPECT_THAT(result.mInitialActorPosition.mValues, ElementsAreArray(record.mInitialActorPosition.mValues));
        }

        TEST_P(Esm3SaveLoadRecordTest, aiSequenceAiTravelShouldNotChange)
        {
            AiSequence::AiTravel record;
            record.mData.mX = 1;
            record.mData.mY = 2;
            record.mData.mZ = 3;
            record.mHidden = true;
            record.mRepeat = true;

            AiSequence::AiTravel result;
            saveAndLoadRecord(record, GetParam(), result);

            EXPECT_EQ(result.mData.mX, record.mData.mX);
            EXPECT_EQ(result.mData.mY, record.mData.mY);
            EXPECT_EQ(result.mData.mZ, record.mData.mZ);
            EXPECT_EQ(result.mHidden, record.mHidden);
            EXPECT_EQ(result.mRepeat, record.mRepeat);
        }

        TEST_P(Esm3SaveLoadRecordTest, aiSequenceAiEscortShouldNotChange)
        {
            AiSequence::AiEscort record;
            record.mData.mX = 1;
            record.mData.mY = 2;
            record.mData.mZ = 3;
            record.mData.mDuration = 4;
            record.mTargetActorId = 5;
            record.mTargetId = generateRandomRefId(32);
            record.mCellId = generateRandomString(257);
            record.mRemainingDuration = 6;
            record.mRepeat = true;

            AiSequence::AiEscort result;
            saveAndLoadRecord(record, GetParam(), result);

            EXPECT_EQ(result.mData.mX, record.mData.mX);
            EXPECT_EQ(result.mData.mY, record.mData.mY);
            EXPECT_EQ(result.mData.mZ, record.mData.mZ);
            if (GetParam() <= MaxOldAiPackageFormatVersion)
                EXPECT_EQ(result.mData.mDuration, record.mRemainingDuration);
            else
                EXPECT_EQ(result.mData.mDuration, record.mData.mDuration);
            EXPECT_EQ(result.mTargetActorId, record.mTargetActorId);
            EXPECT_EQ(result.mTargetId, record.mTargetId);
            EXPECT_EQ(result.mCellId, record.mCellId);
            EXPECT_EQ(result.mRemainingDuration, record.mRemainingDuration);
            EXPECT_EQ(result.mRepeat, record.mRepeat);
        }

        INSTANTIATE_TEST_SUITE_P(FormatVersions, Esm3SaveLoadRecordTest, ValuesIn(getFormats()));
    }
}
