#ifndef OPENMW_COMPONENTS_NIFOSG_LOADER
#define OPENMW_COMPONENTS_NIFOSG_LOADER

#include <components/nif/niffile.hpp>

#include <osg/ref_ptr>
#include <osg/Referenced>

#include "controller.hpp"

namespace osg
{
    class Node;
}

namespace Resource
{
    class TextureManager;
}

namespace NifOsg
{
    typedef std::multimap<float,std::string> TextKeyMap;

    struct TextKeyMapHolder : public osg::Object
    {
    public:
        TextKeyMapHolder() {}
        TextKeyMapHolder(const TextKeyMapHolder& copy, const osg::CopyOp& copyop)
            : osg::Object(copy, copyop)
            , mTextKeys(copy.mTextKeys)
        {}

        TextKeyMap mTextKeys;

        META_Object(OpenMW, TextKeyMapHolder)

    };

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

        META_Object(OpenMW, KeyframeHolder)

        typedef std::map<std::string, osg::ref_ptr<const KeyframeController> > KeyframeControllerMap;
        KeyframeControllerMap mKeyframeControllers;
    };

    class FrameSwitch : public osg::Group
    {
    public:
        FrameSwitch() {}

        FrameSwitch(const FrameSwitch& copy, const osg::CopyOp& copyop)
            : osg::Group(copy, copyop)
        {}

        META_Object(OpenMW, FrameSwitch)

        virtual void traverse(osg::NodeVisitor& nv);
    };

    class BillboardCallback : public osg::NodeCallback
    {
    public:
        BillboardCallback() {}
        BillboardCallback(const BillboardCallback& copy, const osg::CopyOp& copyop)
            : osg::NodeCallback(copy, copyop)
        {}

        META_Object(OpenMW, BillboardCallback)

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);
    };

    class UpdateMorphGeometry : public osg::Drawable::CullCallback
    {
    public:
        UpdateMorphGeometry()
            : mLastFrameNumber(0)
        {
        }

        UpdateMorphGeometry(const UpdateMorphGeometry& copy, const osg::CopyOp& copyop)
            : osg::Drawable::CullCallback(copy, copyop)
            , mLastFrameNumber(0)
        {
        }

        META_Object(OpenMW, UpdateMorphGeometry)

        virtual bool cull(osg::NodeVisitor* nv, osg::Drawable * drw, osg::State *) const;

    private:
        mutable unsigned int mLastFrameNumber;
    };

    // Callback to return a static bounding box for a MorphGeometry. The idea is to not recalculate the bounding box
    // every time the morph weights change. To do so we return a maximum containing box that is big enough for all possible combinations of morph targets.
    class StaticBoundingBoxCallback : public osg::Drawable::ComputeBoundingBoxCallback
    {
    public:
        StaticBoundingBoxCallback()
        {
        }

        StaticBoundingBoxCallback(const osg::BoundingBox& bounds)
            : mBoundingBox(bounds)
        {
        }

        StaticBoundingBoxCallback(const StaticBoundingBoxCallback& copy, const osg::CopyOp& copyop)
            : osg::Drawable::ComputeBoundingBoxCallback(copy, copyop)
            , mBoundingBox(copy.mBoundingBox)
        {
        }

        META_Object(OpenMW, StaticBoundingBoxCallback)

        virtual osg::BoundingBox computeBound(const osg::Drawable&) const
        {
            return mBoundingBox;
        }

        // For serialization.
        const osg::BoundingBox& getBoundingBox() const { return mBoundingBox; }
        inline void setBoundingBox(const osg::BoundingBox& bbox) { mBoundingBox = bbox; }

    private:
        osg::BoundingBox mBoundingBox;
    };

    /// The main class responsible for loading NIF files into an OSG-Scenegraph.
    /// @par This scene graph is self-contained and can be cloned using osg::clone if desired. Particle emitters
    ///      and programs hold a pointer to their ParticleSystem, which would need to be manually updated when cloning.
    class Loader
    {
    public:
        /// Create a scene graph for the given NIF. Auto-detects when skinning is used and wraps the graph in a Skeleton if so.
        static osg::ref_ptr<osg::Node> load(Nif::NIFFilePtr file, Resource::TextureManager* textureManager);

        /// Load keyframe controllers from the given kf file.
        static void loadKf(Nif::NIFFilePtr kf, KeyframeHolder& target);

        /// Set whether or not nodes marked as "MRK" should be shown.
        /// These should be hidden ingame, but visible in the editor.
        /// Default: false.
        static void setShowMarkers(bool show);

        static bool getShowMarkers();

    private:

        static bool sShowMarkers;
    };

}

#endif
