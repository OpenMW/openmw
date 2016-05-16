#ifndef CSV_RENDER_PATHGRID_H
#define CSV_RENDER_PATHGRID_H

#include <vector>

#include <QString>
#include <osg/ref_ptr>

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
    class Vec3d;
}

namespace CSMWorld
{
    class CommandMacro;
    class Data;
    class Pathgrid;
}

namespace CSVRender
{
    class Pathgrid;

    class PathgridTag : public TagBase
    {
        public:

            PathgridTag (Pathgrid* pathgrid);

            Pathgrid* getPathgrid () const;

            virtual QString getToolTip (bool hideBasics) const;

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
            void resetMove();

            void applyPoint(CSMWorld::CommandMacro& commands, const osg::Vec3d& worldPos);
            void applyPosition(CSMWorld::CommandMacro& commands);
            void applyEdge(CSMWorld::CommandMacro& commands, unsigned short node1, unsigned short node2);
            void applyEdges(CSMWorld::CommandMacro& commands, unsigned short node);

            osg::ref_ptr<PathgridTag> getTag() const;

            void recreateGeometry();

        private:

            CSMWorld::Data& mData;
            CSMWorld::SubCellCollection<CSMWorld::Pathgrid>& mPathgridCollection;
            std::string mId;
            CSMWorld::CellCoordinates mCoords;

            NodeList mSelected;

            osg::Group* mParent;
            osg::ref_ptr<osg::PositionAttitudeTransform> mBaseNode;
            osg::ref_ptr<osg::PositionAttitudeTransform> mSelectedNode;
            osg::ref_ptr<osg::Geode> mPathgridGeode;
            osg::ref_ptr<osg::Geode> mSelectedGeode;
            osg::ref_ptr<osg::Geometry> mPathgridGeometry;
            osg::ref_ptr<osg::Geometry> mSelectedGeometry;

            osg::ref_ptr<PathgridTag> mTag;

            void recreateSelectedGeometry();
            void recreateSelectedGeometry(const CSMWorld::Pathgrid& source);
            void removePathgridGeometry();
            void removeSelectedGeometry();

            const CSMWorld::Pathgrid* getPathgridSource();

    };
}

#endif
