#include "pathgrid.hpp"

#include <algorithm>

#include <osg/Array>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>
#include <osg/PositionAttitudeTransform>
#include <osg/Vec3>

#include <components/sceneutil/pathgridutil.hpp>

#include "../../model/world/cell.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/commandmacro.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/idtree.hpp"

namespace CSVRender
{
    class PathgridNodeCallback : public osg::NodeCallback
    {
        public:

            virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
            {
                PathgridTag* tag = static_cast<PathgridTag*>(node->getUserData());
                tag->getPathgrid()->update();
            }
    };

    PathgridTag::PathgridTag(Pathgrid* pathgrid)
        : TagBase(Mask_Pathgrid), mPathgrid(pathgrid)
    {
    }

    Pathgrid* PathgridTag::getPathgrid() const
    {
        return mPathgrid;
    }

    QString PathgridTag::getToolTip(bool hideBasics) const
    {
        QString text("Pathgrid: ");
        text += mPathgrid->getId().c_str();

        return text;
    }

    Pathgrid::Pathgrid(CSMWorld::Data& data, osg::Group* parent, const std::string& pathgridId,
        const CSMWorld::CellCoordinates& coordinates)
        : mData(data)
        , mPathgridCollection(mData.getPathgrids())
        , mId(pathgridId)
        , mCoords(coordinates)
        , mInterior(false)
        , mDragOrigin(0)
        , mChangeGeometry(true)
        , mRemoveGeometry(false)
        , mUseOffset(true)
        , mParent(parent)
        , mPathgridGeometry(0)
        , mDragGeometry(0)
        , mTag(new PathgridTag(this))
    {
        const float CoordScalar = ESM::Land::REAL_SIZE;

        mBaseNode = new osg::PositionAttitudeTransform ();
        mBaseNode->setPosition(osg::Vec3f(mCoords.getX() * CoordScalar, mCoords.getY() * CoordScalar, 0.f));
        mBaseNode->setUserData(mTag);
        mBaseNode->setUpdateCallback(new PathgridNodeCallback());
        mBaseNode->setNodeMask(Mask_Pathgrid);
        mParent->addChild(mBaseNode);

        mPathgridGeode = new osg::Geode();
        mBaseNode->addChild(mPathgridGeode);

        recreateGeometry();

        int index = mData.getCells().searchId(mId);
        if (index != -1)
        {
            const CSMWorld::Cell& cell = mData.getCells().getRecord(index).get();
            mInterior = cell.mData.mFlags & ESM::Cell::Interior;
        }
    }

    Pathgrid::~Pathgrid()
    {
        mParent->removeChild(mBaseNode);
    }

    const CSMWorld::CellCoordinates& Pathgrid::getCoordinates() const
    {
        return mCoords;
    }

    const std::string& Pathgrid::getId() const
    {
        return mId;
    }

    bool Pathgrid::isSelected() const
    {
        return !mSelected.empty();
    }

    const Pathgrid::NodeList& Pathgrid::getSelected() const
    {
        return mSelected;
    }

    void Pathgrid::selectAll()
    {
        mSelected.clear();

        const CSMWorld::Pathgrid* source = getPathgridSource();
        if (source)
        {
            for (unsigned short i = 0; i < static_cast<unsigned short>(source->mPoints.size()); ++i)
                mSelected.push_back(i);

            createSelectedGeometry(*source);
        }
        else
        {
            removeSelectedGeometry();
        }
    }

    void Pathgrid::toggleSelected(unsigned short node)
    {
        NodeList::iterator searchResult = std::find(mSelected.begin(), mSelected.end(), node);
        if (searchResult != mSelected.end())
        {
            mSelected.erase(searchResult);
        }
        else
        {
            mSelected.push_back(node);
        }

        createSelectedGeometry();
    }

    void Pathgrid::invertSelected()
    {
        NodeList temp = NodeList(mSelected);
        mSelected.clear();

        const CSMWorld::Pathgrid* source = getPathgridSource();
        if (source)
        {
            for (unsigned short i = 0; i < static_cast<unsigned short>(source->mPoints.size()); ++i)
            {
                if (std::find(temp.begin(), temp.end(), i) == temp.end())
                    mSelected.push_back(i);
            }

            createSelectedGeometry(*source);
        }
        else
        {
            removeSelectedGeometry();
        }
    }

    void Pathgrid::clearSelected()
    {
        mSelected.clear();
        removeSelectedGeometry();
    }

    void Pathgrid::moveSelected(const osg::Vec3d& offset)
    {
        mUseOffset = true;
        mMoveOffset += offset;

        recreateGeometry();
    }

    void Pathgrid::setDragOrigin(unsigned short node)
    {
        mDragOrigin = node;
    }

    void Pathgrid::setDragEndpoint(unsigned short node)
    {
        const CSMWorld::Pathgrid* source = getPathgridSource();
        if (source)
        {
            const CSMWorld::Pathgrid::Point& pointA = source->mPoints[mDragOrigin];
            const CSMWorld::Pathgrid::Point& pointB = source->mPoints[node];

            osg::Vec3f start = osg::Vec3f(pointA.mX, pointA.mY, pointA.mZ + SceneUtil::DiamondHalfHeight);
            osg::Vec3f end = osg::Vec3f(pointB.mX, pointB.mY, pointB.mZ + SceneUtil::DiamondHalfHeight);

            createDragGeometry(start, end, true);
        }
    }

    void Pathgrid::setDragEndpoint(const osg::Vec3d& pos)
    {
        const CSMWorld::Pathgrid* source = getPathgridSource();
        if (source)
        {
            const CSMWorld::Pathgrid::Point& point = source->mPoints[mDragOrigin];

            osg::Vec3f start = osg::Vec3f(point.mX, point.mY, point.mZ + SceneUtil::DiamondHalfHeight);
            osg::Vec3f end = pos - mBaseNode->getPosition();
            createDragGeometry(start, end, false);
        }
    }

    void Pathgrid::resetIndicators()
    {
        mUseOffset = false;
        mMoveOffset.set(0, 0, 0);

        mPathgridGeode->removeDrawable(mDragGeometry);
        mDragGeometry = 0;
    }

    void Pathgrid::applyPoint(CSMWorld::CommandMacro& commands, const osg::Vec3d& worldPos)
    {
        CSMWorld::IdTree* model = &dynamic_cast<CSMWorld::IdTree&>(*mData.getTableModel(CSMWorld::UniversalId::Type_Pathgrids));

        const CSMWorld::Pathgrid* source = getPathgridSource();
        if (source)
        {
            osg::Vec3d localCoords = worldPos - mBaseNode->getPosition();

            int posX = clampToCell(static_cast<int>(localCoords.x()));
            int posY = clampToCell(static_cast<int>(localCoords.y()));
            int posZ = clampToCell(static_cast<int>(localCoords.z()));

            int recordIndex = mPathgridCollection.getIndex (mId);
            int parentColumn = mPathgridCollection.findColumnIndex(CSMWorld::Columns::ColumnId_PathgridPoints);

            int posXColumn = mPathgridCollection.searchNestedColumnIndex(parentColumn,
                CSMWorld::Columns::ColumnId_PathgridPosX);

            int posYColumn = mPathgridCollection.searchNestedColumnIndex(parentColumn,
                CSMWorld::Columns::ColumnId_PathgridPosY);

            int posZColumn = mPathgridCollection.searchNestedColumnIndex(parentColumn,
                CSMWorld::Columns::ColumnId_PathgridPosZ);

            QModelIndex parent = model->index(recordIndex, parentColumn);
            int row = static_cast<int>(source->mPoints.size());

            // Add node to end of list
            commands.push(new CSMWorld::AddNestedCommand(*model, mId, row, parentColumn));
            commands.push(new CSMWorld::ModifyCommand(*model, model->index(row, posXColumn, parent), posX));
            commands.push(new CSMWorld::ModifyCommand(*model, model->index(row, posYColumn, parent), posY));
            commands.push(new CSMWorld::ModifyCommand(*model, model->index(row, posZColumn, parent), posZ));
        }
        else
        {
            int index = mPathgridCollection.searchId(mId);
            if (index == -1)
            {
                // Does not exist
                commands.push(new CSMWorld::CreatePathgridCommand(*model, mId));
            }
            else
            {
                source = &mPathgridCollection.getRecord(index).get();

                // Deleted, so revert and remove all data
                commands.push(new CSMWorld::RevertCommand(*model, mId));

                int parentColumn = mPathgridCollection.findColumnIndex(CSMWorld::Columns::ColumnId_PathgridPoints);
                for (int row = source->mPoints.size() - 1; row >= 0; --row)
                {
                    commands.push(new CSMWorld::DeleteNestedCommand(*model, mId, row, parentColumn));
                }

                parentColumn = mPathgridCollection.findColumnIndex(CSMWorld::Columns::ColumnId_PathgridEdges);
                for (int row = source->mEdges.size() - 1; row >= 0; --row)
                {
                    commands.push(new CSMWorld::DeleteNestedCommand(*model, mId, row, parentColumn));
                }
            }
        }
    }

    void Pathgrid::applyPosition(CSMWorld::CommandMacro& commands)
    {
        const CSMWorld::Pathgrid* source = getPathgridSource();
        if (source)
        {
            osg::Vec3d localCoords = mMoveOffset;

            int offsetX = static_cast<int>(localCoords.x());
            int offsetY = static_cast<int>(localCoords.y());
            int offsetZ = static_cast<int>(localCoords.z());

            QAbstractItemModel* model = mData.getTableModel(CSMWorld::UniversalId::Type_Pathgrids);

            int recordIndex = mPathgridCollection.getIndex(mId);
            int parentColumn = mPathgridCollection.findColumnIndex(CSMWorld::Columns::ColumnId_PathgridPoints);

            int posXColumn = mPathgridCollection.searchNestedColumnIndex(parentColumn,
                CSMWorld::Columns::ColumnId_PathgridPosX);

            int posYColumn = mPathgridCollection.searchNestedColumnIndex(parentColumn,
                CSMWorld::Columns::ColumnId_PathgridPosY);

            int posZColumn = mPathgridCollection.searchNestedColumnIndex(parentColumn,
                CSMWorld::Columns::ColumnId_PathgridPosZ);

            QModelIndex parent = model->index(recordIndex, parentColumn);

            for (size_t i = 0; i < mSelected.size(); ++i)
            {
                const CSMWorld::Pathgrid::Point& point = source->mPoints[mSelected[i]];
                int row = static_cast<int>(mSelected[i]);

                commands.push(new CSMWorld::ModifyCommand(*model, model->index(row, posXColumn, parent),
                    clampToCell(point.mX + offsetX)));

                commands.push(new CSMWorld::ModifyCommand(*model, model->index(row, posYColumn, parent),
                    clampToCell(point.mY + offsetY)));

                commands.push(new CSMWorld::ModifyCommand(*model, model->index(row, posZColumn, parent),
                    clampToCell(point.mZ + offsetZ)));
            }
        }
    }

    void Pathgrid::applyEdge(CSMWorld::CommandMacro& commands, unsigned short node1, unsigned short node2)
    {
        const CSMWorld::Pathgrid* source = getPathgridSource();
        if (source)
        {
            addEdge(commands, *source, node1, node2);
        }
    }

    void Pathgrid::applyEdges(CSMWorld::CommandMacro& commands, unsigned short node)
    {
        const CSMWorld::Pathgrid* source = getPathgridSource();
        if (source)
        {
            for (size_t i = 0; i < mSelected.size(); ++i)
            {
                addEdge(commands, *source, node, mSelected[i]);
            }
        }
    }

    void Pathgrid::applyRemoveNodes(CSMWorld::CommandMacro& commands)
    {
        const CSMWorld::Pathgrid* source = getPathgridSource();
        if (source)
        {
            CSMWorld::IdTree* model = &dynamic_cast<CSMWorld::IdTree&>(*mData.getTableModel(CSMWorld::UniversalId::Type_Pathgrids));

            // Want to remove nodes from end of list first
            std::sort(mSelected.begin(), mSelected.end(), std::greater<int>());

            int recordIndex = mPathgridCollection.getIndex(mId);
            int parentColumn = mPathgridCollection.findColumnIndex(CSMWorld::Columns::ColumnId_PathgridPoints);

            for (std::vector<unsigned short>::iterator row = mSelected.begin(); row != mSelected.end(); ++row)
            {
                commands.push(new CSMWorld::DeleteNestedCommand(*model, mId, static_cast<int>(*row), parentColumn));
            }

            // Fix/remove edges
            std::set<int, std::greater<int> > edgeRowsToRemove;

            parentColumn = mPathgridCollection.findColumnIndex(CSMWorld::Columns::ColumnId_PathgridEdges);

            int edge0Column = mPathgridCollection.searchNestedColumnIndex(parentColumn,
                CSMWorld::Columns::ColumnId_PathgridEdge0);

            int edge1Column = mPathgridCollection.searchNestedColumnIndex(parentColumn,
                CSMWorld::Columns::ColumnId_PathgridEdge1);

            QModelIndex parent = model->index(recordIndex, parentColumn);

            for (size_t edge = 0; edge < source->mEdges.size(); ++edge)
            {
                int adjustment0 = 0;
                int adjustment1 = 0;

                // Determine necessary adjustment
                for (std::vector<unsigned short>::iterator point = mSelected.begin(); point != mSelected.end(); ++point)
                {
                    if (source->mEdges[edge].mV0 == *point || source->mEdges[edge].mV1 == *point)
                    {
                        edgeRowsToRemove.insert(static_cast<int>(edge));

                        adjustment0 = 0; // No need to adjust, its getting removed
                        adjustment1 = 0;
                        break;
                    }

                    if (source->mEdges[edge].mV0 > *point)
                        --adjustment0;

                    if (source->mEdges[edge].mV1 > *point)
                        --adjustment1;
                }

                if (adjustment0 != 0)
                {
                    int adjustedEdge = source->mEdges[edge].mV0 + adjustment0;
                    commands.push(new CSMWorld::ModifyCommand(*model, model->index(edge, edge0Column, parent),
                        adjustedEdge));
                }

                if (adjustment1 != 0)
                {
                    int adjustedEdge = source->mEdges[edge].mV1 + adjustment1;
                    commands.push(new CSMWorld::ModifyCommand(*model, model->index(edge, edge1Column, parent),
                        adjustedEdge));
                }
            }

            std::set<int, std::greater<int> >::iterator row;
            for (row = edgeRowsToRemove.begin(); row != edgeRowsToRemove.end(); ++row)
            {
                commands.push(new CSMWorld::DeleteNestedCommand(*model, mId, *row, parentColumn));
            }
        }

        clearSelected();
    }

    void Pathgrid::applyRemoveEdges(CSMWorld::CommandMacro& commands)
    {
        const CSMWorld::Pathgrid* source = getPathgridSource();
        if (source)
        {
            // Want to remove from end of row first
            std::set<int, std::greater<int> > rowsToRemove;
            for (size_t i = 0; i <= mSelected.size(); ++i)
            {
                for (size_t j = i + 1; j < mSelected.size(); ++j)
                {
                    int row = edgeExists(*source, mSelected[i], mSelected[j]);
                    if (row != -1)
                    {
                        rowsToRemove.insert(row);
                    }

                    row = edgeExists(*source, mSelected[j], mSelected[i]);
                    if (row != -1)
                    {
                        rowsToRemove.insert(row);
                    }
                }
            }

            CSMWorld::IdTree* model = &dynamic_cast<CSMWorld::IdTree&>(*mData.getTableModel(CSMWorld::UniversalId::Type_Pathgrids));
            int parentColumn = mPathgridCollection.findColumnIndex(CSMWorld::Columns::ColumnId_PathgridEdges);

            std::set<int, std::greater<int> >::iterator row;
            for (row = rowsToRemove.begin(); row != rowsToRemove.end(); ++row)
            {
                commands.push(new CSMWorld::DeleteNestedCommand(*model, mId, *row, parentColumn));
            }
        }
    }

    osg::ref_ptr<PathgridTag> Pathgrid::getTag() const
    {
        return mTag;
    }

    void Pathgrid::recreateGeometry()
    {
        mChangeGeometry = true;
    }

    void Pathgrid::removeGeometry()
    {
        mRemoveGeometry = true;
    }

    void Pathgrid::update()
    {
        if (mRemoveGeometry)
        {
            removePathgridGeometry();
            removeSelectedGeometry();
        }
        else if (mChangeGeometry)
        {
            createGeometry();
        }

        mChangeGeometry = false;
        mRemoveGeometry = false;
    }

    void Pathgrid::createGeometry()
    {
        const CSMWorld::Pathgrid* source = getPathgridSource();
        if (source)
        {
            CSMWorld::Pathgrid temp;
            if (mUseOffset)
            {
                temp = *source;

                for (NodeList::iterator it = mSelected.begin(); it != mSelected.end(); ++it)
                {
                    temp.mPoints[*it].mX += mMoveOffset.x();
                    temp.mPoints[*it].mY += mMoveOffset.y();
                    temp.mPoints[*it].mZ += mMoveOffset.z();
                }

                source = &temp;
            }

            removePathgridGeometry();
            mPathgridGeometry = SceneUtil::createPathgridGeometry(*source);
            mPathgridGeode->addDrawable(mPathgridGeometry);

            createSelectedGeometry(*source);
        }
        else
        {
            removePathgridGeometry();
            removeSelectedGeometry();
        }
    }

    void Pathgrid::createSelectedGeometry()
    {
        const CSMWorld::Pathgrid* source = getPathgridSource();
        if (source)
        {
            createSelectedGeometry(*source);
        }
        else
        {
            removeSelectedGeometry();
        }
    }

    void Pathgrid::createSelectedGeometry(const CSMWorld::Pathgrid& source)
    {
        removeSelectedGeometry();

        mSelectedGeometry = SceneUtil::createPathgridSelectedWireframe(source, mSelected);
        mPathgridGeode->addDrawable(mSelectedGeometry);
    }

    void Pathgrid::removePathgridGeometry()
    {
        if (mPathgridGeometry)
        {
            mPathgridGeode->removeDrawable(mPathgridGeometry);
            mPathgridGeometry = 0;
        }
    }

    void Pathgrid::removeSelectedGeometry()
    {
        if (mSelectedGeometry)
        {
            mPathgridGeode->removeDrawable(mSelectedGeometry);
            mSelectedGeometry = 0;
        }
    }

    void Pathgrid::createDragGeometry(const osg::Vec3f& start, const osg::Vec3f& end, bool valid)
    {
        if (mDragGeometry)
            mPathgridGeode->removeDrawable(mDragGeometry);

        mDragGeometry = new osg::Geometry();

        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array(2);
        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array(1);
        osg::ref_ptr<osg::DrawElementsUShort> indices = new osg::DrawElementsUShort(osg::PrimitiveSet::LINES, 2);

        (*vertices)[0] = start;
        (*vertices)[1] = end;

        if (valid)
        {
            (*colors)[0] = osg::Vec4f(0.91f, 0.66f, 1.f, 1.f);
        }
        else
        {
            (*colors)[0] = osg::Vec4f(1.f, 0.f, 0.f, 1.f);
        }

        indices->setElement(0, 0);
        indices->setElement(1, 1);

        mDragGeometry->setVertexArray(vertices);
        mDragGeometry->setColorArray(colors, osg::Array::BIND_OVERALL);
        mDragGeometry->addPrimitiveSet(indices);
        mDragGeometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

        mPathgridGeode->addDrawable(mDragGeometry);
    }

    const CSMWorld::Pathgrid* Pathgrid::getPathgridSource()
    {
        int index = mPathgridCollection.searchId(mId);
        if (index != -1 && !mPathgridCollection.getRecord(index).isDeleted())
        {
            return &mPathgridCollection.getRecord(index).get();
        }

        return 0;
    }

    int Pathgrid::edgeExists(const CSMWorld::Pathgrid& source, unsigned short node1, unsigned short node2)
    {
        for (size_t i = 0; i < source.mEdges.size(); ++i)
        {
            if (source.mEdges[i].mV0 == node1 && source.mEdges[i].mV1 == node2)
                return static_cast<int>(i);
        }

        return -1;
    }

    void Pathgrid::addEdge(CSMWorld::CommandMacro& commands, const CSMWorld::Pathgrid& source, unsigned short node1,
        unsigned short node2)
    {
        CSMWorld::IdTree* model = &dynamic_cast<CSMWorld::IdTree&>(*mData.getTableModel(CSMWorld::UniversalId::Type_Pathgrids));

        int recordIndex = mPathgridCollection.getIndex(mId);
        int parentColumn = mPathgridCollection.findColumnIndex(CSMWorld::Columns::ColumnId_PathgridEdges);

        int edge0Column = mPathgridCollection.searchNestedColumnIndex(parentColumn,
            CSMWorld::Columns::ColumnId_PathgridEdge0);

        int edge1Column = mPathgridCollection.searchNestedColumnIndex(parentColumn,
            CSMWorld::Columns::ColumnId_PathgridEdge1);

        QModelIndex parent = model->index(recordIndex, parentColumn);
        int row = static_cast<int>(source.mEdges.size());

        if (edgeExists(source, node1, node2) == -1)
        {
            commands.push(new CSMWorld::AddNestedCommand(*model, mId, row, parentColumn));
            commands.push(new CSMWorld::ModifyCommand(*model, model->index(row, edge0Column, parent), node1));
            commands.push(new CSMWorld::ModifyCommand(*model, model->index(row, edge1Column, parent), node2));
            ++row;
        }

        if (edgeExists(source, node2, node1) == -1)
        {
            commands.push(new CSMWorld::AddNestedCommand(*model, mId, row, parentColumn));
            commands.push(new CSMWorld::ModifyCommand(*model, model->index(row, edge0Column, parent), node2));
            commands.push(new CSMWorld::ModifyCommand(*model, model->index(row, edge1Column, parent), node1));
        }
    }

    int Pathgrid::clampToCell(int v)
    {
        const int CellExtent = ESM::Land::REAL_SIZE;

        if (mInterior)
            return v;
        else if (v > CellExtent)
            return CellExtent;
        else if (v < 0)
            return 0;
        else
            return v;
    }
}
