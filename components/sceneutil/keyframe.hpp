#ifndef OPENMW_COMPONENTS_SCENEUTIL_KEYFRAME_HPP
#define OPENMW_COMPONENTS_SCENEUTIL_KEYFRAME_HPP

#include <map>

#include <osg/Node>

#include <components/sceneutil/controller.hpp>
#include <components/sceneutil/textkeymap.hpp>
#include <components/resource/animation.hpp>

namespace SceneUtil
{
    class KeyframeController : public osg::NodeCallback, public SceneUtil::Controller
    {
    public:
        KeyframeController() {}

        KeyframeController(const KeyframeController& copy, const osg::CopyOp& copyop)
            : osg::NodeCallback(copy, copyop)
            , SceneUtil::Controller(copy)
        {}
        META_Object(SceneUtil, KeyframeController)

        virtual osg::Vec3f getTranslation(float time) const  { return osg::Vec3f(); }

        virtual void operator() (osg::Node* node, osg::NodeVisitor* nodeVisitor) override { traverse(node, nodeVisitor); }
    };

    /// Wrapper object containing an animation track as a ref-countable osg::Object.
    struct TextKeyMapHolder : public osg::Object
    {
    public:
        TextKeyMapHolder() {}
        TextKeyMapHolder(const TextKeyMapHolder& copy, const osg::CopyOp& copyop)
            : osg::Object(copy, copyop)
            , mTextKeys(copy.mTextKeys)
        {}

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
        typedef std::map<std::string, osg::ref_ptr<const KeyframeController> > KeyframeControllerMap;
        KeyframeControllerMap mKeyframeControllers;
    };

}

#endif
