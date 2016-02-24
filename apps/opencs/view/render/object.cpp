#include "object.hpp"

#include <stdexcept>
#include <iostream>

#include <osg/Group>
#include <osg/PositionAttitudeTransform>

#include <osg/ShapeDrawable>
#include <osg/Shape>
#include <osg/Geode>

#include <osgFX/Scribe>

#include "../../model/world/data.hpp"
#include "../../model/world/ref.hpp"
#include "../../model/world/refidcollection.hpp"

#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/lightutil.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/fallback/fallback.hpp>

#include "mask.hpp"

namespace
{

    osg::ref_ptr<osg::Geode> createErrorCube()
    {
        osg::ref_ptr<osg::Box> shape(new osg::Box(osg::Vec3f(0,0,0), 50.f));
        osg::ref_ptr<osg::ShapeDrawable> shapedrawable(new osg::ShapeDrawable);
        shapedrawable->setShape(shape);

        osg::ref_ptr<osg::Geode> geode (new osg::Geode);
        geode->addDrawable(shapedrawable);
        return geode;
    }

}


CSVRender::ObjectTag::ObjectTag (Object* object)
: TagBase (Mask_Reference), mObject (object)
{}

QString CSVRender::ObjectTag::getToolTip (bool hideBasics) const
{
    return QString::fromUtf8 (mObject->getReferenceableId().c_str());
}


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
    const ESM::Light* light = NULL;

    if (index==-1)
        error = 1;
    else
    {
        /// \todo check for Deleted state (error 1)

        model = referenceables.getData (index,
            referenceables.findColumnIndex (CSMWorld::Columns::ColumnId_Model)).
            toString().toUtf8().constData();

        int recordType =
                referenceables.getData (index,
                referenceables.findColumnIndex(CSMWorld::Columns::ColumnId_RecordType)).toInt();
        if (recordType == CSMWorld::UniversalId::Type_Light)
        {
            light = &dynamic_cast<const CSMWorld::Record<ESM::Light>& >(referenceables.getRecord(index)).get();
        }

        if (model.empty())
            error = 2;
    }

    mBaseNode->removeChildren(0, mBaseNode->getNumChildren());

    if (error)
    {
        mBaseNode->addChild(createErrorCube());
    }
    else
    {
        try
        {
            std::string path = "meshes\\" + model;

            mResourceSystem->getSceneManager()->getInstance(path, mBaseNode);
        }
        catch (std::exception& e)
        {
            // TODO: use error marker mesh
            std::cerr << e.what() << std::endl;
        }
    }

    if (light)
    {
        const Fallback::Map* fallback = mData.getFallbackMap();
        static bool outQuadInLin = fallback->getFallbackBool("LightAttenuation_OutQuadInLin");
        static bool useQuadratic = fallback->getFallbackBool("LightAttenuation_UseQuadratic");
        static float quadraticValue = fallback->getFallbackFloat("LightAttenuation_QuadraticValue");
        static float quadraticRadiusMult = fallback->getFallbackFloat("LightAttenuation_QuadraticRadiusMult");
        static bool useLinear = fallback->getFallbackBool("LightAttenuation_UseLinear");
        static float linearRadiusMult = fallback->getFallbackFloat("LightAttenuation_LinearRadiusMult");
        static float linearValue = fallback->getFallbackFloat("LightAttenuation_LinearValue");
        bool isExterior = false; // FIXME
        SceneUtil::addLight(mBaseNode, light, Mask_ParticleSystem, Mask_Lighting, isExterior, outQuadInLin, useQuadratic,
                            quadraticValue, quadraticRadiusMult, useLinear, linearRadiusMult, linearValue);
    }
}

void CSVRender::Object::adjustTransform()
{
    if (mReferenceId.empty())
        return;

    const CSMWorld::CellRef& reference = getReference();

    // position
    mBaseNode->setPosition(mForceBaseToZero ? osg::Vec3() : osg::Vec3f(reference.mPos.pos[0], reference.mPos.pos[1], reference.mPos.pos[2]));

    // orientation
    osg::Quat xr (-reference.mPos.rot[0], osg::Vec3f(1,0,0));
    osg::Quat yr (-reference.mPos.rot[1], osg::Vec3f(0,1,0));
    osg::Quat zr (-reference.mPos.rot[2], osg::Vec3f(0,0,1));
    mBaseNode->setAttitude(zr*yr*xr);

    mBaseNode->setScale(osg::Vec3(reference.mScale, reference.mScale, reference.mScale));
}

const CSMWorld::CellRef& CSVRender::Object::getReference() const
{
    if (mReferenceId.empty())
        throw std::logic_error ("object does not represent a reference");

    return mData.getReferences().getRecord (mReferenceId).get();
}

CSVRender::Object::Object (CSMWorld::Data& data, osg::Group* parentNode,
    const std::string& id, bool referenceable, bool forceBaseToZero)
: mData (data), mBaseNode(0), mSelected(false), mParentNode(parentNode), mResourceSystem(data.getResourceSystem().get()), mForceBaseToZero (forceBaseToZero)
{
    mBaseNode = new osg::PositionAttitudeTransform;
    mBaseNode->addCullCallback(new SceneUtil::LightListCallback);

    mOutline = new osgFX::Scribe;

    mBaseNode->setUserData(new ObjectTag(this));

    parentNode->addChild(mBaseNode);

    mBaseNode->setNodeMask(Mask_Reference);

    if (referenceable)
    {
        mReferenceableId = id;
    }
    else
    {
        mReferenceId = id;
        mReferenceableId = getReference().mRefID;
    }

    adjustTransform();
    update();
}

CSVRender::Object::~Object()
{
    clear();

    mParentNode->removeChild(mBaseNode);
    mParentNode->removeChild(mOutline);
}

void CSVRender::Object::setSelected(bool selected)
{
    mSelected = selected;

    mOutline->removeChild(mBaseNode);
    mParentNode->removeChild(mOutline);
    mParentNode->removeChild(mBaseNode);
    if (selected)
    {
        mOutline->addChild(mBaseNode);
        mParentNode->addChild(mOutline);
    }
    else
        mParentNode->addChild(mBaseNode);
}

bool CSVRender::Object::getSelected() const
{
    return mSelected;
}

bool CSVRender::Object::referenceableDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    const CSMWorld::RefIdCollection& referenceables = mData.getReferenceables();

    int index = referenceables.searchId (mReferenceableId);

    if (index!=-1 && index>=topLeft.row() && index<=bottomRight.row())
    {
        adjustTransform();
        update();
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
            adjustTransform();
            update();
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

        adjustTransform();

        if (columnIndex>=topLeft.column() && columnIndex<=bottomRight.row())
        {
            mReferenceableId =
                references.getData (index, columnIndex).toString().toUtf8().constData();

            update();
        }

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

osg::ref_ptr<CSVRender::TagBase> CSVRender::Object::getTag() const
{
    return static_cast<CSVRender::TagBase *> (mBaseNode->getUserData());
}
