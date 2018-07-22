#ifndef OPENCS_VIEW_RENDER_ACTOR_H
#define OPENCS_VIEW_RENDER_ACTOR_H

#include <string>

#include <osg/ref_ptr>

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
    class Actor
    {
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

    private:
        void loadSkeleton(const std::string& model);
        void updateCreature();
        void updateNpc();

        std::string getBodyPartMesh(const std::string& bodyPartId);

        static const std::string MeshPrefix;

        std::string mId;
        int mType;
        CSMWorld::Data& mData;

        SceneUtil::Skeleton* mSkeleton;
        osg::ref_ptr<osg::Group> mBaseNode;
    };
}

#endif
