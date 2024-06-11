#include <components/settings/shadermanager.hpp>
#include <components/testing/util.hpp>

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

namespace
{
    using namespace testing;
    using namespace Settings;

    struct ShaderSettingsTest : Test
    {
        template <typename F>
        void withSettingsFile(const std::string& content, F&& f)
        {
            auto path = TestingOpenMW::outputFilePath(
                std::string(UnitTest::GetInstance()->current_test_info()->name()) + ".yaml");

            {
                std::ofstream stream;
                stream.open(path);
                stream << content;
                stream.close();
            }

            f(path);
        }
    };

    TEST_F(ShaderSettingsTest, fail_to_fetch_then_set_and_succeed)
    {
        const std::string content =
            R"YAML(
config:
    shader:
        vec3_uniform: [1.0, 2.0]
)YAML";

        withSettingsFile(content, [](const auto& path) {
            EXPECT_TRUE(ShaderManager::get().load(path));
            EXPECT_FALSE(ShaderManager::get().getValue<osg::Vec3f>("shader", "vec3_uniform").has_value());
            EXPECT_TRUE(ShaderManager::get().setValue<osg::Vec3f>("shader", "vec3_uniform", osg::Vec3f(1, 2, 3)));
            EXPECT_TRUE(ShaderManager::get().getValue<osg::Vec3f>("shader", "vec3_uniform").has_value());
            EXPECT_EQ(ShaderManager::get().getValue<osg::Vec3f>("shader", "vec3_uniform").value(), osg::Vec3f(1, 2, 3));
            EXPECT_TRUE(ShaderManager::get().save());
        });
    }

    TEST_F(ShaderSettingsTest, fail_to_load_file_then_fail_to_set_and_get)
    {
        const std::string content =
            R"YAML(
config:
    shader:
        uniform: 12.0
 >Defeated by a sideways carrot
)YAML";

        withSettingsFile(content, [](const auto& path) {
            EXPECT_FALSE(ShaderManager::get().load(path));
            EXPECT_FALSE(ShaderManager::get().setValue("shader", "uniform", 12.0));
            EXPECT_FALSE(ShaderManager::get().getValue<float>("shader", "uniform").has_value());
            EXPECT_FALSE(ShaderManager::get().save());
        });
    }
}
