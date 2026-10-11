#include <components/sceneutil/userdata.hpp>

#include <osg/Group>
#include <osg/Node>

#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
    using namespace SceneUtil;

    TEST(SceneUtilUserData, find_should_return_nullptr_without_a_container)
    {
        osg::ref_ptr<osg::Node> node(new osg::Node);
        EXPECT_EQ(findUserData<int>(*node), nullptr);
    }

    TEST(SceneUtilUserData, find_should_return_the_added_value)
    {
        osg::ref_ptr<osg::Node> node(new osg::Node);
        addUserData(*node, std::string("hello"));
        const std::string* value = findUserData<std::string>(*node);
        ASSERT_NE(value, nullptr);
        EXPECT_EQ(*value, "hello");
    }

    TEST(SceneUtilUserData, find_should_distinguish_payload_types)
    {
        osg::ref_ptr<osg::Node> node(new osg::Node);
        addUserData(*node, 42);
        addUserData(*node, std::string("hello"));

        ASSERT_NE(findUserData<int>(*node), nullptr);
        EXPECT_EQ(*findUserData<int>(*node), 42);
        ASSERT_NE(findUserData<std::string>(*node), nullptr);
        EXPECT_EQ(*findUserData<std::string>(*node), "hello");
        EXPECT_EQ(findUserData<float>(*node), nullptr);
    }

    TEST(SceneUtilUserData, find_should_return_a_mutable_reference_into_the_container)
    {
        osg::ref_ptr<osg::Node> node(new osg::Node);
        addUserData(*node, 1);
        *findUserData<int>(*node) = 2;
        EXPECT_EQ(*findUserData<int>(*node), 2);
    }

    TEST(SceneUtilUserData, find_should_not_descend_into_children)
    {
        osg::ref_ptr<osg::Group> parent(new osg::Group);
        osg::ref_ptr<osg::Node> child(new osg::Node);
        parent->addChild(child);
        addUserData(*child, 1);
        EXPECT_EQ(findUserData<int>(*parent), nullptr);
    }

    TEST(SceneUtilUserData, clone_should_deep_copy_the_payload)
    {
        osg::ref_ptr<UserData<std::vector<int>>> original(new UserData<std::vector<int>>(std::vector<int>{ 1, 2, 3 }));
        osg::ref_ptr<osg::Object> copy(original->clone(osg::CopyOp::DEEP_COPY_ALL));

        auto* typed = dynamic_cast<UserData<std::vector<int>>*>(copy.get());
        ASSERT_NE(typed, nullptr);
        EXPECT_EQ(typed->mValue, (std::vector<int>{ 1, 2, 3 }));

        typed->mValue.push_back(4);
        EXPECT_EQ(original->mValue.size(), 3);
    }

    // Merged paged geometry carries one RefnumMarker per source reference, and the intersection code walks
    // them in order to map a vertex index back to a refnum. Finding only the first would lose refnums.
    TEST(SceneUtilUserData, for_each_should_visit_every_payload_of_the_type_in_order)
    {
        osg::ref_ptr<osg::Node> node(new osg::Node);
        addUserData(*node, 1);
        addUserData(*node, std::string("skipped"));
        addUserData(*node, 2);
        addUserData(*node, 3);

        std::vector<int> seen;
        forEachUserData<int>(*node, [&](int value) { seen.push_back(value); });
        EXPECT_EQ(seen, (std::vector<int>{ 1, 2, 3 }));
    }

    TEST(SceneUtilUserData, for_each_should_do_nothing_without_a_container)
    {
        osg::ref_ptr<osg::Node> node(new osg::Node);
        int calls = 0;
        forEachUserData<int>(*node, [&](int) { ++calls; });
        EXPECT_EQ(calls, 0);
    }

    // The serializer skips these by libraryName::className, so the pair has to stay "SceneUtil::UserData".
    TEST(SceneUtilUserData, should_report_one_serializer_name_for_every_payload_type)
    {
        const UserData<int> a{};
        const UserData<std::string> b{};
        EXPECT_STREQ(a.libraryName(), "SceneUtil");
        EXPECT_STREQ(a.className(), "UserData");
        EXPECT_STREQ(b.libraryName(), "SceneUtil");
        EXPECT_STREQ(b.className(), "UserData");
    }

    TEST(SceneUtilUserData, is_same_kind_as_should_discriminate_on_the_payload_type)
    {
        const UserData<int> a{};
        const UserData<int> b{};
        const UserData<std::string> c{};
        EXPECT_TRUE(a.isSameKindAs(&b));
        EXPECT_FALSE(a.isSameKindAs(&c));
    }
}
