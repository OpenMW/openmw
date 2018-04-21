#include "navmesh.hpp"
#include "vismask.hpp"

#include <components/sceneutil/navmesh.hpp>

#include <osg/PositionAttitudeTransform>

namespace MWRender
{
    NavMesh::NavMesh(const osg::ref_ptr<osg::Group>& root, bool enabled)
        : mRootNode(root)
        , mEnabled(enabled)
        , mRevision(0)
    {
    }

    NavMesh::~NavMesh()
    {
        if (mEnabled)
            disable();
    }

    bool NavMesh::toggle()
    {
        if (mEnabled)
            disable();
        else
            enable();

        return mEnabled;
    }

    void NavMesh::update(const DetourNavigator::SharedNavMesh& sharedNavMesh, std::size_t revision,
                         const DetourNavigator::Settings& settings)
    {
        if (!mEnabled || mRevision >= revision)
            return;

        mRevision = revision;
        if (mGroup)
            mRootNode->removeChild(mGroup);
        mGroup = SceneUtil::createNavMeshGroup(*sharedNavMesh.lock(), settings);
        if (mGroup)
        {
            mGroup->setNodeMask(Mask_Debug);
            mRootNode->addChild(mGroup);
        }
    }

    void NavMesh::enable()
    {
        if (mGroup)
            mRootNode->addChild(mGroup);
        mEnabled = true;
    }

    void NavMesh::disable()
    {
        if (mGroup)
            mRootNode->removeChild(mGroup);
        mEnabled = false;
    }
}
