#include "navmesh.hpp"
#include "vismask.hpp"

#include <components/sceneutil/navmesh.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>

#include <osg/PositionAttitudeTransform>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include <limits>

namespace MWRender
{
    NavMesh::NavMesh(const osg::ref_ptr<osg::Group>& root, bool enabled)
        : mRootNode(root)
        , mEnabled(enabled)
        , mId(std::numeric_limits<std::size_t>::max())
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

    void NavMesh::update(const dtNavMesh& navMesh, std::size_t id, const DetourNavigator::Version& version,
        const DetourNavigator::Settings& settings)
    {
        if (!mEnabled || (mGroup && mId == id && mVersion == version))
            return;

        mId = id;
        mVersion = version;
        if (mGroup)
            mRootNode->removeChild(mGroup);
        mGroup = SceneUtil::createNavMeshGroup(navMesh, settings);
        if (mGroup)
        {
            MWBase::Environment::get().getResourceSystem()->getSceneManager()->recreateShaders(mGroup, "debug");
            mGroup->setNodeMask(Mask_Debug);
            mRootNode->addChild(mGroup);
        }
    }

    void NavMesh::reset()
    {
        if (mGroup)
        {
            mRootNode->removeChild(mGroup);
            mGroup = nullptr;
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
