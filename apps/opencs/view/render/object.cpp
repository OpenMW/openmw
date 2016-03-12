#include "object.hpp"

#include <stdexcept>
#include <iostream>

#include <osg/Group>
#include <osg/PositionAttitudeTransform>

#include <osg/ShapeDrawable>
#include <osg/Shape>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PrimitiveSet>

#include <osgFX/Scribe>

#include "../../model/world/data.hpp"
#include "../../model/world/ref.hpp"
#include "../../model/world/refidcollection.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/universalid.hpp"
#include "../../model/world/commandmacro.hpp"
#include "../../model/world/cellcoordinates.hpp"

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


CSVRender::ObjectMarkerTag::ObjectMarkerTag (Object* object, int axis)
: ObjectTag (object), mAxis (axis)
{}


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

    ESM::Position position = getPosition();

    // position
    mRootNode->setPosition(mForceBaseToZero ? osg::Vec3() : osg::Vec3f(position.pos[0], position.pos[1], position.pos[2]));

    // orientation
    osg::Quat xr (-position.rot[0], osg::Vec3f(1,0,0));
    osg::Quat yr (-position.rot[1], osg::Vec3f(0,1,0));
    osg::Quat zr (-position.rot[2], osg::Vec3f(0,0,1));
    mBaseNode->setAttitude(zr*yr*xr);

    float scale = getScale();

    mBaseNode->setScale(osg::Vec3(scale, scale, scale));
}

const CSMWorld::CellRef& CSVRender::Object::getReference() const
{
    if (mReferenceId.empty())
        throw std::logic_error ("object does not represent a reference");

    return mData.getReferences().getRecord (mReferenceId).get();
}

void CSVRender::Object::updateMarker()
{
    for (int i=0; i<3; ++i)
    {
        if (mMarker[i])
        {
            mRootNode->removeChild (mMarker[i]);
            mMarker[i] = osg::ref_ptr<osg::Node>();
        }

        if (mSelected)
        {
            if (mSubMode==0)
            {
                mMarker[i] = makeMarker (i);
                mMarker[i]->setUserData(new ObjectMarkerTag (this, i));

                mRootNode->addChild (mMarker[i]);
            }
        }
    }
}

osg::ref_ptr<osg::Node> CSVRender::Object::makeMarker (int axis)
{
    osg::ref_ptr<osg::Geometry> geometry (new osg::Geometry);

    const float shaftWidth = 10;
    const float shaftBaseLength = 50;
    const float headWidth = 30;
    const float headLength = 30;

    float shaftLength = shaftBaseLength + mBaseNode->getBound().radius();

    // shaft
    osg::Vec3Array *vertices = new osg::Vec3Array;

    for (int i=0; i<2; ++i)
    {
        float length = i ? shaftLength : 0;

        vertices->push_back (getMarkerPosition (-shaftWidth/2, -shaftWidth/2, length, axis));
        vertices->push_back (getMarkerPosition (-shaftWidth/2, shaftWidth/2, length, axis));
        vertices->push_back (getMarkerPosition (shaftWidth/2, shaftWidth/2, length, axis));
        vertices->push_back (getMarkerPosition (shaftWidth/2, -shaftWidth/2, length, axis));
    }

    // head backside
    vertices->push_back (getMarkerPosition (-headWidth/2, -headWidth/2, shaftLength, axis));
    vertices->push_back (getMarkerPosition (-headWidth/2, headWidth/2, shaftLength, axis));
    vertices->push_back (getMarkerPosition (headWidth/2, headWidth/2, shaftLength, axis));
    vertices->push_back (getMarkerPosition (headWidth/2, -headWidth/2, shaftLength, axis));

    // head
    vertices->push_back (getMarkerPosition (0, 0, shaftLength+headLength, axis));

    geometry->setVertexArray (vertices);

    osg::DrawElementsUShort *primitives = new osg::DrawElementsUShort (osg::PrimitiveSet::TRIANGLES, 0);

    // shaft
    for (int i=0; i<4; ++i)
    {
        int i2 = i==3 ? 0 : i+1;
        primitives->push_back (i);
        primitives->push_back (4+i);
        primitives->push_back (i2);

        primitives->push_back (4+i);
        primitives->push_back (4+i2);
        primitives->push_back (i2);
    }

    // cap
    primitives->push_back (0);
    primitives->push_back (1);
    primitives->push_back (2);

    primitives->push_back (2);
    primitives->push_back (3);
    primitives->push_back (0);

    // head, backside
    primitives->push_back (0+8);
    primitives->push_back (1+8);
    primitives->push_back (2+8);

    primitives->push_back (2+8);
    primitives->push_back (3+8);
    primitives->push_back (0+8);

    for (int i=0; i<4; ++i)
    {
        primitives->push_back (12);
        primitives->push_back (8+(i==3 ? 0 : i+1));
        primitives->push_back (8+i);
    }

    geometry->addPrimitiveSet (primitives);

    osg::Vec4Array *colours = new osg::Vec4Array;

    for (int i=0; i<8; ++i)
        colours->push_back (osg::Vec4f (axis==0 ? 1.0f : 0.2f, axis==1 ? 1.0f : 0.2f,
            axis==2 ? 1.0f : 0.2f, 1.0f));

    for (int i=8; i<8+4+1; ++i)
        colours->push_back (osg::Vec4f (axis==0 ? 1.0f : 0.0f, axis==1 ? 1.0f : 0.0f,
            axis==2 ? 1.0f : 0.0f, 1.0f));

    geometry->setColorArray (colours, osg::Array::BIND_PER_VERTEX);

    geometry->getOrCreateStateSet()->setMode (GL_LIGHTING, osg::StateAttribute::OFF);

    osg::ref_ptr<osg::Geode> geode (new osg::Geode);
    geode->addDrawable (geometry);

    return geode;
}

osg::Vec3f CSVRender::Object::getMarkerPosition (float x, float y, float z, int axis)
{
    switch (axis)
    {
        case 2: return osg::Vec3f (x, y, z);
        case 0: return osg::Vec3f (z, x, y);
        case 1: return osg::Vec3f (y, z, x);

        default:

            throw std::logic_error ("invalid axis for marker geometry");
    }
}

CSVRender::Object::Object (CSMWorld::Data& data, osg::Group* parentNode,
    const std::string& id, bool referenceable, bool forceBaseToZero)
: mData (data), mBaseNode(0), mSelected(false), mParentNode(parentNode), mResourceSystem(data.getResourceSystem().get()), mForceBaseToZero (forceBaseToZero),
  mScaleOverride (1), mOverrideFlags (0), mSubMode (-1)
{
    mRootNode = new osg::PositionAttitudeTransform;

    mBaseNode = new osg::PositionAttitudeTransform;
    mBaseNode->addCullCallback(new SceneUtil::LightListCallback);

    mOutline = new osgFX::Scribe;

    mBaseNode->setUserData(new ObjectTag(this));

    mRootNode->addChild (mBaseNode);

    parentNode->addChild (mRootNode);

    mRootNode->setNodeMask(Mask_Reference);

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
    updateMarker();
}

CSVRender::Object::~Object()
{
    clear();

    mParentNode->removeChild (mRootNode);
}

void CSVRender::Object::setSelected(bool selected)
{
    mSelected = selected;

    mOutline->removeChild(mBaseNode);
    mRootNode->removeChild(mOutline);
    mRootNode->removeChild(mBaseNode);
    if (selected)
    {
        mOutline->addChild(mBaseNode);
        mRootNode->addChild(mOutline);
    }
    else
        mRootNode->addChild(mBaseNode);

    updateMarker();
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
        updateMarker();
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
            updateMarker();
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

bool CSVRender::Object::isEdited() const
{
    return mOverrideFlags;
}

void CSVRender::Object::setEdited (int flags)
{
    bool discard = mOverrideFlags & ~flags;
    int added = flags & ~mOverrideFlags;

    mOverrideFlags = flags;

    if (added & Override_Position)
        for (int i=0; i<3; ++i)
            mPositionOverride.pos[i] = getReference().mPos.pos[i];

    if (added & Override_Rotation)
        for (int i=0; i<3; ++i)
            mPositionOverride.rot[i] = getReference().mPos.rot[i];

    if (added & Override_Scale)
        mScaleOverride = getReference().mScale;

    if (discard)
        adjustTransform();
}

ESM::Position CSVRender::Object::getPosition() const
{
    ESM::Position position = getReference().mPos;

    if (mOverrideFlags & Override_Position)
        for (int i=0; i<3; ++i)
            position.pos[i] = mPositionOverride.pos[i];

    if (mOverrideFlags & Override_Rotation)
        for (int i=0; i<3; ++i)
            position.rot[i] = mPositionOverride.rot[i];

    return position;
}

float CSVRender::Object::getScale() const
{
    return mOverrideFlags & Override_Scale ? mScaleOverride : getReference().mScale;
}

void CSVRender::Object::setPosition (const float position[3])
{
    mOverrideFlags |= Override_Position;

    for (int i=0; i<3; ++i)
        mPositionOverride.pos[i] = position[i];

    adjustTransform();
}

void CSVRender::Object::setRotation (const float rotation[3])
{
    mOverrideFlags |= Override_Rotation;

    for (int i=0; i<3; ++i)
        mPositionOverride.rot[i] = rotation[i];

    adjustTransform();
}

void CSVRender::Object::setScale (float scale)
{
    mOverrideFlags |= Override_Scale;

    mScaleOverride = scale;

    adjustTransform();
}

void CSVRender::Object::apply (CSMWorld::CommandMacro& commands)
{
    const CSMWorld::RefCollection& collection = mData.getReferences();
    QAbstractItemModel *model = mData.getTableModel (CSMWorld::UniversalId::Type_References);

    int recordIndex = collection.getIndex (mReferenceId);

    if (mOverrideFlags & Override_Position)
    {
        for (int i=0; i<3; ++i)
        {
            int column = collection.findColumnIndex (static_cast<CSMWorld::Columns::ColumnId> (
                CSMWorld::Columns::ColumnId_PositionXPos+i));

            commands.push (new CSMWorld::ModifyCommand (*model,
                model->index (recordIndex, column), mPositionOverride.pos[i]));
        }

        int column = collection.findColumnIndex (static_cast<CSMWorld::Columns::ColumnId> (
            CSMWorld::Columns::ColumnId_Cell));

        std::pair<int, int> cellIndex = collection.getRecord (recordIndex).get().getCellIndex();

        /// \todo figure out worldspace (not important until multiple worldspaces are supported)
        std::string cellId = CSMWorld::CellCoordinates (cellIndex).getId ("");

        commands.push (new CSMWorld::ModifyCommand (*model,
            model->index (recordIndex, column), QString::fromUtf8 (cellId.c_str())));
    }

    if (mOverrideFlags & Override_Rotation)
    {
        for (int i=0; i<3; ++i)
        {
            int column = collection.findColumnIndex (static_cast<CSMWorld::Columns::ColumnId> (
                CSMWorld::Columns::ColumnId_PositionXRot+i));

            commands.push (new CSMWorld::ModifyCommand (*model,
                model->index (recordIndex, column), mPositionOverride.rot[i]));
        }
    }

    if (mOverrideFlags & Override_Scale)
    {
        int column = collection.findColumnIndex (CSMWorld::Columns::ColumnId_Scale);

        commands.push (new CSMWorld::ModifyCommand (*model,
            model->index (recordIndex, column), mScaleOverride));
    }

    mOverrideFlags = 0;
}

void CSVRender::Object::setSubMode (int subMode)
{
    if (subMode!=mSubMode)
    {
        mSubMode = subMode;
        updateMarker();
    }
}

void CSVRender::Object::reset()
{
    mOverrideFlags = 0;
    adjustTransform();
    updateMarker();
}
