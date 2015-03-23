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
        // TODO: add auto-detection for skinning. We will still need a "force skeleton" parameter
        // though, when assembling from several files, i.e. equipment parts
        /// Create a scene graph for the given NIF. Assumes no skinning is used.
        /// @param node The parent of the new root node for the created scene graph.
        osg::ref_ptr<osg::Node> load(Nif::NIFFilePtr file, TextKeyMap* textKeys = NULL);

        /// Create a scene graph for the given NIF. Assumes skinning will be used.
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
