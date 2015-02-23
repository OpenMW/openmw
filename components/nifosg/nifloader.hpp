#ifndef OPENMW_COMPONENTS_NIFOSG_LOADER
#define OPENMW_COMPONENTS_NIFOSG_LOADER

#include <components/nif/niffile.hpp>

#include <components/nifcache/nifcache.hpp> // NIFFilePtr

#include <osg/Group>

#include "controller.hpp"

namespace osg
{
    class Geometry;
}
namespace osgAnimation
{
    class Bone;
}

namespace Nif
{
    class Node;
    class NiTriShape;
    class Property;
}
namespace Bsa
{
    class BSAFile;
}

namespace NifOsg
{

    /// The main class responsible for loading NIF files into an OSG-Scenegraph.
    class Loader
    {
    public:
        /// @param node The parent of the root node for the created NIF file.
        void load(Nif::NIFFilePtr file, osg::Group* parentNode);

        void loadAsSkeleton(Nif::NIFFilePtr file, osg::Group* parentNode);

        // FIXME replace with resource system
        Bsa::BSAFile* resourceManager;

        // FIXME move
        std::vector<Controller> mControllers;

    private:

        /// @param createSkeleton If true, use an osgAnimation::Bone for NIF nodes, otherwise an osg::MatrixTransform.
        void handleNode(const Nif::Node* nifNode, osg::Group* parentNode, bool createSkeleton, std::map<int, int> boundTextures);

        void handleMeshControllers(const Nif::Node* nifNode, osg::MatrixTransform* transformNode, const std::map<int, int>& boundTextures);

        void handleNodeControllers(const Nif::Node* nifNode, osg::MatrixTransform* transformNode);

        void handleMaterialControllers(const Nif::Property* materialProperty, osg::StateSet* stateset);

        void handleProperty (const Nif::Property* property, const Nif::Node* nifNode,
                             osg::Node* node, std::map<int, int>& boundTextures);

        // Creates an osg::Geometry object for the given TriShape, populates it, and attaches it to the given node.
        void handleTriShape(const Nif::NiTriShape* triShape, osg::Group* parentNode, const std::map<int, int>& boundTextures);

        // Fills the vertex data for the given TriShape into the given Geometry.
        void triShapeToGeometry(const Nif::NiTriShape* triShape, osg::Geometry* geom, const std::map<int, int>& boundTextures);

        // Creates a skinned osg::Geometry object for the given TriShape, populates it, and attaches it to the given node.
        void handleSkinnedTriShape(const Nif::NiTriShape* triShape, osg::Group* parentNode, const std::map<int, int>& boundTextures);

        // Applies the Properties of the given nifNode onto the StateSet of the given OSG node.
        void applyNodeProperties(const Nif::Node* nifNode, osg::Node* applyTo, std::map<int, int>& boundTextures);

        void applyMaterialProperties(osg::StateSet* stateset, const std::vector<const Nif::Property*>& properties, bool hasVertexColors);

        void createController(const Nif::Controller* ctrl, boost::shared_ptr<ControllerValue> value, int animflags);

        Nif::NIFFilePtr mNif;

        osg::Group* mRootNode;
        osg::Group* mSkeleton;
    };

}

#endif
