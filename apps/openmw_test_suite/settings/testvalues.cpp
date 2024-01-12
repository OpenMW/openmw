#include "components/misc/strings/conversion.hpp"
#include "components/settings/parser.hpp"
#include "components/settings/values.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#ifndef OPENMW_PROJECT_SOURCE_DIR
#define OPENMW_PROJECT_SOURCE_DIR "."
#endif

namespace Settings
{
    namespace
    {
        using namespace testing;

        struct SettingsValuesTest : Test
        {
            const std::filesystem::path mSettingsDefaultPath = std::filesystem::path{ OPENMW_PROJECT_SOURCE_DIR }
                / "files" / Misc::StringUtils::stringToU8String("settings-default.cfg");

            SettingsValuesTest()
            {
                Manager::mDefaultSettings.clear();
                Manager::mUserSettings.clear();
                Manager::mChangedSettings.clear();
                SettingsFileParser parser;
                parser.loadSettingsFile(mSettingsDefaultPath, Manager::mDefaultSettings);
            }
        };

        TEST_F(SettingsValuesTest, shouldLoadFromSettingsManager)
        {
            Index index;
            Values values(index);
            EXPECT_EQ(values.mCamera.mFieldOfView.get(), 60);
        }

        TEST_F(SettingsValuesTest, shouldFillIndexOnLoad)
        {
            Index index;
            Values values(index);
            EXPECT_EQ(index.get<float>("Camera", "field of view").get(), 60);
        }

        TEST_F(SettingsValuesTest, constructorShouldThrowExceptionOnMissingSetting)
        {
            Manager::mDefaultSettings.erase({ "Camera", "field of view" });
            Index index;
            EXPECT_THROW([&] { Values values(index); }(), std::runtime_error);
        }

        TEST_F(SettingsValuesTest, constructorShouldSanitize)
        {
            Manager::mUserSettings[std::make_pair("Camera", "field of view")] = "-1";
            Index index;
            Values values(index);
            EXPECT_EQ(values.mCamera.mFieldOfView.get(), 1);
        }

        TEST_F(SettingsValuesTest, constructorWithDefaultShouldDoLookup)
        {
            Manager::mUserSettings[std::make_pair("category", "value")] = "13";
            Index index;
            SettingValue<int> value{ index, "category", "value", 42 };
            EXPECT_EQ(value.get(), 13);
            value.reset();
            EXPECT_EQ(value.get(), 42);
        }

        TEST_F(SettingsValuesTest, constructorWithDefaultShouldSanitize)
        {
            Manager::mUserSettings[std::make_pair("category", "value")] = "2";
            Index index;
            SettingValue<int> value{ index, "category", "value", -1, Settings::makeClampSanitizerInt(0, 1) };
            EXPECT_EQ(value.get(), 1);
            value.reset();
            EXPECT_EQ(value.get(), 0);
        }

        TEST_F(SettingsValuesTest, constructorWithDefaultShouldFallbackToDefault)
        {
            Index index;
            const SettingValue<int> value{ index, "category", "value", 42 };
            EXPECT_EQ(value.get(), 42);
        }

        TEST_F(SettingsValuesTest, moveConstructorShouldSetDefaults)
        {
            Index index;
            Values defaultValues(index);
            Manager::mUserSettings.emplace(std::make_pair("Camera", "field of view"), "61");
            Values values(std::move(defaultValues));
            EXPECT_EQ(values.mCamera.mFieldOfView.get(), 61);
            values.mCamera.mFieldOfView.reset();
            EXPECT_EQ(values.mCamera.mFieldOfView.get(), 60);
        }

        TEST_F(SettingsValuesTest, moveConstructorShouldSanitize)
        {
            Index index;
            Values defaultValues(index);
            Manager::mUserSettings[std::make_pair("Camera", "field of view")] = "-1";
            Values values(std::move(defaultValues));
            EXPECT_EQ(values.mCamera.mFieldOfView.get(), 1);
        }

        TEST_F(SettingsValuesTest, moveConstructorShouldThrowOnMissingSetting)
        {
            Index index;
            SettingValue<int> defaultValue{ index, "category", "value", 42 };
            EXPECT_THROW([&] { SettingValue<int> value(std::move(defaultValue)); }(), std::runtime_error);
        }

        TEST_F(SettingsValuesTest, findShouldThrowExceptionOnTypeMismatch)
        {
            Index index;
            Values values(index);
            EXPECT_THROW(index.find<int>("Camera", "field of view"), std::invalid_argument);
        }

        TEST_F(SettingsValuesTest, findShouldReturnNullptrForAbsentSetting)
        {
            Index index;
            Values values(index);
            EXPECT_EQ(index.find<int>("foo", "bar"), nullptr);
        }

        TEST_F(SettingsValuesTest, getShouldThrowExceptionForAbsentSetting)
        {
            Index index;
            Values values(index);
            EXPECT_THROW(index.get<int>("foo", "bar").get(), std::invalid_argument);
        }

        TEST_F(SettingsValuesTest, setShouldChangeManagerUserSettings)
        {
            Index index;
            Values values(index);
            values.mCamera.mFieldOfView.set(42);
            EXPECT_EQ(Manager::mUserSettings.at({ "Camera", "field of view" }), "42");
            EXPECT_THAT(Manager::mChangedSettings, ElementsAre(std::make_pair("Camera", "field of view")));
        }

        TEST_F(SettingsValuesTest, setShouldNotChangeManagerChangedSettingsForNoChange)
        {
            Index index;
            Values values(index);
            values.mCamera.mFieldOfView.set(values.mCamera.mFieldOfView.get());
            EXPECT_THAT(Manager::mChangedSettings, ElementsAre());
        }
    }
}
