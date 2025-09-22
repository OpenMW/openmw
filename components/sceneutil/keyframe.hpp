#ifndef OPENMW_COMPONENTS_SCENEUTIL_KEYFRAME_HPP
#define OPENMW_COMPONENTS_SCENEUTIL_KEYFRAME_HPP

#include <map>
#include <optional>

#include <osg/Object>

#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/textkeymap.hpp>

namespace SceneUtil
{
    /// @note Derived classes are expected to derive from osg::Callback and implement getAsCallback().
    class KeyframeController : public SceneUtil::Controller, public virtual osg::Object
    {
    public:
        KeyframeController() {}

        KeyframeController(const KeyframeController& copy, const osg::CopyOp& copyop)
            : osg::Object(copy, copyop)
            , SceneUtil::Controller(copy)
        {
        }

        std::shared_ptr<float> mTime = std::make_shared<float>(0.0f);

        struct KfTransform
        {
            std::optional<osg::Vec3f> mTranslation;
            std::optional<osg::Quat> mRotation;
            std::optional<float> mScale;
        };

        virtual osg::Vec3f getTranslation(float time) const { return osg::Vec3f(); }

        virtual KfTransform getCurrentTransformation(osg::NodeVisitor* nv) { return KfTransform(); }

        /// @note We could drop this function in favour of osg::Object::asCallback from OSG 3.6 on.
        virtual osg::Callback* getAsCallback() = 0;
    };

    /// Wrapper object containing an animation track as a ref-countable osg::Object.
    struct TextKeyMapHolder : public osg::Object
    {
    public:
        TextKeyMapHolder() {}
        TextKeyMapHolder(const TextKeyMapHolder& copy, const osg::CopyOp& copyop)
            : osg::Object(copy, copyop)
            , mTextKeys(copy.mTextKeys)
        {
        }

        TextKeyMap mTextKeys;

        META_Object(SceneUtil, TextKeyMapHolder)
    };

    /// Wrapper object containing the animation track and its KeyframeControllers.
    class KeyframeHolder : public osg::Object
    {
    public:
        KeyframeHolder() {}
        KeyframeHolder(const KeyframeHolder& copy, const osg::CopyOp& copyop)
            : mTextKeys(copy.mTextKeys)
            , mKeyframeControllers(copy.mKeyframeControllers)
        {
        }

        TextKeyMap mTextKeys;

        META_Object(SceneUtil, KeyframeHolder)

        /// Controllers mapped to node name.
        typedef std::map<std::string, osg::ref_ptr<const KeyframeController>> KeyframeControllerMap;
        KeyframeControllerMap mKeyframeControllers;
    };

}

#endif
