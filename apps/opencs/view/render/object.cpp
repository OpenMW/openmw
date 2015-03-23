#include "object.hpp"

#include <osg/Group>

#include "../../model/world/data.hpp"
#include "../../model/world/ref.hpp"
#include "../../model/world/refidcollection.hpp"

#include "../world/physicssystem.hpp"

#include <components/nifosg/nifloader.hpp>

#include "elements.hpp"

void CSVRender::Object::clear()
{
}

void CSVRender::Object::update()
{
    clear();

    std::string model;
    int error = 0; // 1 referenceable does not exist, 2 referenceable does not specify a mesh

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
        /*
        Ogre::Entity* entity = mBase->getCreator()->createEntity (Ogre::SceneManager::PT_CUBE);
        entity->setMaterialName("BaseWhite"); /// \todo adjust material according to error
        entity->setVisibilityFlags (Element_Reference);

        mBase->attachObject (entity);
        */
    }
    else
    {
        NifOsg::Loader loader;
        loader.resourceManager = mVFS;

        std::string path = "meshes\\" + model;

        Nif::NIFFilePtr file(new Nif::NIFFile(mVFS->get(path), path));

        mBaseNode->addChild(loader.loadAsSkeleton(file));

        //mObject->setVisibilityFlags (Element_Reference);

        /*
        if (mPhysics && !mReferenceId.empty())
        {
            const CSMWorld::CellRef& reference = getReference();

            // position
            Ogre::Vector3 position;
            if (!mForceBaseToZero)
                position = Ogre::Vector3(reference.mPos.pos[0], reference.mPos.pos[1], reference.mPos.pos[2]);

            // orientation
            Ogre::Quaternion xr (Ogre::Radian (-reference.mPos.rot[0]), Ogre::Vector3::UNIT_X);
            Ogre::Quaternion yr (Ogre::Radian (-reference.mPos.rot[1]), Ogre::Vector3::UNIT_Y);
            Ogre::Quaternion zr (Ogre::Radian (-reference.mPos.rot[2]), Ogre::Vector3::UNIT_Z);

            mPhysics->addObject("meshes\\" + model, mBase->getName(), mReferenceId, reference.mScale, position, xr*yr*zr);
        }
        */
    }
}

void CSVRender::Object::adjust()
{
    /*
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
    */
}

const CSMWorld::CellRef& CSVRender::Object::getReference() const
{
    if (mReferenceId.empty())
        throw std::logic_error ("object does not represent a reference");

    return mData.getReferences().getRecord (mReferenceId).get();
}

CSVRender::Object::Object (const VFS::Manager* vfs, const CSMWorld::Data& data, osg::Group* parentNode,
    const std::string& id, bool referenceable, boost::shared_ptr<CSVWorld::PhysicsSystem> physics,
    bool forceBaseToZero)
: mVFS(vfs), mData (data), mBaseNode(0), mParentNode(parentNode), mForceBaseToZero (forceBaseToZero), mPhysics(physics)
{
    mBaseNode = new osg::Group;
    parentNode->addChild(mBaseNode);

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
}

CSVRender::Object::~Object()
{
    clear();

    mParentNode->removeChild(mBaseNode);
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
