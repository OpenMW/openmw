#ifndef OPENMW_COMPONENTS_NIFOSG_LOADER
#define OPENMW_COMPONENTS_NIFOSG_LOADER

#include <components/nif/niffile.hpp>

#include <components/nifcache/nifcache.hpp> // NIFFilePtr

#include <components/vfs/manager.hpp>

#include <osg/ref_ptr>

namespace osg
{
    class Node;
}

namespace NifOsg
{
    typedef std::multimap<float,std::string> TextKeyMap;

    /// The main class responsible for loading NIF files into an OSG-Scenegraph.
    /// @par This scene graph is self-contained and can be cloned using osg::clone if desired. Particle emitters
    ///      and programs hold a pointer to their ParticleSystem, which would need to be manually updated when cloning.
    class Loader
    {
    public:
        /// Create a scene graph for the given NIF. Auto-detects when skinning is used and calls loadAsSkeleton instead.
        /// @param node The parent of the new root node for the created scene graph.
        osg::ref_ptr<osg::Node> load(Nif::NIFFilePtr file, TextKeyMap* textKeys = NULL);

        /// Create a scene graph for the given NIF. Always creates a skeleton so that rigs can be attached on the created scene.
        osg::ref_ptr<osg::Node> loadAsSkeleton(Nif::NIFFilePtr file, TextKeyMap* textKeys = NULL);

        /// Load keyframe controllers from the given kf file onto the given scene graph.
        /// @param sourceIndex The source index for this animation source, used for identifying
        ///        which animation source a keyframe controller came from.
        void loadKf(Nif::NIFFilePtr kf, osg::Node* rootNode, int sourceIndex, TextKeyMap &textKeys);

        /// Set whether or not nodes marked as "MRK" should be shown.
        /// These should be hidden ingame, but visible in the editior.
        /// Default: false.
        static void setShowMarkers(bool show);

        const VFS::Manager* resourceManager;

    private:

        static bool sShowMarkers;
    };

}

#endif
