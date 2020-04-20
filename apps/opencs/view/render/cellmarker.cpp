#include "cellmarker.hpp"

#include <osg/AutoTransform>
#include <osg/Material>
#include <osg/Geode>
#include <osgText/Text>

#include <components/misc/constants.hpp>

CSVRender::CellMarkerTag::CellMarkerTag(CellMarker *marker)
: TagBase(SceneUtil::Mask_EditorCellMarker), mMarker(marker)
{}

CSVRender::CellMarker *CSVRender::CellMarkerTag::getCellMarker() const
{
    return mMarker;
}

void CSVRender::CellMarker::buildMarker()
{
    const int characterSize = 20;

    // Set up attributes of marker text.
    osg::ref_ptr<osgText::Text> markerText (new osgText::Text);
    markerText->setLayout(osgText::Text::LEFT_TO_RIGHT);
    markerText->setCharacterSize(characterSize);
    markerText->setAlignment(osgText::Text::CENTER_CENTER);
    markerText->setDrawMode(osgText::Text::TEXT | osgText::Text::FILLEDBOUNDINGBOX);

    // If cell exists then show black bounding box otherwise show red.
    if (mExists)
    {
        markerText->setBoundingBoxColor(osg::Vec4f(0.0f, 0.0f, 0.0f, 1.0f));
    }
    else
    {
        markerText->setBoundingBoxColor(osg::Vec4f(1.0f, 0.0f, 0.0f, 1.0f));
    }

    // Add text containing cell's coordinates.
    std::string coordinatesText =
        std::to_string(mCoordinates.getX()) + "," +
        std::to_string(mCoordinates.getY());
    markerText->setText(coordinatesText);

    // Add text to marker node.
    osg::ref_ptr<osg::Geode> geode (new osg::Geode);
    geode->addDrawable(markerText);
    mMarkerNode->addChild(geode);
}

void CSVRender::CellMarker::positionMarker()
{
    const int cellSize = Constants::CellSizeInUnits;
    const int markerHeight = 0;

    // Move marker to center of cell.
    int x = (mCoordinates.getX() * cellSize) + (cellSize / 2);
    int y = (mCoordinates.getY() * cellSize) + (cellSize / 2);
    mMarkerNode->setPosition(osg::Vec3f(x, y, markerHeight));
}

CSVRender::CellMarker::CellMarker(
    osg::Group *cellNode,
    const CSMWorld::CellCoordinates& coordinates,
    const bool cellExists
) : mCellNode(cellNode),
    mCoordinates(coordinates),
    mExists(cellExists)
{
    // Set up node for cell marker.
    mMarkerNode = new osg::AutoTransform();
    mMarkerNode->setAutoRotateMode(osg::AutoTransform::ROTATE_TO_SCREEN);
    mMarkerNode->setAutoScaleToScreen(true);
    mMarkerNode->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    mMarkerNode->getOrCreateStateSet()->setRenderBinDetails(11, "RenderBin");
    osg::ref_ptr<osg::Material> mat = new osg::Material;
    mat->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
    mMarkerNode->getOrCreateStateSet()->setAttribute(mat);

    mMarkerNode->setUserData(new CellMarkerTag(this));
    mMarkerNode->setNodeMask(SceneUtil::Mask_EditorCellMarker);

    mCellNode->addChild(mMarkerNode);

    buildMarker();
    positionMarker();
}

CSVRender::CellMarker::~CellMarker()
{
    mCellNode->removeChild(mMarkerNode);
}
