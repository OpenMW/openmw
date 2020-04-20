#include "object.hpp"

#include <stdexcept>
#include <string>
#include <iostream>

#include <osg/Depth>
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
#include "../../model/prefs/state.hpp"

#include <components/debug/debuglog.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/lightutil.hpp>
#include <components/sceneutil/lightmanager.hpp>

#include "actor.hpp"
#include "mask.hpp"


const float CSVRender::Object::MarkerShaftWidth = 30;
const float CSVRender::Object::MarkerShaftBaseLength = 70;
const float CSVRender::Object::MarkerHeadWidth = 50;
const float CSVRender::Object::MarkerHeadLength = 50;


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

    const CSMWorld::RefIdCollection& referenceables = mData.getReferenceables();
    const int TypeIndex = referenceables.findColumnIndex(CSMWorld::Columns::ColumnId_RecordType);
    const int ModelIndex = referenceables.findColumnIndex (CSMWorld::Columns::ColumnId_Model);

    int index = referenceables.searchId (mReferenceableId);
    const ESM::Light* light = nullptr;

    mBaseNode->removeChildren(0, mBaseNode->getNumChildren());

    if (index == -1)
    {
        mBaseNode->addChild(createErrorCube());
        return;
    }

    /// \todo check for Deleted state (error 1)

    int recordType = referenceables.getData(index, TypeIndex).toInt();
    std::string model = referenceables.getData(index, ModelIndex).toString().toUtf8().constData();

    if (recordType == CSMWorld::UniversalId::Type_Light)
    {
        light = &dynamic_cast<const CSMWorld::Record<ESM::Light>& >(referenceables.getRecord(index)).get();
        if (model.empty())
            model = "marker_light.nif";
    }

    if (recordType == CSMWorld::UniversalId::Type_CreatureLevelledList)
    {
        if (model.empty())
            model = "marker_creature.nif";
    }

    try
    {
        if (recordType == CSMWorld::UniversalId::Type_Npc || recordType == CSMWorld::UniversalId::Type_Creature)
        {
            if (!mActor) mActor.reset(new Actor(mReferenceableId, mData));
            mActor->update();
            mBaseNode->addChild(mActor->getBaseNode());
        }
        else if (!model.empty())
        {
            std::string path = "meshes\\" + model;
            mResourceSystem->getSceneManager()->getInstance(path, mBaseNode);
        }
        else
        {
            throw std::runtime_error(mReferenceableId + " has no model");
        }
    }
    catch (std::exception& e)
    {
        // TODO: use error marker mesh
        Log(Debug::Error) << e.what();
    }

    if (light)
    {
        bool isExterior = false; // FIXME
        SceneUtil::addLight(mBaseNode, light, Mask_ParticleSystem, Mask_Lighting, isExterior);
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
                mMarker[i] = makeMoveOrScaleMarker (i);
                mMarker[i]->setUserData(new ObjectMarkerTag (this, i));

                mRootNode->addChild (mMarker[i]);
            }
            else if (mSubMode==1)
            {
                mMarker[i] = makeRotateMarker (i);
                mMarker[i]->setUserData(new ObjectMarkerTag (this, i));

                mRootNode->addChild (mMarker[i]);
            }
            else if (mSubMode==2)
            {
                mMarker[i] = makeMoveOrScaleMarker (i);
                mMarker[i]->setUserData(new ObjectMarkerTag (this, i));

                mRootNode->addChild (mMarker[i]);
            }
        }
    }
}

osg::ref_ptr<osg::Node> CSVRender::Object::makeMoveOrScaleMarker (int axis)
{
    osg::ref_ptr<osg::Geometry> geometry (new osg::Geometry);

    float shaftLength = MarkerShaftBaseLength + mBaseNode->getBound().radius();

    // shaft
    osg::Vec3Array *vertices = new osg::Vec3Array;

    for (int i=0; i<2; ++i)
    {
        float length = i ? shaftLength : MarkerShaftWidth;

        vertices->push_back (getMarkerPosition (-MarkerShaftWidth/2, -MarkerShaftWidth/2, length, axis));
        vertices->push_back (getMarkerPosition (-MarkerShaftWidth/2, MarkerShaftWidth/2, length, axis));
        vertices->push_back (getMarkerPosition (MarkerShaftWidth/2, MarkerShaftWidth/2, length, axis));
        vertices->push_back (getMarkerPosition (MarkerShaftWidth/2, -MarkerShaftWidth/2, length, axis));
    }

    // head backside
    vertices->push_back (getMarkerPosition (-MarkerHeadWidth/2, -MarkerHeadWidth/2, shaftLength, axis));
    vertices->push_back (getMarkerPosition (-MarkerHeadWidth/2, MarkerHeadWidth/2, shaftLength, axis));
    vertices->push_back (getMarkerPosition (MarkerHeadWidth/2, MarkerHeadWidth/2, shaftLength, axis));
    vertices->push_back (getMarkerPosition (MarkerHeadWidth/2, -MarkerHeadWidth/2, shaftLength, axis));

    // head
    vertices->push_back (getMarkerPosition (0, 0, shaftLength+MarkerHeadLength, axis));

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
            axis==2 ? 1.0f : 0.2f, mMarkerTransparency));

    for (int i=8; i<8+4+1; ++i)
        colours->push_back (osg::Vec4f (axis==0 ? 1.0f : 0.0f, axis==1 ? 1.0f : 0.0f,
            axis==2 ? 1.0f : 0.0f, mMarkerTransparency));

    geometry->setColorArray (colours, osg::Array::BIND_PER_VERTEX);

    setupCommonMarkerState(geometry);

    osg::ref_ptr<osg::Geode> geode (new osg::Geode);
    geode->addDrawable (geometry);

    return geode;
}

osg::ref_ptr<osg::Node> CSVRender::Object::makeRotateMarker (int axis)
{
    const float InnerRadius = std::max(MarkerShaftBaseLength, mBaseNode->getBound().radius());
    const float OuterRadius = InnerRadius + MarkerShaftWidth;

    const float SegmentDistance = 100.f;
    const size_t SegmentCount = std::min(64, std::max(24, (int)(OuterRadius * 2 * osg::PI / SegmentDistance)));
    const size_t VerticesPerSegment = 4;
    const size_t IndicesPerSegment = 24;

    const size_t VertexCount = SegmentCount * VerticesPerSegment;
    const size_t IndexCount = SegmentCount * IndicesPerSegment;

    const float Angle = 2 * osg::PI / SegmentCount;

    const unsigned short IndexPattern[IndicesPerSegment] =
    {
        0, 4, 5, 0, 5, 1,
        2, 6, 4, 2, 4, 0,
        3, 7, 6, 3, 6, 2,
        1, 5, 7, 1, 7, 3
    };


    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();

    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array(VertexCount);
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array(1);
    osg::ref_ptr<osg::DrawElementsUShort> primitives = new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLES,
        IndexCount);

    // prevent some depth collision issues from overlaps
    osg::Vec3f offset = getMarkerPosition(0, MarkerShaftWidth/4, 0, axis);

    for (size_t i = 0; i < SegmentCount; ++i)
    {
        size_t index = i * VerticesPerSegment;

        float innerX = InnerRadius * std::cos(i * Angle);
        float innerY = InnerRadius * std::sin(i * Angle);

        float outerX = OuterRadius * std::cos(i * Angle);
        float outerY = OuterRadius * std::sin(i * Angle);

        vertices->at(index++) = getMarkerPosition(innerX, innerY,  MarkerShaftWidth / 2, axis) + offset;
        vertices->at(index++) = getMarkerPosition(innerX, innerY, -MarkerShaftWidth / 2, axis) + offset;
        vertices->at(index++) = getMarkerPosition(outerX, outerY,  MarkerShaftWidth / 2, axis) + offset;
        vertices->at(index++) = getMarkerPosition(outerX, outerY, -MarkerShaftWidth / 2, axis) + offset;
    }

    colors->at(0) = osg::Vec4f (
        axis==0 ? 1.0f : 0.2f,
        axis==1 ? 1.0f : 0.2f,
        axis==2 ? 1.0f : 0.2f,
        mMarkerTransparency);

    for (size_t i = 0; i < SegmentCount; ++i)
    {
        size_t indices[IndicesPerSegment];
        for (size_t j = 0; j < IndicesPerSegment; ++j)
        {
            indices[j] = i * VerticesPerSegment + j;

            if (indices[j] >= VertexCount)
                indices[j] -= VertexCount;
        }

        size_t elementOffset = i * IndicesPerSegment;
        for (size_t j = 0; j < IndicesPerSegment; ++j)
        {
            primitives->setElement(elementOffset++, indices[IndexPattern[j]]);
        }
    }

    geometry->setVertexArray(vertices);
    geometry->setColorArray(colors, osg::Array::BIND_OVERALL);
    geometry->addPrimitiveSet(primitives);

    setupCommonMarkerState(geometry);

    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->addDrawable (geometry);

    return geode;
}

void CSVRender::Object::setupCommonMarkerState(osg::ref_ptr<osg::Geometry> geometry)
{
    osg::ref_ptr<osg::StateSet> state = geometry->getOrCreateStateSet();
    state->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    state->setMode(GL_BLEND, osg::StateAttribute::ON);

    state->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
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
  mScaleOverride (1), mOverrideFlags (0), mSubMode (-1), mMarkerTransparency(0.5f)
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

    mMarkerTransparency = CSMPrefs::get()["Rendering"]["object-marker-alpha"].toDouble();
    updateMarker();
}

bool CSVRender::Object::getSelected() const
{
    return mSelected;
}

osg::ref_ptr<osg::Group> CSVRender::Object::getRootNode()
{
    return mRootNode;
}

osg::ref_ptr<osg::Group> CSVRender::Object::getBaseNode()
{
    return mBaseNode;
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

void CSVRender::Object::reloadAssets()
{
    update();
    updateMarker();
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
    return (mOverrideFlags & Override_Scale) ? mScaleOverride : getReference().mScale;
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

void CSVRender::Object::setMarkerTransparency(float value)
{
    mMarkerTransparency = value;
    updateMarker();
}

void CSVRender::Object::apply (CSMWorld::CommandMacro& commands)
{
    const CSMWorld::RefCollection& collection = mData.getReferences();
    QAbstractItemModel *model = mData.getTableModel (CSMWorld::UniversalId::Type_References);

    int recordIndex = collection.getIndex (mReferenceId);

    if (mOverrideFlags & Override_Position)
    {
        //Do cell check first so positions can be compared
        const CSMWorld::CellRef& ref = collection.getRecord(recordIndex).get();

        if (CSMWorld::CellCoordinates::isExteriorCell(ref.mCell))
        {
            // Find cell index at new position
            std::pair<int, int> cellIndex = CSMWorld::CellCoordinates::coordinatesToCellIndex(
                mPositionOverride.pos[0], mPositionOverride.pos[1]);
            std::pair<int, int> originalIndex = ref.getCellIndex();

            int cellColumn = collection.findColumnIndex (static_cast<CSMWorld::Columns::ColumnId> (
                CSMWorld::Columns::ColumnId_Cell));
            int refNumColumn = collection.findColumnIndex (static_cast<CSMWorld::Columns::ColumnId> (
                CSMWorld::Columns::ColumnId_RefNum));

            if (cellIndex != originalIndex)
            {
                /// \todo figure out worldspace (not important until multiple worldspaces are supported)
                std::string cellId = CSMWorld::CellCoordinates (cellIndex).getId ("");

                commands.push (new CSMWorld::ModifyCommand (*model,
                    model->index (recordIndex, cellColumn), QString::fromUtf8 (cellId.c_str())));
                commands.push (new CSMWorld::ModifyCommand( *model,
                    model->index (recordIndex, refNumColumn), 0));
            }
        }

        for (int i=0; i<3; ++i)
        {
            int column = collection.findColumnIndex (static_cast<CSMWorld::Columns::ColumnId> (
                CSMWorld::Columns::ColumnId_PositionXPos+i));

            commands.push (new CSMWorld::ModifyCommand (*model,
                model->index (recordIndex, column), mPositionOverride.pos[i]));
        }
    }

    if (mOverrideFlags & Override_Rotation)
    {
        for (int i=0; i<3; ++i)
        {
            int column = collection.findColumnIndex (static_cast<CSMWorld::Columns::ColumnId> (
                CSMWorld::Columns::ColumnId_PositionXRot+i));

            commands.push (new CSMWorld::ModifyCommand (*model,
                model->index (recordIndex, column), osg::RadiansToDegrees(mPositionOverride.rot[i])));
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
