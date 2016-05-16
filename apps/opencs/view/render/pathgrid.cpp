#include "pathgrid.hpp"

#include <algorithm>

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>
#include <osg/PositionAttitudeTransform>
#include <osg/Vec3>

#include <components/sceneutil/pathgridutil.hpp>

#include "../../model/world/commands.hpp"
#include "../../model/world/commandmacro.hpp"
#include "../../model/world/data.hpp"

namespace CSVRender
{
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

        if (!hideBasics)
        {
        }

        return text;
    }

    Pathgrid::Pathgrid(CSMWorld::Data& data, osg::Group* parent, const std::string& pathgridId,
        const CSMWorld::CellCoordinates& coordinates)
        : mData(data)
        , mPathgridCollection(mData.getPathgrids())
        , mId(pathgridId)
        , mCoords(coordinates)
        , mParent(parent)
        , mPathgridGeometry(0)
        , mSelectedGeometry(0)
        , mTag(new PathgridTag(this))
    {
        const float CoordScalar = ESM::Land::REAL_SIZE;

        mBaseNode = new osg::PositionAttitudeTransform ();
        mBaseNode->setPosition(osg::Vec3f(mCoords.getX() * CoordScalar, mCoords.getY() * CoordScalar, 0.f));
        mBaseNode->setUserData(mTag);
        mBaseNode->setNodeMask(Mask_Pathgrid);
        mParent->addChild(mBaseNode);

        mSelectedNode = new osg::PositionAttitudeTransform();
        mBaseNode->addChild(mSelectedNode);

        mPathgridGeode = new osg::Geode();
        mBaseNode->addChild(mPathgridGeode);

        mSelectedGeode = new osg::Geode();
        mSelectedNode->addChild(mSelectedGeode);

        recreateGeometry();
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

            recreateSelectedGeometry(*source);
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

        recreateSelectedGeometry();
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

            recreateSelectedGeometry(*source);
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
        mSelectedNode->setPosition(mSelectedNode->getPosition() + offset);
    }

    void Pathgrid::resetMove()
    {
        mSelectedNode->setPosition(osg::Vec3f(0,0,0));
    }

    void Pathgrid::applyPosition(CSMWorld::CommandMacro& commands)
    {
        const CSMWorld::Pathgrid* source = getPathgridSource();
        if (source)
        {
            osg::Vec3d localCoords = mSelectedNode->getPosition();

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
                    point.mX + offsetX));

                commands.push(new CSMWorld::ModifyCommand(*model, model->index(row, posYColumn, parent),
                    point.mY + offsetY));

                commands.push(new CSMWorld::ModifyCommand(*model, model->index(row, posZColumn, parent),
                    point.mZ + offsetZ));
            }
        }

        resetMove();
    }

    void Pathgrid::applyEdge(CSMWorld::CommandMacro& commands, unsigned short node1, unsigned short node2)
    {
        // TODO
    }

    void Pathgrid::applyEdges(CSMWorld::CommandMacro& commands, unsigned short node)
    {
        // TODO
    }

    osg::ref_ptr<PathgridTag> Pathgrid::getTag() const
    {
        return mTag;
    }

    void Pathgrid::recreateGeometry()
    {
        // Make new
        const CSMWorld::Pathgrid* source = getPathgridSource();
        if (source)
        {
            removePathgridGeometry();
            mPathgridGeometry = SceneUtil::createPathgridGeometry(*source);
            mPathgridGeode->addDrawable(mPathgridGeometry);

            recreateSelectedGeometry(*source);
        }
        else
        {
            removePathgridGeometry();
            removeSelectedGeometry();
        }
    }

    void Pathgrid::recreateSelectedGeometry()
    {
        const CSMWorld::Pathgrid* source = getPathgridSource();
        if (source)
        {
            recreateSelectedGeometry(*source);
        }
        else
        {
            removeSelectedGeometry();
        }
    }

    void Pathgrid::recreateSelectedGeometry(const CSMWorld::Pathgrid& source)
    {
        removeSelectedGeometry();
        mSelectedGeometry = SceneUtil::createPathgridSelectedWireframe(source, mSelected);
        mSelectedGeode->addDrawable(mSelectedGeometry);
    }

    void Pathgrid::removePathgridGeometry()
    {
        if (mPathgridGeometry)
        {
            mPathgridGeode->removeDrawable(mPathgridGeometry);
            mPathgridGeometry.release();
        }
    }

    void Pathgrid::removeSelectedGeometry()
    {
        if (mSelectedGeometry)
        {
            mSelectedGeode->removeDrawable(mSelectedGeometry);
            mSelectedGeometry.release();
        }
    }

    const CSMWorld::Pathgrid* Pathgrid::getPathgridSource()
    {
        int index = mPathgridCollection.searchId(mId);
        if (index != -1)
        {
            return &mPathgridCollection.getRecord(index).get();
        }

        return 0;
    }
}
