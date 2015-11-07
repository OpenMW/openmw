#include "pathgridpoint.hpp"

#include <QRegExp>

#include <OgreSceneManager.h>
#include <OgreSceneNode.h>

#include "../world/physicssystem.hpp"

#include "elements.hpp"

namespace CSVRender
{
    PathgridPoint::PathgridPoint(const std::string &name,
            Ogre::SceneNode *cellNode, const Ogre::Vector3 &pos, boost::shared_ptr<CSVWorld::PhysicsSystem> physics)
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

    // FIXME: Is there a way to identify the pathgrid point other than via the index?
    // ESM::Pathgrid::Edge itself uses the indicies so any change (add/delete) must be
    // propagated everywhere.
    std::pair<std::string, int> PathgridPoint::getIdAndIndex(const std::string &name)
    {
        // decode name
        QString id = QString(name.c_str());
        QRegExp pathgridRe("^Pathgrid_(.+)_(\\d+)$");

        if (id.isEmpty() || !id.startsWith("Pathgrid_"))
            return std::make_pair("", -1);

        std::string pathgridId = "";
        int index = -1;
        if (pathgridRe.indexIn(id) != -1)
        {
            pathgridId = pathgridRe.cap(1).toStdString();
            index = pathgridRe.cap(2).toInt();

            return std::make_pair(pathgridId, index);
        }

        return std::make_pair("", -1);
    }

    std::string PathgridPoint::getName(const std::string &pathgridId, const int index)
    {
        std::ostringstream stream;
        stream << "Pathgrid_" << pathgridId << "_" << index;

        return stream.str();
    }
}
