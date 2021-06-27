#ifndef CSV_RENDER_PATHGRID_H
#define CSV_RENDER_PATHGRID_H

#include <vector>

#include <QString>
#include <osg/ref_ptr>
#include <osg/Vec3d>

#include "../../model/world/cellcoordinates.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/world/subcellcollection.hpp"

#include "tagbase.hpp"

namespace osg
{
    class Geode;
    class Geometry;
    class Group;
    class PositionAttitudeTransform;
}

namespace CSMWorld
{
    class CommandMacro;
    class Data;
    struct Pathgrid;
}

namespace CSVRender
{
    class Pathgrid;

    class PathgridTag : public TagBase
    {
        public:

            PathgridTag (Pathgrid* pathgrid);

            Pathgrid* getPathgrid () const;

            QString getToolTip (bool hideBasics, const WorldspaceHitResult& hit) const override;

        private:

            Pathgrid* mPathgrid;
    };

    class Pathgrid
    {
        public:

            typedef std::vector<unsigned short> NodeList;

            Pathgrid(CSMWorld::Data& data, osg::Group* parent, const std::string& pathgridId,
                const CSMWorld::CellCoordinates& coordinates);

            ~Pathgrid();

            const CSMWorld::CellCoordinates& getCoordinates() const;
            const std::string& getId() const;

            bool isSelected() const;
            const NodeList& getSelected() const;
            void selectAll();
            void toggleSelected(unsigned short node); // Adds to end of vector
            void invertSelected();
            void clearSelected();

            void moveSelected(const osg::Vec3d& offset);
            void setDragOrigin(unsigned short node);
            void setDragEndpoint(unsigned short node);
            void setDragEndpoint(const osg::Vec3d& pos);

            void resetIndicators();

            void applyPoint(CSMWorld::CommandMacro& commands, const osg::Vec3d& worldPos);
            void applyPosition(CSMWorld::CommandMacro& commands);
            void applyEdge(CSMWorld::CommandMacro& commands, unsigned short node1, unsigned short node2);
            void applyEdges(CSMWorld::CommandMacro& commands, unsigned short node);
            void applyRemoveNodes(CSMWorld::CommandMacro& commands);
            void applyRemoveEdges(CSMWorld::CommandMacro& commands);

            osg::ref_ptr<PathgridTag> getTag() const;

            void recreateGeometry();
            void removeGeometry();

            void update();

        private:

            CSMWorld::Data& mData;
            CSMWorld::SubCellCollection<CSMWorld::Pathgrid>& mPathgridCollection;
            std::string mId;
            CSMWorld::CellCoordinates mCoords;
            bool mInterior;

            NodeList mSelected;
            osg::Vec3d mMoveOffset;
            unsigned short mDragOrigin;

            bool mChangeGeometry;
            bool mRemoveGeometry;
            bool mUseOffset;

            osg::Group* mParent;
            osg::ref_ptr<osg::PositionAttitudeTransform> mBaseNode;
            osg::ref_ptr<osg::Geode> mPathgridGeode;
            osg::ref_ptr<osg::Geometry> mPathgridGeometry;
            osg::ref_ptr<osg::Geometry> mSelectedGeometry;
            osg::ref_ptr<osg::Geometry> mDragGeometry;

            osg::ref_ptr<PathgridTag> mTag;

            void createGeometry();
            void createSelectedGeometry();
            void createSelectedGeometry(const CSMWorld::Pathgrid& source);
            void removePathgridGeometry();
            void removeSelectedGeometry();

            void createDragGeometry(const osg::Vec3f& start, const osg::Vec3f& end, bool valid);

            const CSMWorld::Pathgrid* getPathgridSource();

            int edgeExists(const CSMWorld::Pathgrid& source, unsigned short node1, unsigned short node2);
            void addEdge(CSMWorld::CommandMacro& commands, const CSMWorld::Pathgrid& source, unsigned short node1,
                unsigned short node2);
            void removeEdge(CSMWorld::CommandMacro& commands, const CSMWorld::Pathgrid& source, unsigned short node1,
                unsigned short node2);

            int clampToCell(int v);
    };
}

#endif
