#ifndef OPENCS_VIEW_RENDER_ACTOR_H
#define OPENCS_VIEW_RENDER_ACTOR_H

#include <string>

#include <osg/ref_ptr>

#include <QObject>

#include <components/esm/loadarmo.hpp>
#include <components/sceneutil/visitor.hpp>

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
        Actor(const std::string& id, int type, CSMWorld::Data& data);

        /// Retrieves the base node that meshes are attached to
        osg::Group* getBaseNode();

        /// (Re)creates the npc or creature renderable
        void update();

    private slots:
        void handleActorChanged(const std::string& refId);

    private:
        void updateCreature();
        void updateNpc();

        void loadSkeleton(const std::string& model);
        void loadBodyParts(const std::string& actorId);
        void attachBodyPart(ESM::PartReferenceType, const std::string& mesh);

        std::string getBodyPartMesh(const std::string& bodyPartId);

        static const std::string MeshPrefix;

        std::string mId;
        bool mInitialized;
        int mType;
        CSMWorld::Data& mData;

        osg::ref_ptr<osg::Group> mBaseNode;
        SceneUtil::Skeleton* mSkeleton;
        SceneUtil::NodeMapVisitor::NodeMap mNodeMap;
    };
}

#endif
