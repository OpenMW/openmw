#include "object.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <stddef.h>
#include <stdexcept>
#include <string>
#include <utility>

#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/world/columns.hpp>
#include <apps/opencs/model/world/record.hpp>
#include <apps/opencs/model/world/ref.hpp>
#include <apps/opencs/model/world/refcollection.hpp>
#include <apps/opencs/model/world/refidcollection.hpp>
#include <apps/opencs/model/world/universalid.hpp>
#include <apps/opencs/view/render/tagbase.hpp>

#include <osg/Quat>
#include <osg/ShapeDrawable>

#include <osgFX/Scribe>

#include "../../model/world/cellcoordinates.hpp"
#include "../../model/world/commandmacro.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/data.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm/esmbridge.hpp>
#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/lightcommon.hpp>
#include <components/sceneutil/lightmanager.hpp>
#include <components/sceneutil/lightutil.hpp>

#include "actor.hpp"
#include "mask.hpp"

namespace CSVRender
{
    struct WorldspaceHitResult;
}

namespace ESM
{
    struct Light;
}

namespace
{

    osg::ref_ptr<osg::Group> createErrorCube()
    {
        osg::ref_ptr<osg::Box> shape(new osg::Box(osg::Vec3f(0, 0, 0), 50.f));
        osg::ref_ptr<osg::ShapeDrawable> shapedrawable(new osg::ShapeDrawable);
        shapedrawable->setShape(shape);

        osg::ref_ptr<osg::Group> group(new osg::Group);
        group->addChild(shapedrawable);
        return group;
    }

}

CSVRender::ObjectTag::ObjectTag(Object* object)
    : TagBase(Mask_Reference)
    , mObject(object)
{
}

QString CSVRender::ObjectTag::getToolTip(bool /*hideBasics*/, const WorldspaceHitResult& /*hit*/) const
{
    return QString::fromUtf8(mObject->getReferenceableId().c_str());
}

void CSVRender::Object::clear() {}

void CSVRender::Object::update()
{
    clear();

    const CSMWorld::RefIdCollection& referenceables = mData.getReferenceables();
    const int TypeIndex = referenceables.findColumnIndex(CSMWorld::Columns::ColumnId_RecordType);
    const int ModelIndex = referenceables.findColumnIndex(CSMWorld::Columns::ColumnId_Model);

    int index = referenceables.searchId(mReferenceableId);
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
        light = &dynamic_cast<const CSMWorld::Record<ESM::Light>&>(referenceables.getRecord(index)).get();
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
            if (!mActor)
                mActor = std::make_unique<Actor>(mReferenceableId, mData);
            mActor->update();
            mBaseNode->addChild(mActor->getBaseNode());
        }
        else if (!model.empty())
        {
            constexpr VFS::Path::NormalizedView meshes("meshes");
            VFS::Path::Normalized path(meshes);
            path /= model;
            mResourceSystem->getSceneManager()->getInstance(path, mBaseNode);
        }
        else
        {
            throw std::runtime_error(mReferenceableId.getRefIdString() + " has no model");
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
        SceneUtil::addLight(mBaseNode, SceneUtil::LightCommon(*light), Mask_Lighting, isExterior);
    }
}

void CSVRender::Object::adjustTransform()
{
    if (mReferenceId.empty())
        return;

    ESM::Position position = getPosition();

    // position
    mRootNode->setPosition(
        mForceBaseToZero ? osg::Vec3() : osg::Vec3f(position.pos[0], position.pos[1], position.pos[2]));

    // orientation
    osg::Quat xr(-position.rot[0], osg::Vec3f(1, 0, 0));
    osg::Quat yr(-position.rot[1], osg::Vec3f(0, 1, 0));
    osg::Quat zr(-position.rot[2], osg::Vec3f(0, 0, 1));
    mBaseNode->setAttitude(zr * yr * xr);

    float scale = getScale();

    mBaseNode->setScale(osg::Vec3(scale, scale, scale));
}

const CSMWorld::CellRef& CSVRender::Object::getReference() const
{
    if (mReferenceId.empty())
        throw std::logic_error("object does not represent a reference");

    return mData.getReferences().getRecord(mReferenceId).get();
}

CSVRender::Object::Object(
    CSMWorld::Data& data, osg::Group* parentNode, const std::string& id, bool referenceable, bool forceBaseToZero)
    : mData(data)
    , mBaseNode(nullptr)
    , mSelected(false)
    , mParentNode(parentNode)
    , mResourceSystem(data.getResourceSystem().get())
    , mForceBaseToZero(forceBaseToZero)
    , mScaleOverride(1)
    , mOverrideFlags(0)
{
    mRootNode = new osg::PositionAttitudeTransform;

    mBaseNode = new osg::PositionAttitudeTransform;
    mBaseNode->addCullCallback(new SceneUtil::LightListCallback);

    mOutline = new osgFX::Scribe;

    mBaseNode->setUserData(new ObjectTag(this));

    mRootNode->addChild(mBaseNode);

    parentNode->addChild(mRootNode);

    mRootNode->setNodeMask(Mask_Reference);
    ESM::RefId refId = ESM::RefId::stringRefId(id);
    if (referenceable)
    {
        mReferenceableId = refId;
    }
    else
    {
        mReferenceId = refId;
        mReferenceableId = getReference().mRefID;
    }

    adjustTransform();
    update();
}

CSVRender::Object::~Object()
{
    clear();

    mParentNode->removeChild(mRootNode);
}

void CSVRender::Object::setSelected(bool selected, const osg::Vec4f& color)
{
    mSelected = selected;

    if (mSnapTarget)
    {
        setSnapTarget(false);
    }

    mOutline->removeChild(mBaseNode);
    mRootNode->removeChild(mOutline);
    mRootNode->removeChild(mBaseNode);
    if (selected)
    {
        mOutline->setWireframeColor(color);
        mOutline->addChild(mBaseNode);
        mRootNode->addChild(mOutline);
    }
    else
        mRootNode->addChild(mBaseNode);
}

bool CSVRender::Object::getSelected() const
{
    return mSelected;
}

void CSVRender::Object::setSnapTarget(bool isSnapTarget)
{
    mSnapTarget = isSnapTarget;

    if (mSelected)
    {
        setSelected(false);
    }

    mOutline->removeChild(mBaseNode);
    mRootNode->removeChild(mOutline);
    mRootNode->removeChild(mBaseNode);
    if (isSnapTarget)
    {
        mOutline->setWireframeColor(osg::Vec4f(1, 1, 0, 1));
        mOutline->addChild(mBaseNode);
        mRootNode->addChild(mOutline);
    }
    else
        mRootNode->addChild(mBaseNode);
}

bool CSVRender::Object::getSnapTarget() const
{
    return mSnapTarget;
}

osg::ref_ptr<osg::Group> CSVRender::Object::getRootNode()
{
    return mRootNode;
}

osg::ref_ptr<osg::Group> CSVRender::Object::getBaseNode()
{
    return mBaseNode;
}

bool CSVRender::Object::referenceableDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    const CSMWorld::RefIdCollection& referenceables = mData.getReferenceables();

    int index = referenceables.searchId(mReferenceableId);

    if (index != -1 && index >= topLeft.row() && index <= bottomRight.row())
    {
        adjustTransform();
        update();
        return true;
    }

    return false;
}

bool CSVRender::Object::referenceableAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    const CSMWorld::RefIdCollection& referenceables = mData.getReferenceables();

    int index = referenceables.searchId(mReferenceableId);

    if (index != -1 && index >= start && index <= end)
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

bool CSVRender::Object::referenceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    if (mReferenceId.empty())
        return false;

    const CSMWorld::RefCollection& references = mData.getReferences();

    int index = references.searchId(mReferenceId);

    if (index != -1 && index >= topLeft.row() && index <= bottomRight.row())
    {
        int columnIndex = references.findColumnIndex(CSMWorld::Columns::ColumnId_ReferenceableId);

        adjustTransform();

        if (columnIndex >= topLeft.column() && columnIndex <= bottomRight.row())
        {
            mReferenceableId
                = ESM::RefId::stringRefId(references.getData(index, columnIndex).toString().toUtf8().constData());

            update();
        }

        return true;
    }

    return false;
}

void CSVRender::Object::reloadAssets()
{
    update();
}

std::string CSVRender::Object::getReferenceId() const
{
    return mReferenceId.getRefIdString();
}

std::string CSVRender::Object::getReferenceableId() const
{
    return mReferenceableId.getRefIdString();
}

osg::ref_ptr<CSVRender::TagBase> CSVRender::Object::getTag() const
{
    return static_cast<CSVRender::TagBase*>(mBaseNode->getUserData());
}

bool CSVRender::Object::isEdited() const
{
    return mOverrideFlags;
}

void CSVRender::Object::setEdited(int flags)
{
    bool discard = mOverrideFlags & ~flags;
    int added = flags & ~mOverrideFlags;

    mOverrideFlags = flags;

    if (added & Override_Position)
        for (int i = 0; i < 3; ++i)
            mPositionOverride.pos[i] = getReference().mPos.pos[i];

    if (added & Override_Rotation)
        for (int i = 0; i < 3; ++i)
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
        for (int i = 0; i < 3; ++i)
            position.pos[i] = mPositionOverride.pos[i];

    if (mOverrideFlags & Override_Rotation)
        for (int i = 0; i < 3; ++i)
            position.rot[i] = mPositionOverride.rot[i];

    return position;
}

float CSVRender::Object::getScale() const
{
    return (mOverrideFlags & Override_Scale) ? mScaleOverride : getReference().mScale;
}

void CSVRender::Object::setPosition(const float position[3])
{
    mOverrideFlags |= Override_Position;

    for (int i = 0; i < 3; ++i)
        mPositionOverride.pos[i] = position[i];

    adjustTransform();
}

void CSVRender::Object::setRotation(const float rotation[3])
{
    mOverrideFlags |= Override_Rotation;

    for (int i = 0; i < 3; ++i)
        mPositionOverride.rot[i] = rotation[i];

    adjustTransform();
}

void CSVRender::Object::setScale(float scale)
{
    mOverrideFlags |= Override_Scale;

    mScaleOverride = std::clamp(scale, 0.5f, 2.0f);

    adjustTransform();
}

void CSVRender::Object::apply(CSMWorld::CommandMacro& commands)
{
    const CSMWorld::RefCollection& collection = mData.getReferences();
    QAbstractItemModel* model = mData.getTableModel(CSMWorld::UniversalId::Type_References);

    int recordIndex = collection.getIndex(mReferenceId);

    if (mOverrideFlags & Override_Position)
    {
        // Do cell check first so positions can be compared
        const CSMWorld::CellRef& ref = collection.getRecord(recordIndex).get();

        if (CSMWorld::CellCoordinates::isExteriorCell(ref.mCell))
        {
            // Find cell index at new position
            std::pair<int, int> cellIndex
                = CSMWorld::CellCoordinates::coordinatesToCellIndex(mPositionOverride.pos[0], mPositionOverride.pos[1]);
            std::pair<int, int> originalIndex = ref.getCellIndex();

            int cellColumn = collection.findColumnIndex(
                static_cast<CSMWorld::Columns::ColumnId>(CSMWorld::Columns::ColumnId_Cell));
            int origCellColumn = collection.findColumnIndex(
                static_cast<CSMWorld::Columns::ColumnId>(CSMWorld::Columns::ColumnId_OriginalCell));

            if (cellIndex != originalIndex)
            {
                /// \todo figure out worldspace (not important until multiple worldspaces are supported)
                std::string origCellId = CSMWorld::CellCoordinates(originalIndex).getId("");
                std::string cellId = CSMWorld::CellCoordinates(cellIndex).getId("");

                commands.push(new CSMWorld::ModifyCommand(
                    *model, model->index(recordIndex, origCellColumn), QString::fromUtf8(origCellId.c_str())));
                commands.push(new CSMWorld::ModifyCommand(
                    *model, model->index(recordIndex, cellColumn), QString::fromUtf8(cellId.c_str())));
                // NOTE: refnum is not modified for moving a reference to another cell
            }
        }

        for (int i = 0; i < 3; ++i)
        {
            int column = collection.findColumnIndex(
                static_cast<CSMWorld::Columns::ColumnId>(CSMWorld::Columns::ColumnId_PositionXPos + i));

            commands.push(
                new CSMWorld::ModifyCommand(*model, model->index(recordIndex, column), mPositionOverride.pos[i]));
        }
    }

    if (mOverrideFlags & Override_Rotation)
    {
        for (int i = 0; i < 3; ++i)
        {
            int column = collection.findColumnIndex(
                static_cast<CSMWorld::Columns::ColumnId>(CSMWorld::Columns::ColumnId_PositionXRot + i));

            commands.push(new CSMWorld::ModifyCommand(
                *model, model->index(recordIndex, column), osg::RadiansToDegrees(mPositionOverride.rot[i])));
        }
    }

    if (mOverrideFlags & Override_Scale)
    {
        int column = collection.findColumnIndex(CSMWorld::Columns::ColumnId_Scale);

        commands.push(new CSMWorld::ModifyCommand(*model, model->index(recordIndex, column), mScaleOverride));
    }

    mOverrideFlags = 0;
}

void CSVRender::Object::reset()
{
    mOverrideFlags = 0;
    adjustTransform();
}
