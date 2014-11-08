#include "pathgridpoint.hpp"

#include <iostream> // FIXME

#include <OgreSceneManager.h>
#include <OgreSceneNode.h>

//#include "../../model/world/pathgrid.hpp"
#include "../world/physicssystem.hpp"

#include "elements.hpp"

namespace CSVRender
{
    PathgridPoint::PathgridPoint(const std::string &name,
            Ogre::SceneNode *cellNode, const Ogre::Vector3 &pos, CSVWorld::PhysicsSystem *physics)
        : mBase(cellNode), mPhysics(physics)
    {
        mBase = cellNode->createChildSceneNode();

        mPgPoint = NifOgre::Loader::createObjects(mBase, "pathgrid_pt.nif");
        mPgPoint->setVisibilityFlags(Element_Pathgrid);
        mBase->setPosition(pos);

        physics->addObject("pathgrid_pt.nif",
                mBase->getName(), name, 1, pos, Ogre::Quaternion::IDENTITY);
    }

    PathgridPoint::~PathgridPoint()
    {
        mPgPoint.setNull();

        mPhysics->removeObject(mBase->getName());

        if (mBase)
            mBase->getCreator()->destroySceneNode(mBase);
    }
}
