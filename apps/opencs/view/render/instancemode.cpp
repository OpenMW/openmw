
#include "instancemode.hpp"

#include <QDragEnterEvent>
#include <QPoint>
#include <QString>

#include "../../model/prefs/state.hpp"

#include <osg/ComputeBoundsVisitor>
#include <osg/Group>
#include <osg/Vec3d>
#include <osgUtil/LineSegmentIntersector>

#include "../../model/world/idtable.hpp"
#include "../../model/world/idtree.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/commandmacro.hpp"
#include "../../model/prefs/shortcut.hpp"

#include "../widget/scenetoolbar.hpp"
#include "../widget/scenetoolmode.hpp"

#include "mask.hpp"

#include "object.hpp"
#include "worldspacewidget.hpp"
#include "pagedworldspacewidget.hpp"
#include "instanceselectionmode.hpp"
#include "instancemovemode.hpp"

int CSVRender::InstanceMode::getSubModeFromId (const std::string& id) const
{
    return id=="move" ? 0 : (id=="rotate" ? 1 : 2);
}

osg::Vec3f CSVRender::InstanceMode::quatToEuler(const osg::Quat& rot) const
{
    float x, y, z;
    float test = 2 * (rot.w() * rot.y() + rot.x() * rot.z());

    if (std::abs(test) >= 1.f)
    {
        x = atan2(rot.x(), rot.w());
        y = (test > 0) ? (osg::PI / 2) : (-osg::PI / 2);
        z = 0;
    }
    else
    {
        x = std::atan2(2 * (rot.w() * rot.x() - rot.y() * rot.z()), 1 - 2 * (rot.x() * rot.x() + rot.y() * rot.y()));
        y = std::asin(test);
        z = std::atan2(2 * (rot.w() * rot.z() - rot.x() * rot.y()), 1 - 2 * (rot.y() * rot.y() + rot.z() * rot.z()));
    }

    return osg::Vec3f(-x, -y, -z);
}

osg::Quat CSVRender::InstanceMode::eulerToQuat(const osg::Vec3f& euler) const
{
    osg::Quat xr = osg::Quat(-euler[0], osg::Vec3f(1,0,0));
    osg::Quat yr = osg::Quat(-euler[1], osg::Vec3f(0,1,0));
    osg::Quat zr = osg::Quat(-euler[2], osg::Vec3f(0,0,1));

    return zr * yr * xr;
}

osg::Vec3f CSVRender::InstanceMode::getSelectionCenter(const std::vector<osg::ref_ptr<TagBase> >& selection) const
{
    osg::Vec3f center = osg::Vec3f(0, 0, 0);
    int objectCount = 0;

    for (std::vector<osg::ref_ptr<TagBase> >::const_iterator iter (selection.begin()); iter!=selection.end(); ++iter)
    {
        if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (iter->get()))
        {
            const ESM::Position& position = objectTag->mObject->getPosition();
            center += osg::Vec3f(position.pos[0], position.pos[1], position.pos[2]);

            ++objectCount;
        }
    }

    if (objectCount > 0)
        center /= objectCount;

    return center;
}

osg::Vec3f CSVRender::InstanceMode::getScreenCoords(const osg::Vec3f& pos)
{
    osg::Matrix viewMatrix = getWorldspaceWidget().getCamera()->getViewMatrix();
    osg::Matrix projMatrix = getWorldspaceWidget().getCamera()->getProjectionMatrix();
    osg::Matrix windowMatrix = getWorldspaceWidget().getCamera()->getViewport()->computeWindowMatrix();
    osg::Matrix combined = viewMatrix * projMatrix * windowMatrix;

    return pos * combined;
}

CSVRender::InstanceMode::InstanceMode (WorldspaceWidget *worldspaceWidget,  osg::ref_ptr<osg::Group> parentNode,  QWidget *parent)
: EditMode (worldspaceWidget, QIcon (":scenetoolbar/editing-instance"), Mask_Reference | Mask_Terrain, "Instance editing",
  parent), mSubMode (0), mSubModeId ("move"), mSelectionMode (0), mDragMode (DragMode_None),
  mDragAxis (-1), mLocked (false), mUnitScaleDist(1), mParentNode (parentNode)
{
    connect(this, SIGNAL(requestFocus(const std::string&)),
            worldspaceWidget, SIGNAL(requestFocus(const std::string&)));

    CSMPrefs::Shortcut* deleteShortcut = new CSMPrefs::Shortcut("scene-delete", worldspaceWidget);
    connect(deleteShortcut, SIGNAL(activated(bool)), this, SLOT(deleteSelectedInstances(bool)));

    // Following classes could be simplified by using QSignalMapper, which is obsolete in Qt5.10, but not in Qt4.8 and Qt5.14
    CSMPrefs::Shortcut* dropToCollisionShortcut = new CSMPrefs::Shortcut("scene-instance-drop-collision", worldspaceWidget);
        connect(dropToCollisionShortcut, SIGNAL(activated()), this, SLOT(dropSelectedInstancesToCollision()));
    CSMPrefs::Shortcut* dropToTerrainLevelShortcut = new CSMPrefs::Shortcut("scene-instance-drop-terrain", worldspaceWidget);
        connect(dropToTerrainLevelShortcut, SIGNAL(activated()), this, SLOT(dropSelectedInstancesToTerrain()));
    CSMPrefs::Shortcut* dropToCollisionShortcut2 = new CSMPrefs::Shortcut("scene-instance-drop-collision-separately", worldspaceWidget);
        connect(dropToCollisionShortcut2, SIGNAL(activated()), this, SLOT(dropSelectedInstancesToCollisionSeparately()));
    CSMPrefs::Shortcut* dropToTerrainLevelShortcut2 = new CSMPrefs::Shortcut("scene-instance-drop-terrain-separately", worldspaceWidget);
        connect(dropToTerrainLevelShortcut2, SIGNAL(activated()), this, SLOT(dropSelectedInstancesToTerrainSeparately()));
}

void CSVRender::InstanceMode::activate (CSVWidget::SceneToolbar *toolbar)
{
    if (!mSubMode)
    {
        mSubMode = new CSVWidget::SceneToolMode (toolbar, "Edit Sub-Mode");
        mSubMode->addButton (new InstanceMoveMode (this), "move");
        mSubMode->addButton (":scenetoolbar/transform-rotate", "rotate",
            "Rotate selected instances"
            "<ul><li>Use {scene-edit-primary} to rotate instances freely</li>"
            "<li>Use {scene-edit-secondary} to rotate instances within the grid</li>"
            "<li>The center of the view acts as the axis of rotation</li>"
            "</ul>"
            "<font color=Red>Grid rotate not implemented yet</font color>");
        mSubMode->addButton (":scenetoolbar/transform-scale", "scale",
            "Scale selected instances"
            "<ul><li>Use {scene-edit-primary} to scale instances freely</li>"
            "<li>Use {scene-edit-secondary} to scale instances along the grid</li>"
            "<li>The scaling rate is based on how close the start of a drag is to the center of the screen</li>"
            "</ul>"
            "<font color=Red>Grid scale not implemented yet</font color>");

        mSubMode->setButton (mSubModeId);

        connect (mSubMode, SIGNAL (modeChanged (const std::string&)),
            this, SLOT (subModeChanged (const std::string&)));
    }

    if (!mSelectionMode)
        mSelectionMode = new InstanceSelectionMode (toolbar, getWorldspaceWidget());

    mDragMode = DragMode_None;

    EditMode::activate (toolbar);

    toolbar->addTool (mSubMode);
    toolbar->addTool (mSelectionMode);

    std::string subMode = mSubMode->getCurrentId();

    getWorldspaceWidget().setSubMode (getSubModeFromId (subMode), Mask_Reference);
}

void CSVRender::InstanceMode::deactivate (CSVWidget::SceneToolbar *toolbar)
{
    mDragMode = DragMode_None;
    getWorldspaceWidget().reset (Mask_Reference);

    if (mSelectionMode)
    {
        toolbar->removeTool (mSelectionMode);
        delete mSelectionMode;
        mSelectionMode = 0;
    }

    if (mSubMode)
    {
        toolbar->removeTool (mSubMode);
        delete mSubMode;
        mSubMode = 0;
    }

    EditMode::deactivate (toolbar);
}

void CSVRender::InstanceMode::setEditLock (bool locked)
{
    mLocked = locked;

    if (mLocked)
        getWorldspaceWidget().abortDrag();
}

void CSVRender::InstanceMode::primaryEditPressed (const WorldspaceHitResult& hit)
{
    if (CSMPrefs::get()["3D Scene Input"]["context-select"].isTrue())
        primarySelectPressed (hit);
}

void CSVRender::InstanceMode::primaryOpenPressed (const WorldspaceHitResult& hit)
{
    if(hit.tag)
    {
        if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (hit.tag.get()))
        {
            const std::string refId = objectTag->mObject->getReferenceId();
            emit requestFocus(refId);
        }
    }
}

void CSVRender::InstanceMode::secondaryEditPressed (const WorldspaceHitResult& hit)
{
    if (CSMPrefs::get()["3D Scene Input"]["context-select"].isTrue())
        secondarySelectPressed (hit);
}

void CSVRender::InstanceMode::primarySelectPressed (const WorldspaceHitResult& hit)
{
    getWorldspaceWidget().clearSelection (Mask_Reference);

    if (hit.tag)
    {
        if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (hit.tag.get()))
        {
            // hit an Object, select it
            CSVRender::Object* object = objectTag->mObject;
            object->setSelected (true);
            return;
        }
    }
}

void CSVRender::InstanceMode::secondarySelectPressed (const WorldspaceHitResult& hit)
{
    if (hit.tag)
    {
        if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (hit.tag.get()))
        {
            // hit an Object, toggle its selection state
            CSVRender::Object* object = objectTag->mObject;
            object->setSelected (!object->getSelected());
            return;
        }
    }
}

bool CSVRender::InstanceMode::primaryEditStartDrag (const QPoint& pos)
{
    if (mDragMode!=DragMode_None || mLocked)
        return false;

    WorldspaceHitResult hit = getWorldspaceWidget().mousePick (pos, getWorldspaceWidget().getInteractionMask());

    std::vector<osg::ref_ptr<TagBase> > selection = getWorldspaceWidget().getSelection (Mask_Reference);
    if (selection.empty())
    {
        // Only change selection at the start of drag if no object is already selected
        if (hit.tag && CSMPrefs::get()["3D Scene Input"]["context-select"].isTrue())
        {
            getWorldspaceWidget().clearSelection (Mask_Reference);
            if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (hit.tag.get()))
            {
                CSVRender::Object* object = objectTag->mObject;
                object->setSelected (true);
            }
        }

        selection = getWorldspaceWidget().getSelection (Mask_Reference);
        if (selection.empty())
            return false;
    }

    for (std::vector<osg::ref_ptr<TagBase> >::iterator iter (selection.begin());
        iter!=selection.end(); ++iter)
    {
        if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (iter->get()))
        {
            if (mSubModeId == "move")
            {
                objectTag->mObject->setEdited (Object::Override_Position);
                mDragMode = DragMode_Move;
            }
            else if (mSubModeId == "rotate")
            {
                objectTag->mObject->setEdited (Object::Override_Rotation);
                mDragMode = DragMode_Rotate;
            }
            else if (mSubModeId == "scale")
            {
                objectTag->mObject->setEdited (Object::Override_Scale);
                mDragMode = DragMode_Scale;

                // Calculate scale factor
                std::vector<osg::ref_ptr<TagBase> > editedSelection = getWorldspaceWidget().getEdited (Mask_Reference);
                osg::Vec3f center = getScreenCoords(getSelectionCenter(editedSelection));

                int widgetHeight = getWorldspaceWidget().height();

                float dx = pos.x() - center.x();
                float dy = (widgetHeight - pos.y()) - center.y();

                mUnitScaleDist = std::sqrt(dx * dx + dy * dy);
            }
        }
    }

    if (CSVRender::ObjectMarkerTag *objectTag = dynamic_cast<CSVRender::ObjectMarkerTag *> (hit.tag.get()))
    {
        mDragAxis = objectTag->mAxis;
    }
    else
        mDragAxis = -1;

    return true;
}

bool CSVRender::InstanceMode::secondaryEditStartDrag (const QPoint& pos)
{
    if (mLocked)
        return false;

    return false;
}

void CSVRender::InstanceMode::drag (const QPoint& pos, int diffX, int diffY, double speedFactor)
{
    osg::Vec3f offset;
    osg::Quat rotation;

    std::vector<osg::ref_ptr<TagBase> > selection = getWorldspaceWidget().getEdited (Mask_Reference);

    if (mDragMode == DragMode_Move)
    {
        osg::Vec3f eye, centre, up;
        getWorldspaceWidget().getCamera()->getViewMatrix().getLookAt (eye, centre, up);

        if (diffY)
        {
            offset += up * diffY * speedFactor;
        }
        if (diffX)
        {
            offset += ((centre-eye) ^ up) * diffX * speedFactor;
        }

        if (mDragAxis!=-1)
        {
            for (int i=0; i<3; ++i)
            {
                if (i!=mDragAxis)
                    offset[i] = 0;
            }
        }
    }
    else if (mDragMode == DragMode_Rotate)
    {
        osg::Vec3f eye, centre, up;
        getWorldspaceWidget().getCamera()->getViewMatrix().getLookAt (eye, centre, up);

        float angle;
        osg::Vec3f axis;

        if (mDragAxis == -1)
        {
            // Free rotate
            float rotationFactor = CSMPrefs::get()["3D Scene Input"]["rotate-factor"].toDouble() * speedFactor;

            osg::Quat cameraRotation = getWorldspaceWidget().getCamera()->getInverseViewMatrix().getRotate();

            osg::Vec3f camForward = centre - eye;
            osg::Vec3f screenDir = cameraRotation * osg::Vec3f(diffX, diffY, 0);
            screenDir.normalize();

            angle = std::sqrt(diffX*diffX + diffY*diffY) * rotationFactor;
            axis = screenDir ^ camForward;
        }
        else
        {
            // Global axis rotation
            osg::Vec3f camBack = eye - centre;

            for (int i = 0; i < 3; ++i)
            {
                if (i == mDragAxis)
                    axis[i] = 1;
                else
                    axis[i] = 0;
            }

            // Flip axis if facing opposite side
            if (camBack * axis < 0)
                axis *= -1;

            // Convert coordinate system
            osg::Vec3f screenCenter = getScreenCoords(getSelectionCenter(selection));

            int widgetHeight = getWorldspaceWidget().height();

            float newX = pos.x() - screenCenter.x();
            float newY = (widgetHeight - pos.y()) - screenCenter.y();

            float oldX = newX - diffX;
            float oldY = newY - diffY; // diffY appears to already be flipped

            osg::Vec3f oldVec = osg::Vec3f(oldX, oldY, 0);
            oldVec.normalize();

            osg::Vec3f newVec = osg::Vec3f(newX, newY, 0);
            newVec.normalize();

            // Find angle and axis of rotation
            angle = std::acos(oldVec * newVec) * speedFactor;
            if (((oldVec ^ newVec) * camBack < 0) ^ (camBack.z() < 0))
                angle *= -1;
        }

        rotation = osg::Quat(angle, axis);
    }
    else if (mDragMode == DragMode_Scale)
    {
        osg::Vec3f center = getScreenCoords(getSelectionCenter(selection));

        // Calculate scaling distance/rate
        int widgetHeight = getWorldspaceWidget().height();

        float dx = pos.x() - center.x();
        float dy = (widgetHeight - pos.y()) - center.y();

        float dist = std::sqrt(dx * dx + dy * dy);
        float scale = dist / mUnitScaleDist;

        // Only uniform scaling is currently supported
        offset = osg::Vec3f(scale, scale, scale);
    }

    // Apply
    for (std::vector<osg::ref_ptr<TagBase> >::iterator iter (selection.begin()); iter!=selection.end(); ++iter)
    {
        if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (iter->get()))
        {
            if (mDragMode == DragMode_Move)
            {
                ESM::Position position = objectTag->mObject->getPosition();
                for (int i=0; i<3; ++i)
                {
                    position.pos[i] += offset[i];
                }

                objectTag->mObject->setPosition(position.pos);
            }
            else if (mDragMode == DragMode_Rotate)
            {
                ESM::Position position = objectTag->mObject->getPosition();

                osg::Quat currentRot = eulerToQuat(osg::Vec3f(position.rot[0], position.rot[1], position.rot[2]));
                osg::Quat combined = currentRot * rotation;

                osg::Vec3f euler = quatToEuler(combined);
                // There appears to be a very rare rounding error that can cause asin to return NaN
                if (!euler.isNaN())
                {
                    position.rot[0] = euler.x();
                    position.rot[1] = euler.y();
                    position.rot[2] = euler.z();
                }

                objectTag->mObject->setRotation(position.rot);
            }
            else if (mDragMode == DragMode_Scale)
            {
                // Reset scale
                objectTag->mObject->setEdited(0);
                objectTag->mObject->setEdited(Object::Override_Scale);

                float scale = objectTag->mObject->getScale();
                scale *= offset.x();

                objectTag->mObject->setScale (scale);
            }
        }
    }
}

void CSVRender::InstanceMode::dragCompleted(const QPoint& pos)
{
    std::vector<osg::ref_ptr<TagBase> > selection =
        getWorldspaceWidget().getEdited (Mask_Reference);

    QUndoStack& undoStack = getWorldspaceWidget().getDocument().getUndoStack();

    QString description;

    switch (mDragMode)
    {
        case DragMode_Move: description = "Move Instances"; break;
        case DragMode_Rotate: description = "Rotate Instances"; break;
        case DragMode_Scale: description = "Scale Instances"; break;

        case DragMode_None: break;
    }


    CSMWorld::CommandMacro macro (undoStack, description);

    for (std::vector<osg::ref_ptr<TagBase> >::iterator iter (selection.begin());
        iter!=selection.end(); ++iter)
    {
        if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (iter->get()))
        {
            objectTag->mObject->apply (macro);
        }
    }

    mDragMode = DragMode_None;
}

void CSVRender::InstanceMode::dragAborted()
{
    getWorldspaceWidget().reset (Mask_Reference);
    mDragMode = DragMode_None;
}

void CSVRender::InstanceMode::dragWheel (int diff, double speedFactor)
{
    if (mDragMode==DragMode_Move)
    {
        osg::Vec3f eye;
        osg::Vec3f centre;
        osg::Vec3f up;

        getWorldspaceWidget().getCamera()->getViewMatrix().getLookAt (eye, centre, up);

        osg::Vec3f offset = centre - eye;
        offset.normalize();
        offset *= diff * speedFactor;

        std::vector<osg::ref_ptr<TagBase> > selection =
            getWorldspaceWidget().getEdited (Mask_Reference);

        for (std::vector<osg::ref_ptr<TagBase> >::iterator iter (selection.begin());
            iter!=selection.end(); ++iter)
        {
            if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (iter->get()))
            {
                ESM::Position position = objectTag->mObject->getPosition();
                for (int i=0; i<3; ++i)
                    position.pos[i] += offset[i];
                objectTag->mObject->setPosition (position.pos);
            }
        }
    }
}

void CSVRender::InstanceMode::dragEnterEvent (QDragEnterEvent *event)
{
    if (const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData()))
    {
        if (!mime->fromDocument (getWorldspaceWidget().getDocument()))
            return;

        if (mime->holdsType (CSMWorld::UniversalId::Type_Referenceable))
            event->accept();
    }
}

void CSVRender::InstanceMode::dropEvent (QDropEvent* event)
{
    if (const CSMWorld::TableMimeData* mime = dynamic_cast<const CSMWorld::TableMimeData*> (event->mimeData()))
    {
        CSMDoc::Document& document = getWorldspaceWidget().getDocument();

        if (!mime->fromDocument (document))
            return;

        WorldspaceHitResult hit = getWorldspaceWidget().mousePick (event->pos(), getWorldspaceWidget().getInteractionMask());

        std::string cellId = getWorldspaceWidget().getCellId (hit.worldPos);

        CSMWorld::IdTree& cellTable = dynamic_cast<CSMWorld::IdTree&> (
            *document.getData().getTableModel (CSMWorld::UniversalId::Type_Cells));

        bool noCell = document.getData().getCells().searchId (cellId)==-1;

        if (noCell)
        {
            std::string mode = CSMPrefs::get()["3D Scene Editing"]["outside-drop"].toString();

            // target cell does not exist
            if (mode=="Discard")
                return;

            if (mode=="Create cell and insert")
            {
                std::unique_ptr<CSMWorld::CreateCommand> createCommand (
                    new CSMWorld::CreateCommand (cellTable, cellId));

                int parentIndex = cellTable.findColumnIndex (CSMWorld::Columns::ColumnId_Cell);
                int index = cellTable.findNestedColumnIndex (parentIndex, CSMWorld::Columns::ColumnId_Interior);
                createCommand->addNestedValue (parentIndex, index, false);

                document.getUndoStack().push (createCommand.release());

                if (CSVRender::PagedWorldspaceWidget *paged =
                    dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
                {
                    CSMWorld::CellSelection selection = paged->getCellSelection();
                    selection.add (CSMWorld::CellCoordinates::fromId (cellId).first);
                    paged->setCellSelection (selection);
                }
            }
        }
        else if (CSVRender::PagedWorldspaceWidget *paged =
            dynamic_cast<CSVRender::PagedWorldspaceWidget *> (&getWorldspaceWidget()))
        {
            CSMWorld::CellSelection selection = paged->getCellSelection();
            if (!selection.has (CSMWorld::CellCoordinates::fromId (cellId).first))
            {
                // target cell exists, but is not shown
                std::string mode =
                    CSMPrefs::get()["3D Scene Editing"]["outside-visible-drop"].toString();

                if (mode=="Discard")
                    return;

                if (mode=="Show cell and insert")
                {
                    selection.add (CSMWorld::CellCoordinates::fromId (cellId).first);
                    paged->setCellSelection (selection);
                }
            }
        }

        CSMWorld::IdTable& referencesTable = dynamic_cast<CSMWorld::IdTable&> (
            *document.getData().getTableModel (CSMWorld::UniversalId::Type_References));

        bool dropped = false;

        std::vector<CSMWorld::UniversalId> ids = mime->getData();

        for (std::vector<CSMWorld::UniversalId>::const_iterator iter (ids.begin());
            iter!=ids.end(); ++iter)
            if (mime->isReferencable (iter->getType()))
            {
                // create reference
                std::unique_ptr<CSMWorld::CreateCommand> createCommand (
                    new CSMWorld::CreateCommand (
                    referencesTable, document.getData().getReferences().getNewId()));

                 createCommand->addValue (referencesTable.findColumnIndex (
                     CSMWorld::Columns::ColumnId_Cell), QString::fromUtf8 (cellId.c_str()));
                 createCommand->addValue (referencesTable.findColumnIndex (
                     CSMWorld::Columns::ColumnId_PositionXPos), hit.worldPos.x());
                 createCommand->addValue (referencesTable.findColumnIndex (
                     CSMWorld::Columns::ColumnId_PositionYPos), hit.worldPos.y());
                 createCommand->addValue (referencesTable.findColumnIndex (
                     CSMWorld::Columns::ColumnId_PositionZPos), hit.worldPos.z());
                 createCommand->addValue (referencesTable.findColumnIndex (
                     CSMWorld::Columns::ColumnId_ReferenceableId),
                     QString::fromUtf8 (iter->getId().c_str()));

                document.getUndoStack().push (createCommand.release());

                dropped = true;
            }

        if (dropped)
            event->accept();
    }
}

int CSVRender::InstanceMode::getSubMode() const
{
    return mSubMode ? getSubModeFromId (mSubMode->getCurrentId()) : 0;
}

void CSVRender::InstanceMode::subModeChanged (const std::string& id)
{
    mSubModeId = id;
    getWorldspaceWidget().abortDrag();
    getWorldspaceWidget().setSubMode (getSubModeFromId (id), Mask_Reference);
}

void CSVRender::InstanceMode::deleteSelectedInstances(bool active)
{
    std::vector<osg::ref_ptr<TagBase> > selection = getWorldspaceWidget().getSelection (Mask_Reference);
    if (selection.empty()) return;

    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    CSMWorld::IdTable& referencesTable = dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_References));
    QUndoStack& undoStack = document.getUndoStack();

    CSMWorld::CommandMacro macro (undoStack, "Delete Instances");
    for(osg::ref_ptr<TagBase> tag: selection)
        if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (tag.get()))
            macro.push(new CSMWorld::DeleteCommand(referencesTable, objectTag->mObject->getReferenceId()));

    getWorldspaceWidget().clearSelection (Mask_Reference);
}

void CSVRender::InstanceMode::dropInstance(DropMode dropMode, CSVRender::Object* object, float objectHeight)
{
    osg::Vec3d point = object->getPosition().asVec3();

    osg::Vec3d start = point;
    start.z() += objectHeight;
    osg::Vec3d end = point;
    end.z() = std::numeric_limits<float>::lowest();

    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector (new osgUtil::LineSegmentIntersector(
        osgUtil::Intersector::MODEL, start, end) );
    intersector->setIntersectionLimit(osgUtil::LineSegmentIntersector::NO_LIMIT);
    osgUtil::IntersectionVisitor visitor(intersector);

    if (dropMode == TerrainSep)
        visitor.setTraversalMask(Mask_Terrain);
    if (dropMode == CollisionSep)
        visitor.setTraversalMask(Mask_Terrain | Mask_Reference);

    mParentNode->accept(visitor);

    osgUtil::LineSegmentIntersector::Intersections::iterator it = intersector->getIntersections().begin();
    if (it != intersector->getIntersections().end())
    {
        osgUtil::LineSegmentIntersector::Intersection intersection = *it;
        ESM::Position position = object->getPosition();
        object->setEdited (Object::Override_Position);
        position.pos[2] = intersection.getWorldIntersectPoint().z() + objectHeight;
        object->setPosition(position.pos);
    }
}

float CSVRender::InstanceMode::getDropHeight(DropMode dropMode, CSVRender::Object* object, float objectHeight)
{
    osg::Vec3d point = object->getPosition().asVec3();

    osg::Vec3d start = point;
    start.z() += objectHeight;
    osg::Vec3d end = point;
    end.z() = std::numeric_limits<float>::lowest();

    osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector (new osgUtil::LineSegmentIntersector(
        osgUtil::Intersector::MODEL, start, end) );
    intersector->setIntersectionLimit(osgUtil::LineSegmentIntersector::NO_LIMIT);
    osgUtil::IntersectionVisitor visitor(intersector);

    if (dropMode == Terrain)
        visitor.setTraversalMask(Mask_Terrain);
    if (dropMode == Collision)
        visitor.setTraversalMask(Mask_Terrain | Mask_Reference);

    mParentNode->accept(visitor);

    osgUtil::LineSegmentIntersector::Intersections::iterator it = intersector->getIntersections().begin();
    if (it != intersector->getIntersections().end())
    {
        osgUtil::LineSegmentIntersector::Intersection intersection = *it;
        float collisionLevel = intersection.getWorldIntersectPoint().z();
        return point.z() - collisionLevel + objectHeight;
    }

    return 0.0f;
}

void CSVRender::InstanceMode::dropSelectedInstancesToCollision()
{
    handleDropMethod(Collision, "Drop instances to next collision");
}

void CSVRender::InstanceMode::dropSelectedInstancesToTerrain()
{
    handleDropMethod(Terrain, "Drop instances to terrain level");
}

void CSVRender::InstanceMode::dropSelectedInstancesToCollisionSeparately()
{
    handleDropMethod(TerrainSep, "Drop instances to next collision level separately");
}

void CSVRender::InstanceMode::dropSelectedInstancesToTerrainSeparately()
{
    handleDropMethod(CollisionSep, "Drop instances to terrain level separately");
}

void CSVRender::InstanceMode::handleDropMethod(DropMode dropMode, QString commandMsg)
{
    std::vector<osg::ref_ptr<TagBase> > selection = getWorldspaceWidget().getSelection (Mask_Reference);
    if (selection.empty())
        return;

    CSMDoc::Document& document = getWorldspaceWidget().getDocument();
    QUndoStack& undoStack = document.getUndoStack();

    CSMWorld::CommandMacro macro (undoStack, commandMsg);

    DropObjectDataHandler dropObjectDataHandler(&getWorldspaceWidget());

    switch (dropMode)
    {
        case Terrain:
        case Collision:
        {
            float smallestDropHeight = std::numeric_limits<float>::max();
            int counter = 0;
                for(osg::ref_ptr<TagBase> tag: selection)
                    if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (tag.get()))
                    {
                        float thisDrop = getDropHeight(dropMode, objectTag->mObject, dropObjectDataHandler.mObjectHeights[counter]);
                        if (thisDrop < smallestDropHeight)
                            smallestDropHeight = thisDrop;
                        counter++;
                    }
                for(osg::ref_ptr<TagBase> tag: selection)
                    if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (tag.get()))
                    {
                        objectTag->mObject->setEdited (Object::Override_Position);
                        ESM::Position position = objectTag->mObject->getPosition();
                        position.pos[2] -= smallestDropHeight;
                        objectTag->mObject->setPosition(position.pos);
                        objectTag->mObject->apply (macro);
                    }
        }
            break;

        case TerrainSep:
        case CollisionSep:
        {
            int counter = 0;
            for(osg::ref_ptr<TagBase> tag: selection)
                if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (tag.get()))
                {
                    dropInstance(dropMode, objectTag->mObject, dropObjectDataHandler.mObjectHeights[counter]);
                    objectTag->mObject->apply (macro);
                    counter++;
                }
        }
            break;
    }
}

CSVRender::DropObjectDataHandler::DropObjectDataHandler(WorldspaceWidget* worldspacewidget)
    : mWorldspaceWidget(worldspacewidget)
{
    std::vector<osg::ref_ptr<TagBase> > selection = mWorldspaceWidget->getSelection (Mask_Reference);
    for(osg::ref_ptr<TagBase> tag: selection)
    {
        if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (tag.get()))
        {
            osg::ref_ptr<osg::Group> objectNodeWithGUI = objectTag->mObject->getRootNode();
            osg::ref_ptr<osg::Group> objectNodeWithoutGUI = objectTag->mObject->getBaseNode();

            osg::ComputeBoundsVisitor computeBounds;
            computeBounds.setTraversalMask(Mask_Reference);
            objectNodeWithoutGUI->accept(computeBounds);
            osg::BoundingBox bounds = computeBounds.getBoundingBox();
            float boundingBoxOffset = 0.0f;
            if (bounds.valid())
                boundingBoxOffset = bounds.zMin();

            mObjectHeights.emplace_back(boundingBoxOffset);
            mOldMasks.emplace_back(objectNodeWithGUI->getNodeMask());

            objectNodeWithGUI->setNodeMask(0);
        }
    }
}

CSVRender::DropObjectDataHandler::~DropObjectDataHandler()
{
    std::vector<osg::ref_ptr<TagBase> > selection = mWorldspaceWidget->getSelection (Mask_Reference);
    int counter = 0;
    for(osg::ref_ptr<TagBase> tag: selection)
    {
        if (CSVRender::ObjectTag *objectTag = dynamic_cast<CSVRender::ObjectTag *> (tag.get()))
        {
            osg::ref_ptr<osg::Group> objectNodeWithGUI = objectTag->mObject->getRootNode();
            objectNodeWithGUI->setNodeMask(mOldMasks[counter]);
            counter++;
        }
    }
}
