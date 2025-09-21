#include <unordered_set>

#include <QFile>

#include <osg/ClipPlane>
#include <osg/Material>
#include <osg/PositionAttitudeTransform>
#include <osgUtil/CullVisitor>

#include <components/resource/resourcesystem.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/visitor.hpp>

#include "../../model/prefs/state.hpp"
#include "objectmarker.hpp"
#include "worldspacewidget.hpp"

namespace
{
    class FindMaterialVisitor : public osg::NodeVisitor
    {
    public:
        FindMaterialVisitor(CSVRender::NodeMap& map)
            : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
            , mMap(map)
        {
        }

        void apply(osg::Geometry& node) override
        {
            osg::StateSet* state = node.getStateSet();
            if (state->getAttribute(osg::StateAttribute::MATERIAL))
                mMap.emplace(node.getName(), &node);

            traverse(node);
        }

    private:
        CSVRender::NodeMap& mMap;
    };

    class ToCamera : public SceneUtil::NodeCallback<ToCamera, osg::Node*, osgUtil::CullVisitor*>
    {
    public:
        ToCamera(osg::ref_ptr<osg::ClipPlane> clipPlane)
            : mClipPlane(std::move(clipPlane))
        {
        }
        void operator()(osg::Node* node, osgUtil::CullVisitor* cv)
        {
            osg::Vec3f normal = cv->getEyePoint();
            mClipPlane->setClipPlane(normal.x(), normal.y(), normal.z(), 0);
            traverse(node, cv);
        }

    private:
        osg::ref_ptr<osg::ClipPlane> mClipPlane;
    };

    auto addTagToActiveMarkerNodes = [](CSVRender::NodeMap& mMarkerNodes, CSVRender::Object* object,
                                         std::initializer_list<std::string> suffixes) {
        for (const auto& markerSuffix : suffixes)
        {
            for (char axis = 'X'; axis <= 'Z'; ++axis)
                mMarkerNodes[axis + markerSuffix]->setUserData(new CSVRender::ObjectMarkerTag(object, axis - 'X'));
        }
    };
}

namespace CSVRender
{
    ObjectMarkerTag::ObjectMarkerTag(Object* object, int axis)
        : ObjectTag(object)
        , mAxis(axis)
    {
    }

    ObjectMarker::ObjectMarker(WorldspaceWidget* worldspaceWidget, Resource::ResourceSystem* resourceSystem)
        : mWorldspaceWidget(worldspaceWidget)
        , mResourceSystem(resourceSystem)
        , mMarkerScale(CSMPrefs::get()["Rendering"]["object-marker-scale"].toDouble())
        , mSubMode(Object::Mode_None)
    {
        mBaseNode = new osg::PositionAttitudeTransform;
        mBaseNode->setNodeMask(Mask_Reference);
        mBaseNode->setScale(osg::Vec3f(mMarkerScale, mMarkerScale, mMarkerScale));

        mRootNode = new osg::PositionAttitudeTransform;
        mRootNode->addChild(mBaseNode);
        worldspaceWidget->setSelectionMarkerRoot(mRootNode);

        QFile file(":render/selection-marker");

        if (!file.open(QIODevice::ReadOnly))
            throw std::runtime_error("Failed to open selection marker file");

        auto markerData = file.readAll();

        mResourceSystem->getSceneManager()->loadSelectionMarker(mBaseNode, markerData.data(), markerData.size());

        osg::ref_ptr<osg::StateSet> baseNodeState = mBaseNode->getOrCreateStateSet();
        baseNodeState->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
        baseNodeState->setRenderBinDetails(1000, "RenderBin");

        FindMaterialVisitor matMapper(mMarkerNodes);

        mBaseNode->accept(matMapper);

        for (const auto& [name, node] : mMarkerNodes)
        {
            osg::StateSet* state = node->getStateSet();
            osg::Material* mat = static_cast<osg::Material*>(state->getAttribute(osg::StateAttribute::MATERIAL));
            osg::Vec4f emis = mat->getEmission(osg::Material::FRONT_AND_BACK);
            mat->setEmission(osg::Material::FRONT_AND_BACK, emis / 4);
            mOriginalColors.emplace(name, emis);
        }

        SceneUtil::NodeMap sceneNodes;
        SceneUtil::NodeMapVisitor nodeMapper(sceneNodes);
        mBaseNode->accept(nodeMapper);

        mMarkerNodes.insert(sceneNodes.begin(), sceneNodes.end());

        osg::ref_ptr<osg::Node> rotateMarkers = mMarkerNodes["rotateMarkers"];
        osg::ClipPlane* clip = new osg::ClipPlane(0);
        rotateMarkers->setCullCallback(new ToCamera(clip));
        rotateMarkers->getStateSet()->setAttributeAndModes(clip, osg::StateAttribute::ON);
    }

    void ObjectMarker::toggleVisibility()
    {
        bool isVisible = mBaseNode->getNodeMask() == Mask_Reference;
        mBaseNode->setNodeMask(isVisible ? Mask_Hidden : Mask_Reference);
    }

    void ObjectMarker::updateScale(const float scale)
    {
        mMarkerScale = scale;
        mBaseNode->setScale(osg::Vec3f(scale, scale, scale));
    }

    void ObjectMarker::setSubMode(const int subMode)
    {
        if (subMode == mSubMode)
            return;
        mSubMode = subMode;
        resetMarkerHighlight();
        updateSelectionMarker();
    }

    bool ObjectMarker::hitBehindMarker(const osg::Vec3d& hitPos, osg::ref_ptr<osg::Camera> camera)
    {
        if (mSubMode != Object::Mode_Rotate)
            return false;

        osg::Vec3d center, eye, forwardVector, _;
        std::vector<osg::Node*> rotMark = mMarkerNodes["rotateMarkers"]->getParentalNodePaths()[0];
        const osg::Vec3f markerPos = osg::computeLocalToWorld(rotMark).getTrans();

        camera->getViewMatrixAsLookAt(eye, center, _);
        forwardVector = center - eye;
        forwardVector.normalize();

        return (hitPos - markerPos) * forwardVector > 0;
    }

    bool ObjectMarker::attachMarker(const std::string& refId)
    {
        const auto& object = mWorldspaceWidget->getObjectByReferenceId(refId);

        if (!object)
            removeFromSelectionHistory(refId);

        if (!object || !object->getSelected())
            return false;

        if (!object->getRootNode()->addChild(mRootNode))
            throw std::runtime_error("Failed to add marker to object");

        std::string parentMarkerNode;

        switch (mSubMode)
        {
            case (Object::Mode_Rotate):
                parentMarkerNode = "rotateMarkers";
                addTagToActiveMarkerNodes(mMarkerNodes, object, { "_Axis_Rot" });
                break;
            case (Object::Mode_Scale):
                parentMarkerNode = "scaleMarkers";
                addTagToActiveMarkerNodes(mMarkerNodes, object, { "_Axis_Scale", "_Wall_Scale" });
                break;
            case (Object::Mode_Move):
            default:
                parentMarkerNode = "moveMarkers";
                addTagToActiveMarkerNodes(mMarkerNodes, object, { "_Axis", "_Wall" });
                break;
        }

        mMarkerNodes[parentMarkerNode]->asGroup()->setNodeMask(Mask_Reference);

        return true;
    }

    void ObjectMarker::detachMarker()
    {
        for (unsigned index = mRootNode->getNumParents(); index > 0;)
            mRootNode->getParent(--index)->removeChild(mRootNode);

        osg::ref_ptr<osg::Group> widgetRoot = mMarkerNodes["unitArrows"]->asGroup();
        for (unsigned index = widgetRoot->getNumChildren(); index > 0;)
            widgetRoot->getChild(--index)->setNodeMask(Mask_Hidden);
    }

    void ObjectMarker::addToSelectionHistory(const std::string& refId, bool update)
    {
        auto foundObject = std::find_if(mSelectionHistory.begin(), mSelectionHistory.end(),
            [&refId](const std::string& objId) { return objId == refId; });

        if (foundObject == mSelectionHistory.end())
            mSelectionHistory.push_back(refId);
        else
            std::rotate(foundObject, foundObject + 1, mSelectionHistory.end());

        if (update)
            updateSelectionMarker(refId);
    }

    void ObjectMarker::removeFromSelectionHistory(const std::string& refId)
    {
        mSelectionHistory.erase(std::remove_if(mSelectionHistory.begin(), mSelectionHistory.end(),
                                    [&refId](const std::string& objId) { return objId == refId; }),
            mSelectionHistory.end());
    }

    void ObjectMarker::updateSelectionMarker(const std::string& refId)
    {
        if (mSelectionHistory.empty())
            return;

        detachMarker();

        if (refId.empty())
        {
            for (std::size_t index = mSelectionHistory.size(); index > 0;)
                if (attachMarker(mSelectionHistory[--index]))
                    break;
        }
        else
            attachMarker(refId);
    }

    void ObjectMarker::resetMarkerHighlight()
    {
        if (mLastHighlightedNodes.empty())
            return;

        for (const auto& [nodeName, mat] : mLastHighlightedNodes)
            mat->setEmission(osg::Material::FRONT_AND_BACK, mat->getEmission(osg::Material::FRONT_AND_BACK) / 4);

        mLastHighlightedNodes.clear();
        mLastHitNode.clear();
    }

    void ObjectMarker::updateMarkerHighlight(const std::string_view hitNode, const int axis)
    {
        if (hitNode == mLastHitNode)
            return;

        resetMarkerHighlight();

        std::string colorName;

        switch (axis)
        {
            case Object::Axis_X:
                colorName = "red";
                break;
            case Object::Axis_Y:
                colorName = "green";
                break;
            case Object::Axis_Z:
                colorName = "blue";
                break;
            default:
                throw std::runtime_error("Invalid axis for highlighting: " + std::to_string(axis));
        }

        std::vector<std::string> targetMaterials = { colorName + "-material" };

        if (mSubMode != Object::Mode_Rotate)
            targetMaterials.emplace_back(colorName + "_alpha-material");

        for (const auto& materialNodeName : targetMaterials)
        {
            osg::ref_ptr<osg::Node> matNode = mMarkerNodes[materialNodeName];
            osg::StateSet* state = matNode->getStateSet();
            osg::StateAttribute* matAttr = state->getAttribute(osg::StateAttribute::MATERIAL);

            osg::Material* mat = static_cast<osg::Material*>(matAttr);
            mat->setEmission(osg::Material::FRONT_AND_BACK, mOriginalColors[materialNodeName]);

            mLastHighlightedNodes.emplace(std::make_pair(matNode->getName(), mat));
        }

        mLastHitNode = hitNode;
    }
}
