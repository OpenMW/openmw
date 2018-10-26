#ifndef OPENCS_VIEW_RENDER_ACTOR_H
#define OPENCS_VIEW_RENDER_ACTOR_H

#include <string>

#include <osg/ref_ptr>

#include <QObject>

#include <components/esm/loadarmo.hpp>
#include <components/sceneutil/visitor.hpp>

#include "../../model/world/actoradapter.hpp"

namespace osg
{
    class Group;
}

namespace CSMWorld
{
    class Data;
}

namespace SceneUtil
{
    class Skeleton;
}

namespace CSVRender
{
    /// Handles loading an npc or creature
    class Actor : public QObject
    {
        Q_OBJECT
    public:
        /// Creates an actor.
        /// \param id       The referenceable id
        /// \param type     The record type
        /// \param data     The data store
        Actor(const std::string& id, CSMWorld::Data& data);

        /// Retrieves the base node that meshes are attached to
        osg::Group* getBaseNode();

        /// (Re)creates the npc or creature renderable
        void update();

    private slots:
        void handleActorChanged(const std::string& refId);

    private:
        void loadSkeleton(const std::string& model);
        void loadBodyParts();
        void attachBodyPart(ESM::PartReferenceType, const std::string& mesh);

        std::string getBodyPartMesh(const std::string& bodyPartId);

        static const std::string MeshPrefix;

        std::string mId;
        CSMWorld::Data& mData;
        CSMWorld::ActorAdapter::ActorDataPtr mActorData;

        osg::ref_ptr<osg::Group> mBaseNode;
        SceneUtil::Skeleton* mSkeleton;
        SceneUtil::NodeMapVisitor::NodeMap mNodeMap;
    };
}

#endif
