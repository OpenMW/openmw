
#include "object.hpp"

#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreEntity.h>

#include "../../model/world/data.hpp"
#include "../../model/world/ref.hpp"
#include "../../model/world/refidcollection.hpp"

#include "../world/physicssystem.hpp"

#include "elements.hpp"

void CSVRender::Object::clearSceneNode (Ogre::SceneNode *node)
{
    for (Ogre::SceneNode::ObjectIterator iter = node->getAttachedObjectIterator();
        iter.hasMoreElements(); )
    {
        Ogre::MovableObject* object = dynamic_cast<Ogre::MovableObject*> (iter.getNext());
        node->getCreator()->destroyMovableObject (object);
    }

    for (Ogre::SceneNode::ChildNodeIterator iter = node->getChildIterator();
        iter.hasMoreElements(); )
    {
        Ogre::SceneNode* childNode = dynamic_cast<Ogre::SceneNode*> (iter.getNext());
        clearSceneNode (childNode);
        node->getCreator()->destroySceneNode (childNode);
   }
}

void CSVRender::Object::clear()
{
    mObject.setNull();

    clearSceneNode (mBase);
}

void CSVRender::Object::update()
{
    clear();

    std::string model;
    int error = 0; // 1 referemceanöe does not exist, 2 referenceable does not specify a mesh

    const CSMWorld::RefIdCollection& referenceables = mData.getReferenceables();

    int index = referenceables.searchId (mReferenceableId);

    if (index==-1)
        error = 1;
    else
    {
        /// \todo check for Deleted state (error 1)

        model = referenceables.getData (index,
            referenceables.findColumnIndex (CSMWorld::Columns::ColumnId_Model)).
            toString().toUtf8().constData();

        if (model.empty())
            error = 2;
    }

    if (error)
    {
        Ogre::Entity* entity = mBase->getCreator()->createEntity (Ogre::SceneManager::PT_CUBE);
        entity->setMaterialName("BaseWhite"); /// \todo adjust material according to error
        entity->setVisibilityFlags (Element_Reference);

        mBase->attachObject (entity);
    }
    else
    {
        mObject = NifOgre::Loader::createObjects (mBase, "Meshes\\" + model);
        mObject->setVisibilityFlags (Element_Reference);

        // mwrender/objects.cpp: insertBegin()
        //
        //  creates a ChildSceneNode from root if one doesn't exist for that cell (CellStore*)
        //  creates a ChildSceneNode for that cell scene node
        //  set the position & scale of the new node (for the object)
        //  set the orientation of the new node
        //  set the new node as the basenode (for the object)
        //
        // render/cell.cpp: Cell()
        //
        //  creates a ChildSceneNode from root
        //  set the position to origin <---- each object adjusts its position later
        //  pass the node when creating an Object
        //
        // render/object.cpp: Object()
        //  creates a ChildSceneNode from the cell scene node
        //
        if (!mReferenceId.empty())
        {
            const CSMWorld::CellRef& reference = getReference();
            Ogre::Quaternion xr (Ogre::Radian (-reference.mPos.rot[0]), Ogre::Vector3::UNIT_X);
            Ogre::Quaternion yr (Ogre::Radian (-reference.mPos.rot[1]), Ogre::Vector3::UNIT_Y);
            Ogre::Quaternion zr (Ogre::Radian (-reference.mPos.rot[2]), Ogre::Vector3::UNIT_Z);

            bool placeable = false; // FIXME
            mPhysics->addObject("meshes\\" + model,
                                mBase->getName(),
                                reference.mScale,
                                Ogre::Vector3(reference.mPos.pos[0],
                                              reference.mPos.pos[1],
                                              reference.mPos.pos[2]),
                                xr*yr*zr,
                                0, 0, false, placeable);
            mPhysics->addObject("meshes\\" + model,
                                mBase->getName(),
                                reference.mScale,
                                Ogre::Vector3(reference.mPos.pos[0],
                                              reference.mPos.pos[1],
                                              reference.mPos.pos[2]),
                                xr*yr*zr,
                                0, 0, true, placeable); // FIXME: why again with raycasting?
        }
    }
}

void CSVRender::Object::adjust()
{
    if (mReferenceId.empty())
        return;

    const CSMWorld::CellRef& reference = getReference();

    // position
    if (!mForceBaseToZero)
        mBase->setPosition (Ogre::Vector3 (
            reference.mPos.pos[0], reference.mPos.pos[1], reference.mPos.pos[2]));

    // orientation
    Ogre::Quaternion xr (Ogre::Radian (-reference.mPos.rot[0]), Ogre::Vector3::UNIT_X);

    Ogre::Quaternion yr (Ogre::Radian (-reference.mPos.rot[1]), Ogre::Vector3::UNIT_Y);

    Ogre::Quaternion zr (Ogre::Radian (-reference.mPos.rot[2]), Ogre::Vector3::UNIT_Z);

    mBase->setOrientation (xr*yr*zr);

    // scale
    mBase->setScale (reference.mScale, reference.mScale, reference.mScale);
}

const CSMWorld::CellRef& CSVRender::Object::getReference() const
{
    if (mReferenceId.empty())
        throw std::logic_error ("object does not represent a reference");

    return mData.getReferences().getRecord (mReferenceId).get();
}

CSVRender::Object::Object (const CSMWorld::Data& data, Ogre::SceneNode *cellNode,
    const std::string& id, bool referenceable, CSVWorld::PhysicsSystem* physics,
    bool forceBaseToZero)
: mData (data), mBase (0), mForceBaseToZero (forceBaseToZero), mPhysics(physics)
{
    mBase = cellNode->createChildSceneNode();

    if (referenceable)
    {
        mReferenceableId = id;
    }
    else
    {
        mReferenceId = id;
        mReferenceableId = getReference().mRefID;
    }

    update();
    adjust();
    std::cout << "obj pos" + std::to_string(mBase->getPosition().x)
                    + ", " + std::to_string(mBase->getPosition().y)
                    + ", " + std::to_string(mBase->getPosition().z) << std::endl;

}

CSVRender::Object::~Object()
{
    clear();

    if (mBase)
        mBase->getCreator()->destroySceneNode (mBase);
}

bool CSVRender::Object::referenceableDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    const CSMWorld::RefIdCollection& referenceables = mData.getReferenceables();

    int index = referenceables.searchId (mReferenceableId);

    if (index!=-1 && index>=topLeft.row() && index<=bottomRight.row())
    {
        update();
        adjust();
        return true;
    }

    return false;
}

bool CSVRender::Object::referenceableAboutToBeRemoved (const QModelIndex& parent, int start,
    int end)
{
    const CSMWorld::RefIdCollection& referenceables = mData.getReferenceables();

    int index = referenceables.searchId (mReferenceableId);

    if (index!=-1 && index>=start && index<=end)
    {
        // Deletion of referenceable-type objects is handled outside of Object.
        if (!mReferenceId.empty())
        {
            update();
            adjust();
            return true;
        }
    }

    return false;
}

bool CSVRender::Object::referenceDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    if (mReferenceId.empty())
        return false;

    const CSMWorld::RefCollection& references = mData.getReferences();

    int index = references.searchId (mReferenceId);

    if (index!=-1 && index>=topLeft.row() && index<=bottomRight.row())
    {
        int columnIndex =
            references.findColumnIndex (CSMWorld::Columns::ColumnId_ReferenceableId);

        if (columnIndex>=topLeft.column() && columnIndex<=bottomRight.row())
        {
            mReferenceableId =
                references.getData (index, columnIndex).toString().toUtf8().constData();

            update();
        }

        adjust();

        return true;
    }

    return false;
}

std::string CSVRender::Object::getReferenceId() const
{
    return mReferenceId;
}

std::string CSVRender::Object::getReferenceableId() const
{
    return mReferenceableId;
}
