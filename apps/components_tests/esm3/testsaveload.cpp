#include <components/esm/fourcc.hpp>
#include <components/esm3/aipackage.hpp>
#include <components/esm3/aisequence.hpp>
#include <components/esm3/effectlist.hpp>
#include <components/esm3/esmreader.hpp>
#include <components/esm3/esmwriter.hpp>
#include <components/esm3/loadbook.hpp>
#include <components/esm3/loadclas.hpp>
#include <components/esm3/loadcont.hpp>
#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loaddial.hpp>
#include <components/esm3/loadfact.hpp>
#include <components/esm3/loadinfo.hpp>
#include <components/esm3/loadland.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/esm3/loadrace.hpp>
#include <components/esm3/loadregn.hpp>
#include <components/esm3/loadscpt.hpp>
#include <components/esm3/loadweap.hpp>
#include <components/esm3/player.hpp>
#include <components/esm3/quickkeys.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <iterator>
#include <limits>
#include <memory>
#include <numeric>
#include <random>

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

        Path makePath(std::string&& value)
        {
            Path result;
            result.set(std::move(value));
            return result;
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

    inline bool operator==(const ESM::Path& lhs, const ESM::Path& rhs)
    {
        return lhs.getOriginal() == rhs.getOriginal();
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
        concept HasSave = requires(T v, ESMWriter& w)
        {
            v.save(w);
        };

        template <class T>
        concept NotHasSave = !HasSave<T>;

        template <HasSave T>
        auto save(const T& record, ESMWriter& writer)
        {
            record.save(writer);
        }

        void save(const CellRef& record, ESMWriter& writer)
        {
            record.save(writer, true);
        }

        template <NotHasSave T>
        auto save(const T& record, ESMWriter& writer)
        {
            writer.writeComposite(record);
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
        concept HasLoad = requires(T v, ESMReader& r)
        {
            v.load(r);
        };

        template <class T>
        concept HasLoadWithDelete = requires(T v, ESMReader& r, bool& d)
        {
            v.load(r, d);
        };

        template <class T>
        concept NotHasLoad = !HasLoad<T> && !HasLoadWithDelete<T>;

        template <HasLoad T>
        void load(ESMReader& reader, T& record)
        {
            record.load(reader);
        }

        template <HasLoadWithDelete T>
        void load(ESMReader& reader, T& record)
        {
            bool deleted = false;
            record.load(reader, deleted);
        }

        void load(ESMReader& reader, CellRef& record)
        {
            bool deleted = false;
            record.load(reader, deleted, true);
        }

        template <NotHasLoad T>
        void load(ESMReader& reader, T& record)
        {
            reader.getComposite(record);
        }

        void load(ESMReader& reader, Land& record)
        {
            bool deleted = false;
            record.load(reader, deleted);
            if (deleted)
                return;
            record.mLandData = std::make_unique<LandRecordData>();
            reader.restoreContext(record.mContext);
            loadLandRecordData(record.mDataTypes, reader, *record.mLandData);
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

            void generateBytes(auto iterator, std::size_t count)
            {
                std::uniform_int_distribution<unsigned short> distribution{ 0,
                    std::numeric_limits<unsigned char>::max() };
                std::generate_n(iterator, count, [&] { return static_cast<unsigned char>(distribution(mRandom)); });
            }

            void generateStrings(auto iterator, std::size_t count)
            {
                std::uniform_int_distribution<std::size_t> distribution{ 1, 13 };
                std::generate_n(iterator, count, [&] { return generateRandomString(distribution(mRandom)); });
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
            record.mNumShorts = 42;
            Script result;
            saveAndLoadRecord(record, CurrentSaveGameFormatVersion, result);
            EXPECT_EQ(result.mId, record.mId);
            EXPECT_EQ(result.mNumShorts, record.mNumShorts);
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
            record.mNumShorts = 3;
            record.mNumFloats = 4;
            record.mNumLongs = 5;
            generateStrings(
                std::back_inserter(record.mVarNames), record.mNumShorts + record.mNumFloats + record.mNumLongs);
            generateBytes(std::back_inserter(record.mScriptData), 13);
            record.mScriptText = generateRandomString(17);

            Script result;
            saveAndLoadRecord(record, GetParam(), result);

            EXPECT_EQ(result.mId, record.mId);
            EXPECT_EQ(result.mNumShorts, record.mNumShorts);
            EXPECT_EQ(result.mNumFloats, record.mNumFloats);
            EXPECT_EQ(result.mNumShorts, record.mNumShorts);
            EXPECT_EQ(result.mVarNames, record.mVarNames);
            EXPECT_EQ(result.mScriptData, record.mScriptData);
            EXPECT_EQ(result.mScriptText, record.mScriptText);
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
            record.mTargetActor = RefNum{ .mIndex = 5, .mContentFile = -1 };
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
            if (GetParam() > MaxActorIdSaveGameFormatVersion)
            {
                EXPECT_EQ(result.mTargetActor, record.mTargetActor);
            }
            EXPECT_EQ(result.mTargetId, record.mTargetId);
            EXPECT_EQ(result.mCellId, record.mCellId);
            EXPECT_EQ(result.mRemainingDuration, record.mRemainingDuration);
            EXPECT_EQ(result.mRepeat, record.mRepeat);
        }

        TEST_P(Esm3SaveLoadRecordTest, aiDataShouldNotChange)
        {
            AIData record = {
                .mHello = 1,
                .mFight = 2,
                .mFlee = 3,
                .mAlarm = 4,
                .mServices = 5,
            };

            AIData result;
            saveAndLoadRecord(record, GetParam(), result);

            EXPECT_EQ(result.mHello, record.mHello);
            EXPECT_EQ(result.mFight, record.mFight);
            EXPECT_EQ(result.mFlee, record.mFlee);
            EXPECT_EQ(result.mAlarm, record.mAlarm);
            EXPECT_EQ(result.mServices, record.mServices);
        }

        TEST_P(Esm3SaveLoadRecordTest, enamShouldNotChange)
        {
            EffectList record;
            record.mList.emplace_back(IndexedENAMstruct{ {
                                                             .mEffectID = ESM::MagicEffect::SwiftSwim,
                                                             .mSkill = ESM::Skill::MediumArmor,
                                                             .mAttribute = ESM::Attribute::Agility,
                                                             .mRange = 4,
                                                             .mArea = 5,
                                                             .mDuration = 6,
                                                             .mMagnMin = 7,
                                                             .mMagnMax = 8,
                                                         },
                0 });

            EffectList result;
            saveAndLoadRecord(record, GetParam(), result);

            EXPECT_EQ(result.mList.size(), record.mList.size());
            EXPECT_EQ(result.mList[0].mData.mEffectID, record.mList[0].mData.mEffectID);
            EXPECT_EQ(result.mList[0].mData.mSkill, record.mList[0].mData.mSkill);
            EXPECT_EQ(result.mList[0].mData.mAttribute, record.mList[0].mData.mAttribute);
            EXPECT_EQ(result.mList[0].mData.mRange, record.mList[0].mData.mRange);
            EXPECT_EQ(result.mList[0].mData.mArea, record.mList[0].mData.mArea);
            EXPECT_EQ(result.mList[0].mData.mDuration, record.mList[0].mData.mDuration);
            EXPECT_EQ(result.mList[0].mData.mMagnMin, record.mList[0].mData.mMagnMin);
            EXPECT_EQ(result.mList[0].mData.mMagnMax, record.mList[0].mData.mMagnMax);
        }

        TEST_P(Esm3SaveLoadRecordTest, weaponShouldNotChange)
        {
            Weapon record = {
                .mData = {
                    .mWeight = 0,
                    .mValue = 1,
                    .mType = WeaponType::LongBladeTwoHand,
                    .mHealth = 3,
                    .mSpeed = 4,
                    .mReach = 5,
                    .mEnchant = 6,
                    .mChop = { 7, 8 },
                    .mSlash = { 9, 10 },
                    .mThrust = { 11, 12 },
                    .mFlags = 13,
                },
                .mRecordFlags = 0,
                .mId = generateRandomRefId(32),
                .mEnchant = generateRandomRefId(32),
                .mScript = generateRandomRefId(32),
                .mName = generateRandomString(32),
                .mModel = makePath(generateRandomString(32)),
                .mIcon = makePath(generateRandomString(32)),
            };

            Weapon result;
            saveAndLoadRecord(record, GetParam(), result);

            EXPECT_EQ(result.mData.mWeight, record.mData.mWeight);
            EXPECT_EQ(result.mData.mValue, record.mData.mValue);
            EXPECT_EQ(result.mData.mType, record.mData.mType);
            EXPECT_EQ(result.mData.mHealth, record.mData.mHealth);
            EXPECT_EQ(result.mData.mSpeed, record.mData.mSpeed);
            EXPECT_EQ(result.mData.mReach, record.mData.mReach);
            EXPECT_EQ(result.mData.mEnchant, record.mData.mEnchant);
            EXPECT_EQ(result.mData.mChop, record.mData.mChop);
            EXPECT_EQ(result.mData.mSlash, record.mData.mSlash);
            EXPECT_EQ(result.mData.mThrust, record.mData.mThrust);
            EXPECT_EQ(result.mData.mFlags, record.mData.mFlags);
            EXPECT_EQ(result.mId, record.mId);
            EXPECT_EQ(result.mEnchant, record.mEnchant);
            EXPECT_EQ(result.mScript, record.mScript);
            EXPECT_EQ(result.mName, record.mName);
            EXPECT_EQ(result.mModel, record.mModel);
            EXPECT_EQ(result.mIcon, record.mIcon);
        }

        TEST_P(Esm3SaveLoadRecordTest, infoShouldNotChange)
        {
            DialInfo record = {
                .mData = {
                    .mType = ESM::Dialogue::Topic,
                    .mDisposition = 1,
                    .mRank = 2,
                    .mGender = ESM::DialInfo::NA,
                    .mPCrank = 3,
                },
                .mSelects = {
                    ESM::DialogueCondition{
                        .mVariable = {},
                        .mValue = 42,
                        .mIndex = 0,
                        .mFunction = ESM::DialogueCondition::Function_Level,
                        .mComparison = ESM::DialogueCondition::Comp_Eq
                    },
                    ESM::DialogueCondition{
                        .mVariable = generateRandomString(32),
                        .mValue = 0,
                        .mIndex = 1,
                        .mFunction = ESM::DialogueCondition::Function_NotLocal,
                        .mComparison = ESM::DialogueCondition::Comp_Eq
                    },
                },
                .mId = generateRandomRefId(32),
                .mPrev = generateRandomRefId(32),
                .mNext = generateRandomRefId(32),
                .mActor = generateRandomRefId(32),
                .mRace = generateRandomRefId(32),
                .mClass = generateRandomRefId(32),
                .mFaction = generateRandomRefId(32),
                .mPcFaction = generateRandomRefId(32),
                .mCell = generateRandomRefId(32),
                .mSound = generateRandomString(32),
                .mResponse = generateRandomString(32),
                .mResultScript = generateRandomString(32),
                .mFactionLess = false,
                .mQuestStatus = ESM::DialInfo::QS_None,
            };

            DialInfo result;
            saveAndLoadRecord(record, GetParam(), result);

            EXPECT_EQ(result.mData.mType, record.mData.mType);
            EXPECT_EQ(result.mData.mDisposition, record.mData.mDisposition);
            EXPECT_EQ(result.mData.mRank, record.mData.mRank);
            EXPECT_EQ(result.mData.mGender, record.mData.mGender);
            EXPECT_EQ(result.mData.mPCrank, record.mData.mPCrank);
            EXPECT_EQ(result.mId, record.mId);
            EXPECT_EQ(result.mPrev, record.mPrev);
            EXPECT_EQ(result.mNext, record.mNext);
            EXPECT_EQ(result.mActor, record.mActor);
            EXPECT_EQ(result.mRace, record.mRace);
            EXPECT_EQ(result.mClass, record.mClass);
            EXPECT_EQ(result.mFaction, record.mFaction);
            EXPECT_EQ(result.mPcFaction, record.mPcFaction);
            EXPECT_EQ(result.mCell, record.mCell);
            EXPECT_EQ(result.mSound, record.mSound);
            EXPECT_EQ(result.mResponse, record.mResponse);
            EXPECT_EQ(result.mResultScript, record.mResultScript);
            EXPECT_EQ(result.mFactionLess, record.mFactionLess);
            EXPECT_EQ(result.mQuestStatus, record.mQuestStatus);
            EXPECT_EQ(result.mSelects.size(), record.mSelects.size());
            for (size_t i = 0; i < result.mSelects.size(); ++i)
            {
                const auto& resultS = result.mSelects[i];
                const auto& recordS = record.mSelects[i];
                EXPECT_EQ(resultS.mVariable, recordS.mVariable);
                EXPECT_EQ(resultS.mValue, recordS.mValue);
                EXPECT_EQ(resultS.mIndex, recordS.mIndex);
                EXPECT_EQ(resultS.mFunction, recordS.mFunction);
                EXPECT_EQ(resultS.mComparison, recordS.mComparison);
            }
        }

        TEST_P(Esm3SaveLoadRecordTest, landShouldNotChange)
        {
            LandRecordData data;
            std::iota(data.mHeights.begin(), data.mHeights.end(), 1.0f);
            std::for_each(data.mHeights.begin(), data.mHeights.end(), [](float& v) { v *= Land::sHeightScale; });
            data.mMinHeight = *std::min_element(data.mHeights.begin(), data.mHeights.end());
            data.mMaxHeight = *std::max_element(data.mHeights.begin(), data.mHeights.end());
            std::iota(data.mNormals.begin(), data.mNormals.end(), 2);
            std::iota(data.mTextures.begin(), data.mTextures.end(), 3);
            std::iota(data.mColours.begin(), data.mColours.end(), 4);
            data.mDataLoaded = Land::DATA_VNML | Land::DATA_VHGT | Land::DATA_VCLR | Land::DATA_VTEX;

            Land record;
            record.mFlags = Land::Flag_HeightsNormals | Land::Flag_Colors | Land::Flag_Textures;
            record.mX = 2;
            record.mY = 3;
            record.mDataTypes = Land::DATA_VNML | Land::DATA_VHGT | Land::DATA_WNAM | Land::DATA_VCLR | Land::DATA_VTEX;
            generateWnam(data.mHeights, record.mWnam);
            record.mLandData = std::make_unique<LandRecordData>(data);

            Land result;
            saveAndLoadRecord(record, GetParam(), result);

            EXPECT_EQ(result.mFlags, record.mFlags);
            EXPECT_EQ(result.mX, record.mX);
            EXPECT_EQ(result.mY, record.mY);
            EXPECT_EQ(result.mDataTypes, record.mDataTypes);
            EXPECT_EQ(result.mWnam, record.mWnam);
            EXPECT_EQ(result.mLandData->mHeights, record.mLandData->mHeights);
            EXPECT_EQ(result.mLandData->mMinHeight, record.mLandData->mMinHeight);
            EXPECT_EQ(result.mLandData->mMaxHeight, record.mLandData->mMaxHeight);
            EXPECT_EQ(result.mLandData->mNormals, record.mLandData->mNormals);
            EXPECT_EQ(result.mLandData->mTextures, record.mLandData->mTextures);
            EXPECT_EQ(result.mLandData->mColours, record.mLandData->mColours);
            EXPECT_EQ(result.mLandData->mDataLoaded, record.mLandData->mDataLoaded);
        }

        TEST_P(Esm3SaveLoadRecordTest, bookShouldNotChange)
        {
            Book record = {
                .mData = {
                    .mSkillId = Skill::Alchemy,
                    .mWeight = 1,
                    .mValue = 2,
                    .mIsScroll = true,
                    .mEnchant = 3,
                },
                .mName = generateRandomString(32),
                .mModel = makePath(generateRandomString(32)),
                .mIcon = makePath(generateRandomString(32)),
                .mText = generateRandomString(100),
                .mId = generateRandomRefId(32),
                .mScript = generateRandomRefId(32),
                .mEnchant = generateRandomRefId(32),
            };

            Book result;
            saveAndLoadRecord(record, GetParam(), result);

            EXPECT_EQ(result.mData.mSkillId, record.mData.mSkillId);
            EXPECT_EQ(result.mData.mWeight, record.mData.mWeight);
            EXPECT_EQ(result.mData.mValue, record.mData.mValue);
            EXPECT_EQ(result.mData.mIsScroll, record.mData.mIsScroll);
            EXPECT_EQ(result.mData.mEnchant, record.mData.mEnchant);
            EXPECT_EQ(result.mName, record.mName);
            EXPECT_EQ(result.mModel, record.mModel);
            EXPECT_EQ(result.mIcon, record.mIcon);
            EXPECT_EQ(result.mText, record.mText);
            EXPECT_EQ(result.mRecordFlags, record.mRecordFlags);
            EXPECT_EQ(result.mId, record.mId);
            EXPECT_EQ(result.mScript, record.mScript);
            EXPECT_EQ(result.mEnchant, record.mEnchant);
        }

        TEST_P(Esm3SaveLoadRecordTest, classShouldNotChange)
        {
            Class record = {
                .mName = generateRandomString(32),
                .mDescription = generateRandomString(100),
                .mId = generateRandomRefId(32),
                .mData = {
                    .mAttribute = { Attribute::Agility, Attribute::Luck },
                    .mSpecialization = Class::Magic,
                    .mMinorSkills = { Skill::Acrobatics, Skill::Athletics, Skill::Alchemy, Skill::Armorer, Skill::Alteration },
                    .mMajorSkills = { Skill::Axe, Skill::Block, Skill::BluntWeapon, Skill::Conjuration, Skill::Destruction },
                    .mIsPlayable = true,
                    .mServices = NPC::Services::AllItems,
                },
            };

            Class result;
            saveAndLoadRecord(record, GetParam(), result);

            EXPECT_EQ(result.mName, record.mName);
            EXPECT_EQ(result.mDescription, record.mDescription);
            EXPECT_EQ(result.mId, record.mId);
            EXPECT_EQ(result.mData.mAttribute, record.mData.mAttribute);
            EXPECT_EQ(result.mData.mSpecialization, record.mData.mSpecialization);
            EXPECT_EQ(result.mData.mMinorSkills, record.mData.mMinorSkills);
            EXPECT_EQ(result.mData.mMajorSkills, record.mData.mMajorSkills);
            EXPECT_EQ(result.mData.mIsPlayable, record.mData.mIsPlayable);
            EXPECT_EQ(result.mData.mServices, record.mData.mServices);
        }

        TEST_P(Esm3SaveLoadRecordTest, creatureShouldNotChange)
        {
            Creature record = {
                .mData = {
                    .mAttributes = {
                        { Attribute::Agility, 1 },
                        { Attribute::Endurance, 2 },
                        { Attribute::Intelligence, 3 },
                        { Attribute::Luck, 4 },
                        { Attribute::Personality, 5 },
                        { Attribute::Speed, 6 },
                        { Attribute::Strength, 7 },
                        { Attribute::Willpower, 8 }
                    },
                    .mType = Creature::Daedra,
                    .mLevel = 1,
                    .mHealth = 2,
                    .mMana = 3,
                    .mFatigue = 4,
                    .mSoul = 5,
                    .mCombat = 6,
                    .mMagic = 7,
                    .mStealth = 8,
                    .mAttack = { 9, 10, 11, 12, 13, 14 },
                    .mGold = 15,
                },
                .mBloodType = 16,
                .mFlags = Creature::Base | Creature::Walks,
                .mScale = 1,
                .mId = generateRandomRefId(32),
                .mScript = generateRandomRefId(32),
                .mName = generateRandomString(32),
                .mModel = makePath(generateRandomString(32)),
                .mOriginal = generateRandomRefId(32),
                .mAiData = {
                    .mHello = 17,
                    .mFight = 18,
                    .mFlee = 19,
                    .mAlarm = 20,
                    .mServices = NPC::Services::MagicItems,
                },
            };

            Creature result;
            saveAndLoadRecord(record, GetParam(), result);

            EXPECT_EQ(result.mData.mAttributes, record.mData.mAttributes);
            EXPECT_EQ(result.mData.mType, record.mData.mType);
            EXPECT_EQ(result.mData.mLevel, record.mData.mLevel);
            EXPECT_EQ(result.mData.mHealth, record.mData.mHealth);
            EXPECT_EQ(result.mData.mMana, record.mData.mMana);
            EXPECT_EQ(result.mData.mFatigue, record.mData.mFatigue);
            EXPECT_EQ(result.mData.mSoul, record.mData.mSoul);
            EXPECT_EQ(result.mData.mCombat, record.mData.mCombat);
            EXPECT_EQ(result.mData.mMagic, record.mData.mMagic);
            EXPECT_EQ(result.mData.mStealth, record.mData.mStealth);
            for (size_t i = 0; i < std::size(result.mData.mAttack); ++i)
                EXPECT_EQ(result.mData.mAttack[i], record.mData.mAttack[i]);
            EXPECT_EQ(result.mData.mGold, record.mData.mGold);
            EXPECT_EQ(result.mBloodType, record.mBloodType);
            EXPECT_EQ(result.mFlags, record.mFlags);
            EXPECT_EQ(result.mScale, record.mScale);
            EXPECT_EQ(result.mId, record.mId);
            EXPECT_EQ(result.mScript, record.mScript);
            EXPECT_EQ(result.mName, record.mName);
            EXPECT_EQ(result.mModel, record.mModel);
            EXPECT_EQ(result.mOriginal, record.mOriginal);
            EXPECT_EQ(result.mAiData.mHello, record.mAiData.mHello);
            EXPECT_EQ(result.mAiData.mFight, record.mAiData.mFight);
            EXPECT_EQ(result.mAiData.mFlee, record.mAiData.mFlee);
            EXPECT_EQ(result.mAiData.mAlarm, record.mAiData.mAlarm);
            EXPECT_EQ(result.mAiData.mServices, record.mAiData.mServices);
        }

        TEST_P(Esm3SaveLoadRecordTest, factionShouldNotChange)
        {
            RankData rankData = {
                .mAttribute1 = 1,
                .mAttribute2 = 2,
                .mPrimarySkill = 3,
                .mFavouredSkill = 4,
                .mFactReputation = 5,
            };
            Faction record = {
                .mName = generateRandomString(32),
                .mId = generateRandomRefId(32),
                .mData = {
                    .mAttribute = { Attribute::Willpower, Attribute::Endurance },
                    .mRankData = {
                        rankData, rankData, rankData, rankData, rankData, rankData, rankData, rankData, rankData, rankData
                    },
                    .mSkills = { Skill::Acrobatics, Skill::Athletics, Skill::Alchemy, {}, {}, {}, {} },
                    .mFlags = Faction::Flags::Hidden,
                },
                .mRanks = { generateRandomString(32), {}, {}, {}, {}, {}, {}, {}, {}, {} },
            };

            Faction result;
            saveAndLoadRecord(record, GetParam(), result);

            EXPECT_EQ(result.mName, record.mName);
            EXPECT_EQ(result.mId, record.mId);
            EXPECT_EQ(result.mData.mAttribute, record.mData.mAttribute);
            for (size_t i = 0; i < result.mData.mRankData.size(); ++i)
            {
                EXPECT_EQ(result.mData.mRankData[i].mAttribute1, record.mData.mRankData[i].mAttribute1);
                EXPECT_EQ(result.mData.mRankData[i].mAttribute2, record.mData.mRankData[i].mAttribute2);
                EXPECT_EQ(result.mData.mRankData[i].mPrimarySkill, record.mData.mRankData[i].mPrimarySkill);
                EXPECT_EQ(result.mData.mRankData[i].mFavouredSkill, record.mData.mRankData[i].mFavouredSkill);
                EXPECT_EQ(result.mData.mRankData[i].mFactReputation, record.mData.mRankData[i].mFactReputation);
            }
            EXPECT_EQ(result.mData.mSkills, record.mData.mSkills);
            EXPECT_EQ(result.mData.mFlags, record.mData.mFlags);
            EXPECT_EQ(result.mRanks, record.mRanks);
        }

        TEST_P(Esm3SaveLoadRecordTest, npcShouldNotChange)
        {
            NPC record = {
                .mNpdtType = NPC::NPC_DEFAULT,
                .mNpdt = {
                    .mAttributes = {
                        { Attribute::Agility, 1 },
                        { Attribute::Endurance, 2 },
                        { Attribute::Intelligence, 3 },
                        { Attribute::Luck, 4 },
                        { Attribute::Personality, 5 },
                        { Attribute::Speed, 6 },
                        { Attribute::Strength, 7 },
                        { Attribute::Willpower, 8 }
                    },
                    .mSkills = {
                        { Skill::Acrobatics, 125 }
                    },
                    .mLevel = 1,
                    .mHealth = 2,
                    .mMana = 3,
                    .mFatigue = 4,
                    .mDisposition = 5,
                    .mReputation = 6,
                    .mRank = 7,
                    .mGold = 8,
                },
                .mBloodType = 16,
                .mFlags = NPC::Base,
                .mAiData = {
                    .mHello = 17,
                    .mFight = 18,
                    .mFlee = 19,
                    .mAlarm = 20,
                    .mServices = NPC::Services::MagicItems,
                },
                .mId = generateRandomRefId(32),
                .mRace = generateRandomRefId(32),
                .mClass = generateRandomRefId(32),
                .mFaction = generateRandomRefId(32),
                .mScript = generateRandomRefId(32),
                .mName = generateRandomString(32),
                .mModel = makePath(generateRandomString(32)),
                .mHair = generateRandomRefId(32),
                .mHead = generateRandomRefId(32),
            };

            NPC result;
            saveAndLoadRecord(record, GetParam(), result);

            EXPECT_EQ(result.mNpdt.mAttributes, record.mNpdt.mAttributes);
            for (const auto& [skill, value] : record.mNpdt.mSkills)
                EXPECT_EQ(result.mNpdt.getSkill(skill), value);
            EXPECT_EQ(result.mNpdt.mLevel, record.mNpdt.mLevel);
            EXPECT_EQ(result.mNpdt.mHealth, record.mNpdt.mHealth);
            EXPECT_EQ(result.mNpdt.mMana, record.mNpdt.mMana);
            EXPECT_EQ(result.mNpdt.mFatigue, record.mNpdt.mFatigue);
            EXPECT_EQ(result.mNpdt.mDisposition, record.mNpdt.mDisposition);
            EXPECT_EQ(result.mNpdt.mReputation, record.mNpdt.mReputation);
            EXPECT_EQ(result.mNpdt.mRank, record.mNpdt.mRank);
            EXPECT_EQ(result.mNpdt.mGold, record.mNpdt.mGold);
            EXPECT_EQ(result.mBloodType, record.mBloodType);
            EXPECT_EQ(result.mFlags, record.mFlags);
            EXPECT_EQ(result.mAiData.mHello, record.mAiData.mHello);
            EXPECT_EQ(result.mAiData.mFight, record.mAiData.mFight);
            EXPECT_EQ(result.mAiData.mFlee, record.mAiData.mFlee);
            EXPECT_EQ(result.mAiData.mAlarm, record.mAiData.mAlarm);
            EXPECT_EQ(result.mAiData.mServices, record.mAiData.mServices);
            EXPECT_EQ(result.mId, record.mId);
            EXPECT_EQ(result.mRace, record.mRace);
            EXPECT_EQ(result.mClass, record.mClass);
            EXPECT_EQ(result.mFaction, record.mFaction);
            EXPECT_EQ(result.mScript, record.mScript);
            EXPECT_EQ(result.mName, record.mName);
            EXPECT_EQ(result.mModel, record.mModel);
            EXPECT_EQ(result.mHair, record.mHair);
            EXPECT_EQ(result.mHead, record.mHead);
        }

        TEST_P(Esm3SaveLoadRecordTest, raceShouldNotChange)
        {
            Race record = {
                .mData = {
                    .mBonus = { { { .mSkill = Skill::Athletics, .mBonus = 10 }, {}, {}, {}, {}, {}, {} } },
                    .mAttributeValues = { { Attribute::Endurance, { .mMale = 1, .mFemale = 2 } } },
                    .mMaleHeight = 1.f,
                    .mFemaleHeight = 1.1f,
                    .mMaleWeight = 1.2f,
                    .mFemaleWeight = 1.3f,
                    .mFlags = Race::Flags::Playable,
                },
                .mName = generateRandomString(32),
                .mDescription = generateRandomString(100),
                .mId = generateRandomRefId(32),
            };

            Race result;
            saveAndLoadRecord(record, GetParam(), result);

            for (size_t i = 0; i < result.mData.mBonus.size(); ++i)
            {
                EXPECT_EQ(result.mData.mBonus[i].mSkill, record.mData.mBonus[i].mSkill);
                EXPECT_EQ(result.mData.mBonus[i].mBonus, record.mData.mBonus[i].mBonus);
            }
            for (const auto& [attribute, value] : record.mData.mAttributeValues)
            {
                EXPECT_EQ(result.mData.getAttribute(attribute, true), value.mMale);
                EXPECT_EQ(result.mData.getAttribute(attribute, false), value.mFemale);
            }
            EXPECT_EQ(result.mData.mMaleHeight, record.mData.mMaleHeight);
            EXPECT_EQ(result.mData.mFemaleHeight, record.mData.mFemaleHeight);
            EXPECT_EQ(result.mData.mMaleWeight, record.mData.mMaleWeight);
            EXPECT_EQ(result.mData.mFemaleWeight, record.mData.mFemaleWeight);
            EXPECT_EQ(result.mData.mFlags, record.mData.mFlags);
            EXPECT_EQ(result.mName, record.mName);
            EXPECT_EQ(result.mDescription, record.mDescription);
            EXPECT_EQ(result.mId, record.mId);
        }

        INSTANTIATE_TEST_SUITE_P(FormatVersions, Esm3SaveLoadRecordTest, ValuesIn(getFormats()));
    }
}
