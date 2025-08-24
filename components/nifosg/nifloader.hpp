#ifndef OPENMW_COMPONENTS_NIFOSG_LOADER
#define OPENMW_COMPONENTS_NIFOSG_LOADER

#include <components/nif/niffile.hpp>

#include <osg/ref_ptr>

namespace SceneUtil
{
    class KeyframeHolder;
}

namespace osg
{
    class Node;
}

namespace Resource
{
    class ImageManager;
    class BgsmFileManager;
}

namespace NifOsg
{
    /// The main class responsible for loading NIF files into an OSG-Scenegraph.
    /// @par This scene graph is self-contained and can be cloned using osg::clone if desired. Particle emitters
    ///      and programs hold a pointer to their ParticleSystem, which would need to be manually updated when cloning.
    class Loader
    {
    public:
        /// Create a scene graph for the given NIF. Auto-detects when skinning is used and wraps the graph in a Skeleton
        /// if so.
        static osg::ref_ptr<osg::Node> load(
            Nif::FileView file, Resource::ImageManager* imageManager, Resource::BgsmFileManager* materialManager);

        /// Load keyframe controllers from the given kf file.
        static void loadKf(Nif::FileView kf, SceneUtil::KeyframeHolder& target);

        /// Set whether or not nodes marked as "MRK" should be shown.
        /// These should be hidden ingame, but visible in the editor.
        /// Default: false.
        static void setShowMarkers(bool show);

        static bool getShowMarkers();

        /// Set the mask to use for hidden nodes. The default is 0, i.e. updates to those nodes can no longer happen.
        /// If you need to run animations or physics for hidden nodes, you may want to set this to a non-zero mask and
        /// remove exactly that mask from the camera's cull mask.
        static void setHiddenNodeMask(unsigned int mask);
        static unsigned int getHiddenNodeMask();

        // Set the mask to use for nodes that ignore the crosshair intersection. The default is the default node mask.
        // This is used for NiCollisionSwitch nodes with NiCollisionSwitch state set to disabled.
        static void setIntersectionDisabledNodeMask(unsigned int mask);
        static unsigned int getIntersectionDisabledNodeMask();

        static void setSoftEffectEnabled(bool enabled);
        static bool getSoftEffectEnabled();

    private:
        static unsigned int sHiddenNodeMask;
        static unsigned int sIntersectionDisabledNodeMask;
        static bool sShowMarkers;
        static bool sSoftEffectEnabled;
    };

}

#endif
