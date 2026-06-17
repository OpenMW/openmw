#ifndef OPENCS_VIEW_OBJECT_MARKER_H
#define OPENCS_VIEW_OBJECT_MARKER_H

#include "object.hpp"

namespace osg
{
    class Camera;
    class Material;
}

namespace CSVRender
{
    using NodeMap = std::unordered_map<std::string, osg::ref_ptr<osg::Node>>;
    class WorldspaceWidget;

    class ObjectMarkerTag : public ObjectTag
    {
    public:
        ObjectMarkerTag(Object* object, int axis);

        int mAxis;
    };

    class ObjectMarker
    {
        friend class WorldspaceWidget;

        WorldspaceWidget* mWorldspaceWidget;
        Resource::ResourceSystem* mResourceSystem;
        NodeMap mMarkerNodes;
        osg::ref_ptr<osg::PositionAttitudeTransform> mBaseNode;
        osg::ref_ptr<osg::PositionAttitudeTransform> mRootNode;
        std::unordered_map<std::string, osg::Vec4f> mOriginalColors;
        std::vector<std::string> mSelectionHistory;
        std::string mLastHitNode;
        std::unordered_map<std::string, osg::Material*> mLastHighlightedNodes;
        float mMarkerScale;
        int mSubMode;

        ObjectMarker(WorldspaceWidget* worldspaceWidget, Resource::ResourceSystem* resourceSystem);

        static std::unique_ptr<ObjectMarker> create(WorldspaceWidget* widget, Resource::ResourceSystem* resourceSystem)
        {
            return std::unique_ptr<ObjectMarker>(new ObjectMarker(widget, resourceSystem));
        }

        bool attachMarker(const std::string& refId);

        void removeFromSelectionHistory(const std::string& refId);

    public:
        ObjectMarker(ObjectMarker&) = delete;
        ObjectMarker(ObjectMarker&&) = delete;
        ObjectMarker& operator=(const ObjectMarker&) = delete;
        ObjectMarker& operator=(ObjectMarker&&) = delete;

        void toggleVisibility();

        bool hitBehindMarker(const osg::Vec3d& hitPos, osg::ref_ptr<osg::Camera> camera);

        void detachMarker();

        void addToSelectionHistory(const std::string& refId, bool update = true);

        void updateSelectionMarker(const std::string& refId = std::string());

        void resetMarkerHighlight();

        void updateMarkerHighlight(const std::string_view hitNode, const int axis);

        void setSubMode(const int subMode);

        void updateScale(const float scale);
    };
}
#endif // OPENCS_VIEW_OBJECT_MARKER_H
