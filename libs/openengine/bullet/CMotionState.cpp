#include "CMotionState.h"
#include "physic.hpp"

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <components/nifbullet/bulletnifloader.hpp>

namespace OEngine {
namespace Physic
{

    CMotionState::CMotionState(PhysicEngine* eng,std::string name)
        : isPC(false)
        , isNPC(true)
    {
        pEng = eng;
        tr.setIdentity();
        pName = name;
    }

    void CMotionState::getWorldTransform(btTransform &worldTrans) const
    {
        worldTrans = tr;
    }

    void CMotionState::setWorldTransform(const btTransform &worldTrans)
    {
        tr = worldTrans;

        PhysicEvent evt;
        evt.isNPC = isNPC;
        evt.isPC = isPC;
        evt.newTransform = tr;
        evt.RigidBodyName = pName;

        if(isPC)
        {
            pEng->PEventList.push_back(evt);
        }
        else
        {
            pEng->NPEventList.push_back(evt);
        }
    }

}}
