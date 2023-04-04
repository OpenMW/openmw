#include <components/esm3/loadacti.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/esm3/loadcont.hpp>
#include <components/esm3/loaddoor.hpp>
#include <components/esm3/loadgmst.hpp>
#include <components/esm3/loadland.hpp>
#include <components/esm3/loadstat.hpp>
#include <components/esm3/variant.hpp>
#include <components/esmloader/esmdata.hpp>

#include <gtest/gtest.h>

#include <array>
#include <functional>
#include <string>
#include <vector>

namespace
{
    using namespace testing;
    using namespace EsmLoader;

    struct Params
    {
        std::string mRefId;
        ESM::RecNameInts mType;
        std::string mResult;
        std::function<void(EsmData&)> mPushBack;
    };

    struct EsmLoaderGetModelTest : TestWithParam<Params>
    {
    };

    TEST_P(EsmLoaderGetModelTest, shouldReturnFoundModelName)
    {
        EsmData data;
        GetParam().mPushBack(data);
        EXPECT_EQ(EsmLoader::getModel(data, ESM::RefId::stringRefId(GetParam().mRefId), GetParam().mType),
            GetParam().mResult);
    }

    void pushBack(ESM::Activator&& value, EsmData& esmData)
    {
        esmData.mActivators.push_back(std::move(value));
    }

    void pushBack(ESM::Container&& value, EsmData& esmData)
    {
        esmData.mContainers.push_back(std::move(value));
    }

    void pushBack(ESM::Door&& value, EsmData& esmData)
    {
        esmData.mDoors.push_back(std::move(value));
    }

    void pushBack(ESM::Static&& value, EsmData& esmData)
    {
        esmData.mStatics.push_back(std::move(value));
    }

    template <class T>
    struct PushBack
    {
        std::string mId;
        std::string mModel;

        void operator()(EsmData& esmData) const
        {
            T value;
            value.mId = ESM::RefId::stringRefId(mId);
            value.mModel = mModel;
            pushBack(std::move(value), esmData);
        }
    };

    const std::array params = {
        Params{ "acti_ref_id", ESM::REC_ACTI, "acti_model", PushBack<ESM::Activator>{ "acti_ref_id", "acti_model" } },
        Params{ "cont_ref_id", ESM::REC_CONT, "cont_model", PushBack<ESM::Container>{ "cont_ref_id", "cont_model" } },
        Params{ "door_ref_id", ESM::REC_DOOR, "door_model", PushBack<ESM::Door>{ "door_ref_id", "door_model" } },
        Params{
            "static_ref_id", ESM::REC_STAT, "static_model", PushBack<ESM::Static>{ "static_ref_id", "static_model" } },
        Params{ "acti_ref_id_a", ESM::REC_ACTI, "", PushBack<ESM::Activator>{ "acti_ref_id_z", "acti_model" } },
        Params{ "cont_ref_id_a", ESM::REC_CONT, "", PushBack<ESM::Container>{ "cont_ref_id_z", "cont_model" } },
        Params{ "door_ref_id_a", ESM::REC_DOOR, "", PushBack<ESM::Door>{ "door_ref_id_z", "door_model" } },
        Params{ "static_ref_id_a", ESM::REC_STAT, "", PushBack<ESM::Static>{ "static_ref_id_z", "static_model" } },
        Params{ "acti_ref_id_z", ESM::REC_ACTI, "", PushBack<ESM::Activator>{ "acti_ref_id_a", "acti_model" } },
        Params{ "cont_ref_id_z", ESM::REC_CONT, "", PushBack<ESM::Container>{ "cont_ref_id_a", "cont_model" } },
        Params{ "door_ref_id_z", ESM::REC_DOOR, "", PushBack<ESM::Door>{ "door_ref_id_a", "door_model" } },
        Params{ "static_ref_id_z", ESM::REC_STAT, "", PushBack<ESM::Static>{ "static_ref_id_a", "static_model" } },
        Params{ "ref_id", ESM::REC_STAT, "", [](EsmData&) {} },
        Params{ "ref_id", ESM::REC_BOOK, "", [](EsmData&) {} },
    };

    INSTANTIATE_TEST_SUITE_P(Params, EsmLoaderGetModelTest, ValuesIn(params));

    TEST(EsmLoaderGetGameSettingTest, shouldReturnFoundValue)
    {
        std::vector<ESM::GameSetting> settings;
        ESM::GameSetting setting;
        setting.mId = ESM::RefId::stringRefId("setting");
        setting.mValue = ESM::Variant(42);
        setting.mRecordFlags = 0;
        settings.push_back(setting);
        EXPECT_EQ(EsmLoader::getGameSetting(settings, "setting"), ESM::Variant(42));
    }

    TEST(EsmLoaderGetGameSettingTest, shouldThrowExceptionWhenNotFound)
    {
        const std::vector<ESM::GameSetting> settings;
        EXPECT_THROW(EsmLoader::getGameSetting(settings, "setting"), std::runtime_error);
    }
}
