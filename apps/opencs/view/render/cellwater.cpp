#include "cellwater.hpp"

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>
#include <osg/PolygonOffset>
#include <osg/PositionAttitudeTransform>

#include <components/esm/loadland.hpp>
#include <components/misc/stringops.hpp>

#include "../../model/world/cell.hpp"
#include "../../model/world/cellcoordinates.hpp"
#include "../../model/world/data.hpp"

#include "mask.hpp"

namespace CSVRender
{
    const int CellWater::CellSize = ESM::Land::REAL_SIZE;

    CellWater::CellWater(CSMWorld::Data& data, osg::Group* cellNode, const std::string& id,
        const CSMWorld::CellCoordinates& cellCoords)
        : mData(data)
        , mId(id)
        , mParentNode(cellNode)
        , mWaterTransform(0)
        , mWaterNode(0)
        , mWaterGeometry(0)
        , mExterior(false)
        , mHasWater(false)
        , mWaterHeight(0)
    {
        mWaterTransform = new osg::PositionAttitudeTransform();
        mWaterTransform->setPosition(osg::Vec3f(cellCoords.getX() * CellSize, cellCoords.getY() * CellSize, 0));
        mWaterTransform->setNodeMask(Mask_Water);
        mParentNode->addChild(mWaterTransform);

        mWaterNode = new osg::Geode();
        mWaterTransform->addChild(mWaterNode);

        int cellIndex = mData.getCells().searchId(mId);
        if (cellIndex > -1)
        {
            updateCellData(mData.getCells().getRecord(cellIndex).get());
        }

        // Keep water existance/height up to date
        QAbstractItemModel* cells = mData.getTableModel(CSMWorld::UniversalId::Type_Cells);
        connect(cells, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
            this, SLOT(cellDataChanged(const QModelIndex&, const QModelIndex&)));
    }

    CellWater::~CellWater()
    {
        mParentNode->removeChild(mWaterTransform);
    }

    void CellWater::updateCellData(const CSMWorld::Cell& cell)
    {
        int cellIndex = mData.getCells().searchId(mId);
        if (cellIndex > -1)
        {
            const CSMWorld::Record<CSMWorld::Cell>& cellRecord = mData.getCells().getRecord(cellIndex);

            mDeleted = cellRecord.isDeleted();
            if (!mDeleted)
            {
                mExterior = cellRecord.get().isExterior();

                mHasWater = cellRecord.get().hasWater();
                mWaterHeight = cellRecord.get().mWater;
            }
        }
        else
        {
            mDeleted = true;
        }

        update();
    }

    void CellWater::cellDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
    {
        const CSMWorld::Collection<CSMWorld::Cell>& cells = mData.getCells();

        int rowStart = -1;
        int rowEnd = -1;

        if (topLeft.parent().isValid())
        {
            rowStart = topLeft.parent().row();
            rowEnd = bottomRight.parent().row();
        }
        else
        {
            rowStart = topLeft.row();
            rowEnd = bottomRight.row();
        }

        for (int row = rowStart; row <= rowEnd; ++row)
        {
            const CSMWorld::Cell& cell = cells.getRecord(row).get();

            if (Misc::StringUtils::lowerCase(cell.mId) == mId)
                updateCellData(cell);
        }
    }

    void CellWater::update()
    {
        const int InteriorSize = CellSize * 10;

        const size_t NumPoints = 4;
        const size_t NumIndices = 6;

        const osg::Vec3f ExteriorPoints[] =
        {
            osg::Vec3f(0,        0,        mWaterHeight),
            osg::Vec3f(0,        CellSize, mWaterHeight),
            osg::Vec3f(CellSize, CellSize, mWaterHeight),
            osg::Vec3f(CellSize, 0,        mWaterHeight)
        };

        const osg::Vec3f InteriorPoints[] =
        {
            osg::Vec3f(-InteriorSize, -InteriorSize, mWaterHeight),
            osg::Vec3f(-InteriorSize,  InteriorSize, mWaterHeight),
            osg::Vec3f( InteriorSize,  InteriorSize, mWaterHeight),
            osg::Vec3f( InteriorSize, -InteriorSize, mWaterHeight)
        };

        const unsigned short TriangleStrip[] =
        {
            0, 1, 2, 3, 0, 1
        };

        const osg::Vec4f Color = osg::Vec4f(0.6f, 0.7f, 1.f, 0.5f);

        if (mWaterGeometry)
        {
            mWaterNode->removeDrawable(mWaterGeometry);
            mWaterGeometry = 0;
        }

        if (mDeleted || !mHasWater)
            return;

        mWaterGeometry = new osg::Geometry();

        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
        osg::ref_ptr<osg::DrawElementsUShort> indices = new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLE_STRIP,
            NumIndices);

        for (size_t i = 0; i < NumPoints; ++i)
        {
            if (mExterior)
                vertices->push_back(ExteriorPoints[i]);
            else
                vertices->push_back(InteriorPoints[i]);
        }

        colors->push_back(Color);

        for (size_t i = 0; i < NumIndices; ++i)
        {
            indices->setElement(i, TriangleStrip[i]);
        }

        mWaterGeometry->setVertexArray(vertices);
        mWaterGeometry->setColorArray(colors, osg::Array::BIND_OVERALL);
        mWaterGeometry->addPrimitiveSet(indices);

        // Transparency
        mWaterGeometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        mWaterGeometry->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON );
        mWaterGeometry->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        mWaterGeometry->getOrCreateStateSet()->setRenderBinDetails(1000, "RenderBin");

        // Reduce some z-fighting
        osg::ref_ptr<osg::PolygonOffset> polygonOffset = new osg::PolygonOffset();
        polygonOffset->setFactor(0.2f);
        polygonOffset->setUnits(0.2f);

        mWaterGeometry->getOrCreateStateSet()->setAttributeAndModes(polygonOffset,
            osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);

        mWaterNode->addDrawable(mWaterGeometry);
    }
}
