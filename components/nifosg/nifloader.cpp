#include "nifloader.hpp"

#include <mutex>
#include <string_view>

#include <osg/Array>
#include <osg/Geometry>
#include <osg/LOD>
#include <osg/Matrixf>
#include <osg/Sequence>
#include <osg/Switch>
#include <osg/TexGen>
#include <osg/TexMat>
#include <osg/ValueObject>

// resource
#include <components/debug/debuglog.hpp>
#include <components/misc/constants.hpp>
#include <components/misc/osguservalues.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/lower.hpp>
#include <components/nif/parent.hpp>
#include <components/resource/bgsmfilemanager.hpp>
#include <components/resource/imagemanager.hpp>

// particle
#include <osgParticle/BoxPlacer>
#include <osgParticle/ConstantRateCounter>
#include <osgParticle/ModularProgram>
#include <osgParticle/ParticleSystem>
#include <osgParticle/ParticleSystemUpdater>

#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/FrontFace>
#include <osg/Material>
#include <osg/PolygonMode>
#include <osg/PolygonOffset>
#include <osg/Stencil>
#include <osg/TexEnv>
#include <osg/TexEnvCombine>
#include <osg/Texture2D>

#include <components/bgsm/file.hpp>
#include <components/nif/effect.hpp>
#include <components/nif/exception.hpp>
#include <components/nif/extra.hpp>
#include <components/nif/niffile.hpp>
#include <components/nif/node.hpp>
#include <components/nif/particle.hpp>
#include <components/nif/property.hpp>
#include <components/nif/texture.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/sceneutil/extradata.hpp>
#include <components/sceneutil/morphgeometry.hpp>
#include <components/sceneutil/riggeometry.hpp>
#include <components/sceneutil/skeleton.hpp>
#include <components/sceneutil/texturetype.hpp>

#include "fog.hpp"
#include "matrixtransform.hpp"
#include "particle.hpp"

namespace
{
    struct DisableOptimizer : osg::NodeVisitor
    {
        DisableOptimizer(osg::NodeVisitor::TraversalMode mode = TRAVERSE_ALL_CHILDREN)
            : osg::NodeVisitor(mode)
        {
        }

        void apply(osg::Node& node) override
        {
            node.setDataVariance(osg::Object::DYNAMIC);
            traverse(node);
        }

        void apply(osg::Drawable& node) override { traverse(node); }
    };

    void getAllNiNodes(const Nif::NiAVObject* node, std::vector<int>& outIndices)
    {
        if (const Nif::NiNode* ninode = dynamic_cast<const Nif::NiNode*>(node))
        {
            outIndices.push_back(ninode->recIndex);
            for (const auto& child : ninode->mChildren)
                if (!child.empty())
                    getAllNiNodes(child.getPtr(), outIndices);
        }
    }

    bool isTypeNiGeometry(int type)
    {
        switch (type)
        {
            case Nif::RC_NiTriShape:
            case Nif::RC_NiTriStrips:
            case Nif::RC_NiLines:
            case Nif::RC_BSLODTriShape:
            case Nif::RC_BSSegmentedTriShape:
                return true;
        }
        return false;
    }

    bool isTypeBSGeometry(int type)
    {
        switch (type)
        {
            case Nif::RC_BSTriShape:
            case Nif::RC_BSDynamicTriShape:
            case Nif::RC_BSMeshLODTriShape:
            case Nif::RC_BSSubIndexTriShape:
                return true;
        }
        return false;
    }

    // Collect all properties affecting the given drawable that should be handled on drawable basis rather than on the
    // node hierarchy above it.
    void collectDrawableProperties(
        const Nif::NiAVObject* nifNode, const Nif::Parent* parent, std::vector<const Nif::NiProperty*>& out)
    {
        if (parent != nullptr)
            collectDrawableProperties(&parent->mNiNode, parent->mParent, out);
        for (const auto& property : nifNode->mProperties)
        {
            if (!property.empty())
            {
                switch (property->recType)
                {
                    case Nif::RC_NiMaterialProperty:
                    case Nif::RC_NiVertexColorProperty:
                    case Nif::RC_NiSpecularProperty:
                    case Nif::RC_NiAlphaProperty:
                        out.push_back(property.getPtr());
                        break;
                    default:
                        break;
                }
            }
        }
    }

    // NodeCallback used to have a node always oriented towards the camera. The node can have translation and scale
    // set just like a regular MatrixTransform, but the rotation set will be overridden in order to face the camera.
    class BillboardCallback : public SceneUtil::NodeCallback<BillboardCallback, osg::Node*, osgUtil::CullVisitor*>
    {
    public:
        BillboardCallback() {}
        BillboardCallback(const BillboardCallback& copy, const osg::CopyOp& copyop)
            : SceneUtil::NodeCallback<BillboardCallback, osg::Node*, osgUtil::CullVisitor*>(copy, copyop)
        {
        }

        META_Object(NifOsg, BillboardCallback)

        void operator()(osg::Node* node, osgUtil::CullVisitor* cv)
        {
            osg::Matrix modelView = *cv->getModelViewMatrix();

            // attempt to preserve scale
            float mag[3];
            for (int i = 0; i < 3; ++i)
            {
                mag[i] = std::sqrt(modelView(0, i) * modelView(0, i) + modelView(1, i) * modelView(1, i)
                    + modelView(2, i) * modelView(2, i));
            }

            modelView.setRotate(osg::Quat());
            modelView(0, 0) = mag[0];
            modelView(1, 1) = mag[1];
            modelView(2, 2) = mag[2];

            cv->pushModelViewMatrix(new osg::RefMatrix(modelView), osg::Transform::RELATIVE_RF);

            traverse(node, cv);

            cv->popModelViewMatrix();
        }
    };

    void extractTextKeys(const Nif::NiTextKeyExtraData* tk, SceneUtil::TextKeyMap& textkeys)
    {
        for (const Nif::NiTextKeyExtraData::TextKey& key : tk->mList)
        {
            std::vector<std::string> results;
            Misc::StringUtils::split(key.mText, results, "\r\n");
            for (std::string& result : results)
            {
                Misc::StringUtils::trim(result);
                Misc::StringUtils::lowerCaseInPlace(result);
                if (!result.empty())
                    textkeys.emplace(key.mTime, std::move(result));
            }
        }
    }
}

namespace NifOsg
{
    bool Loader::sShowMarkers = false;

    void Loader::setShowMarkers(bool show)
    {
        sShowMarkers = show;
    }

    bool Loader::getShowMarkers()
    {
        return sShowMarkers;
    }

    unsigned int Loader::sHiddenNodeMask = 0;

    void Loader::setHiddenNodeMask(unsigned int mask)
    {
        sHiddenNodeMask = mask;
    }
    unsigned int Loader::getHiddenNodeMask()
    {
        return sHiddenNodeMask;
    }

    unsigned int Loader::sIntersectionDisabledNodeMask = ~0u;

    void Loader::setIntersectionDisabledNodeMask(unsigned int mask)
    {
        sIntersectionDisabledNodeMask = mask;
    }

    unsigned int Loader::getIntersectionDisabledNodeMask()
    {
        return sIntersectionDisabledNodeMask;
    }

    class LoaderImpl
    {
    public:
        /// @param filename used for warning messages.
        LoaderImpl(const std::filesystem::path& filename, unsigned int ver, unsigned int userver, unsigned int bethver)
            : mFilename(filename)
            , mVersion(ver)
            , mUserVersion(userver)
            , mBethVersion(bethver)
        {
        }
        std::filesystem::path mFilename;
        unsigned int mVersion, mUserVersion, mBethVersion;
        Resource::BgsmFileManager* mMaterialManager{ nullptr };
        Resource::ImageManager* mImageManager{ nullptr };

        size_t mFirstRootTextureIndex{ ~0u };
        bool mFoundFirstRootTexturingProperty = false;

        bool mHasNightDayLabel = false;
        bool mHasHerbalismLabel = false;
        bool mHasStencilProperty = false;

        const Nif::NiSortAdjustNode* mPushedSorter = nullptr;
        const Nif::NiSortAdjustNode* mLastAppliedNoInheritSorter = nullptr;

        // This is used to queue emitters that weren't attached to their node yet.
        std::vector<std::pair<size_t, osg::ref_ptr<Emitter>>> mEmitterQueue;

        void loadKf(Nif::FileView nif, SceneUtil::KeyframeHolder& target) const
        {
            const Nif::NiSequenceStreamHelper* seq = nullptr;
            const size_t numRoots = nif.numRoots();
            for (size_t i = 0; i < numRoots; ++i)
            {
                const Nif::Record* r = nif.getRoot(i);
                if (r && r->recType == Nif::RC_NiSequenceStreamHelper)
                {
                    seq = static_cast<const Nif::NiSequenceStreamHelper*>(r);
                    break;
                }
            }

            if (!seq)
            {
                Log(Debug::Warning) << "NIFFile Warning: Found no NiSequenceStreamHelper root record. File: "
                                    << nif.getFilename();
                return;
            }

            Nif::ExtraList extraList = seq->getExtraList();
            if (extraList.empty())
            {
                Log(Debug::Warning) << "NIFFile Warning: NiSequenceStreamHelper has no text keys. File: "
                                    << nif.getFilename();
                return;
            }

            if (extraList[0]->recType != Nif::RC_NiTextKeyExtraData)
            {
                Log(Debug::Warning) << "NIFFile Warning: First extra data was not a NiTextKeyExtraData, but a "
                                    << std::string_view(extraList[0]->recName) << ". File: " << nif.getFilename();
                return;
            }

            auto textKeyExtraData = static_cast<const Nif::NiTextKeyExtraData*>(extraList[0].getPtr());
            extractTextKeys(textKeyExtraData, target.mTextKeys);

            Nif::NiTimeControllerPtr ctrl = seq->mController;
            for (size_t i = 1; i < extraList.size() && !ctrl.empty(); i++, (ctrl = ctrl->mNext))
            {
                Nif::ExtraPtr extra = extraList[i];
                if (extra->recType != Nif::RC_NiStringExtraData || ctrl->recType != Nif::RC_NiKeyframeController)
                {
                    Log(Debug::Warning) << "NIFFile Warning: Unexpected extra data " << extra->recName
                                        << " with controller " << ctrl->recName << ". File: " << nif.getFilename();
                    continue;
                }

                // Vanilla seems to ignore the "active" flag for NiKeyframeController,
                // so we don't want to skip inactive controllers here.

                const Nif::NiStringExtraData* strdata = static_cast<const Nif::NiStringExtraData*>(extra.getPtr());
                const Nif::NiKeyframeController* key = static_cast<const Nif::NiKeyframeController*>(ctrl.getPtr());

                if (key->mData.empty() && key->mInterpolator.empty())
                    continue;

                if (!key->mInterpolator.empty() && key->mInterpolator->recType != Nif::RC_NiTransformInterpolator)
                {
                    Log(Debug::Error) << "Unsupported interpolator type for NiKeyframeController " << key->recIndex
                                      << " in " << mFilename;
                    continue;
                }

                osg::ref_ptr<SceneUtil::KeyframeController> callback = new NifOsg::KeyframeController(key);
                setupController(key, callback, /*animflags*/ 0);

                if (!target.mKeyframeControllers.emplace(strdata->mData, callback).second)
                    Log(Debug::Verbose) << "Controller " << strdata->mData << " present more than once in "
                                        << nif.getFilename() << ", ignoring later version";
            }
        }

        struct HandleNodeArgs
        {
            unsigned int mNifVersion;
            SceneUtil::TextKeyMap* mTextKeys;
            std::vector<unsigned int> mBoundTextures = {};
            int mAnimFlags = 0;
            bool mSkipMeshes = false;
            bool mHasMarkers = false;
            bool mHasAnimatedParents = false;
            osg::Node* mRootNode = nullptr;
        };

        osg::ref_ptr<osg::Node> load(Nif::FileView nif)
        {
            const size_t numRoots = nif.numRoots();
            std::vector<const Nif::NiAVObject*> roots;
            for (size_t i = 0; i < numRoots; ++i)
            {
                const Nif::Record* r = nif.getRoot(i);
                if (!r)
                    continue;
                const Nif::NiAVObject* nifNode = dynamic_cast<const Nif::NiAVObject*>(r);
                if (nifNode)
                    roots.emplace_back(nifNode);
            }
            if (roots.empty())
                throw Nif::Exception("Found no root nodes", nif.getFilename());

            osg::ref_ptr<SceneUtil::TextKeyMapHolder> textkeys(new SceneUtil::TextKeyMapHolder);

            osg::ref_ptr<osg::Group> created(new osg::Group);
            created->setDataVariance(osg::Object::STATIC);
            for (const Nif::NiAVObject* root : roots)
            {
                auto node = handleNode(
                    root, nullptr, nullptr, { .mNifVersion = nif.getVersion(), .mTextKeys = &textkeys->mTextKeys });
                created->addChild(node);
            }
            if (mHasNightDayLabel)
                created->getOrCreateUserDataContainer()->addDescription(Constants::NightDayLabel);
            if (mHasHerbalismLabel)
                created->getOrCreateUserDataContainer()->addDescription(Constants::HerbalismLabel);

            // Attach particle emitters to their nodes which should all be loaded by now.
            handleQueuedParticleEmitters(created, nif);

            if (nif.getUseSkinning())
            {
                osg::ref_ptr<SceneUtil::Skeleton> skel = new SceneUtil::Skeleton;
                skel->setStateSet(created->getStateSet());
                skel->setName(created->getName());
                for (unsigned int i = 0; i < created->getNumChildren(); ++i)
                    skel->addChild(created->getChild(i));
                created->removeChildren(0, created->getNumChildren());
                created = skel;
            }

            if (!textkeys->mTextKeys.empty())
                created->getOrCreateUserDataContainer()->addUserObject(textkeys);

            created->setUserValue(Misc::OsgUserValues::sFileHash, nif.getHash());

            return created;
        }

        void applyNodeProperties(const Nif::NiAVObject* nifNode, osg::Node* applyTo,
            SceneUtil::CompositeStateSetUpdater* composite, std::vector<unsigned int>& boundTextures, int animflags)
        {
            bool hasStencilProperty = false;

            for (const auto& property : nifNode->mProperties)
            {
                if (property.empty())
                    continue;

                if (property.getPtr()->recType == Nif::RC_NiStencilProperty)
                {
                    const Nif::NiStencilProperty* stencilprop
                        = static_cast<const Nif::NiStencilProperty*>(property.getPtr());
                    if (stencilprop->mEnabled)
                    {
                        hasStencilProperty = true;
                        break;
                    }
                }
            }

            for (const auto& property : nifNode->mProperties)
            {
                if (!property.empty())
                {
                    // Get the lowest numbered recIndex of the NiTexturingProperty root node.
                    // This is what is overridden when a spell effect "particle texture" is used.
                    if (nifNode->mParents.empty() && !mFoundFirstRootTexturingProperty
                        && property.getPtr()->recType == Nif::RC_NiTexturingProperty)
                    {
                        mFirstRootTextureIndex = property.getPtr()->recIndex;
                        mFoundFirstRootTexturingProperty = true;
                    }
                    else if (property.getPtr()->recType == Nif::RC_NiTexturingProperty)
                    {
                        if (property.getPtr()->recIndex == mFirstRootTextureIndex)
                            applyTo->setUserValue("overrideFx", 1);
                    }
                    handleProperty(property.getPtr(), applyTo, composite, boundTextures, animflags, hasStencilProperty);
                }
            }

            // NiAlphaProperty is handled as a drawable property
            Nif::BSShaderPropertyPtr shaderprop = nullptr;
            if (isTypeNiGeometry(nifNode->recType))
                shaderprop = static_cast<const Nif::NiGeometry*>(nifNode)->mShaderProperty;
            else if (isTypeBSGeometry(nifNode->recType))
                shaderprop = static_cast<const Nif::BSTriShape*>(nifNode)->mShaderProperty;

            if (!shaderprop.empty())
                handleProperty(shaderprop.getPtr(), applyTo, composite, boundTextures, animflags, hasStencilProperty);
        }

        static void setupController(const Nif::NiTimeController* ctrl, SceneUtil::Controller* toSetup, int animflags)
        {
            bool autoPlay = animflags & Nif::NiNode::AnimFlag_AutoPlay;
            if (autoPlay)
                toSetup->setSource(std::make_shared<SceneUtil::FrameTimeSource>());

            toSetup->setFunction(std::make_shared<ControllerFunction>(ctrl));
        }

        static osg::ref_ptr<osg::LOD> handleLodNode(const Nif::NiLODNode* niLodNode)
        {
            osg::ref_ptr<osg::LOD> lod(new osg::LOD);
            lod->setName(niLodNode->mName);
            lod->setCenterMode(osg::LOD::USER_DEFINED_CENTER);
            lod->setCenter(niLodNode->mLODCenter);
            for (unsigned int i = 0; i < niLodNode->mLODLevels.size(); ++i)
            {
                const Nif::NiLODNode::LODRange& range = niLodNode->mLODLevels[i];
                lod->setRange(i, range.mMinRange, range.mMaxRange);
            }
            lod->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
            return lod;
        }

        static osg::ref_ptr<osg::Switch> handleSwitchNode(const Nif::NiSwitchNode* niSwitchNode)
        {
            osg::ref_ptr<osg::Switch> switchNode(new osg::Switch);
            switchNode->setName(niSwitchNode->mName);
            switchNode->setNewChildDefaultValue(false);
            switchNode->setSingleChildOn(niSwitchNode->mInitialIndex);
            return switchNode;
        }

        static osg::ref_ptr<osg::Sequence> prepareSequenceNode(const Nif::NiAVObject* nifNode)
        {
            const Nif::NiFltAnimationNode* niFltAnimationNode = static_cast<const Nif::NiFltAnimationNode*>(nifNode);
            osg::ref_ptr<osg::Sequence> sequenceNode(new osg::Sequence);
            sequenceNode->setName(niFltAnimationNode->mName);
            if (!niFltAnimationNode->mChildren.empty())
            {
                if (niFltAnimationNode->swing())
                    sequenceNode->setDefaultTime(
                        niFltAnimationNode->mDuration / (niFltAnimationNode->mChildren.size() * 2));
                else
                    sequenceNode->setDefaultTime(niFltAnimationNode->mDuration / niFltAnimationNode->mChildren.size());
            }
            return sequenceNode;
        }

        static void activateSequenceNode(osg::Group* osgNode, const Nif::NiAVObject* nifNode)
        {
            const Nif::NiFltAnimationNode* niFltAnimationNode = static_cast<const Nif::NiFltAnimationNode*>(nifNode);
            osg::Sequence* sequenceNode = static_cast<osg::Sequence*>(osgNode);
            if (niFltAnimationNode->swing())
                sequenceNode->setInterval(osg::Sequence::SWING, 0, -1);
            else
                sequenceNode->setInterval(osg::Sequence::LOOP, 0, -1);
            sequenceNode->setDuration(1.0f, -1);
            sequenceNode->setMode(osg::Sequence::START);
        }

        osg::ref_ptr<osg::Image> handleSourceTexture(const Nif::NiSourceTexture* st) const
        {
            if (st)
            {
                if (st->mExternal)
                    return getTextureImage(st->mFile);

                if (!st->mData.empty())
                    return handleInternalTexture(st->mData.getPtr());
            }

            return nullptr;
        }

        bool handleEffect(const Nif::NiAVObject* nifNode, osg::StateSet* stateset) const
        {
            if (nifNode->recType != Nif::RC_NiTextureEffect)
            {
                Log(Debug::Info) << "Unhandled effect " << nifNode->recName << " in " << mFilename;
                return false;
            }

            const Nif::NiTextureEffect* textureEffect = static_cast<const Nif::NiTextureEffect*>(nifNode);
            if (!textureEffect->mSwitchState)
                return false;

            if (textureEffect->mTextureType != Nif::NiTextureEffect::TextureType::EnvironmentMap)
            {
                Log(Debug::Info) << "Unhandled NiTextureEffect type "
                                 << static_cast<uint32_t>(textureEffect->mTextureType) << " in " << mFilename;
                return false;
            }

            if (textureEffect->mTexture.empty())
            {
                Log(Debug::Info) << "NiTextureEffect missing source texture in " << mFilename;
                return false;
            }

            osg::ref_ptr<osg::TexGen> texGen(new osg::TexGen);
            switch (textureEffect->mCoordGenType)
            {
                case Nif::NiTextureEffect::CoordGenType::WorldParallel:
                    texGen->setMode(osg::TexGen::OBJECT_LINEAR);
                    break;
                case Nif::NiTextureEffect::CoordGenType::WorldPerspective:
                    texGen->setMode(osg::TexGen::EYE_LINEAR);
                    break;
                case Nif::NiTextureEffect::CoordGenType::SphereMap:
                    texGen->setMode(osg::TexGen::SPHERE_MAP);
                    break;
                default:
                    Log(Debug::Info) << "Unhandled NiTextureEffect CoordGenType "
                                     << static_cast<uint32_t>(textureEffect->mCoordGenType) << " in " << mFilename;
                    return false;
            }

            const unsigned int uvSet = 0;
            const unsigned int texUnit = 3; // FIXME
            std::vector<unsigned int> boundTextures;
            boundTextures.resize(3); // Dummy vector for attachNiSourceTexture
            attachNiSourceTexture("envMap", textureEffect->mTexture.getPtr(), textureEffect->wrapS(),
                textureEffect->wrapT(), uvSet, stateset, boundTextures);
            stateset->setTextureAttributeAndModes(texUnit, texGen, osg::StateAttribute::ON);
            stateset->setTextureAttributeAndModes(texUnit, createEmissiveTexEnv(), osg::StateAttribute::ON);

            stateset->addUniform(new osg::Uniform("envMapColor", osg::Vec4f(1, 1, 1, 1)));
            return true;
        }

        // Get a default dataVariance for this node to be used as a hint by optimization (post)routines
        static osg::ref_ptr<osg::Group> createNode(const Nif::NiAVObject* nifNode)
        {
            osg::ref_ptr<osg::Group> node;
            osg::Object::DataVariance dataVariance = osg::Object::UNSPECIFIED;

            switch (nifNode->recType)
            {
                case Nif::RC_NiBillboardNode:
                    dataVariance = osg::Object::DYNAMIC;
                    break;
                default:
                    // The Root node can be created as a Group if no transformation is required.
                    // This takes advantage of the fact root nodes can't have additional controllers
                    // loaded from an external .kf file (original engine just throws "can't find node" errors if you
                    // try).
                    if (nifNode->mParents.empty() && nifNode->mController.empty() && nifNode->mTransform.isIdentity())
                        node = new osg::Group;

                    dataVariance = nifNode->mIsBone ? osg::Object::DYNAMIC : osg::Object::STATIC;

                    break;
            }
            if (!node)
                node = new NifOsg::MatrixTransform(nifNode->mTransform);

            node->setDataVariance(dataVariance);

            return node;
        }

        osg::ref_ptr<osg::Node> handleNode(
            const Nif::NiAVObject* nifNode, const Nif::Parent* parent, osg::Group* parentNode, HandleNodeArgs args)
        {
            if (args.mRootNode && Misc::StringUtils::ciEqual(nifNode->mName, "Bounding Box"))
                return nullptr;

            osg::ref_ptr<osg::Group> node = createNode(nifNode);

            if (nifNode->recType == Nif::RC_NiBillboardNode)
            {
                node->addCullCallback(new BillboardCallback);
            }

            node->setName(nifNode->mName);

            if (parentNode)
                parentNode->addChild(node);

            if (!args.mRootNode)
                args.mRootNode = node;

            // The original NIF record index is used for a variety of features:
            // - finding the correct emitter node for a particle system
            // - establishing connections to the animated collision shapes, which are handled in a separate loader
            // - finding a random child NiNode in NiBspArrayController
            node->setUserValue("recIndex", nifNode->recIndex);

            for (const auto& e : nifNode->getExtraList())
            {
                if (e->recType == Nif::RC_NiTextKeyExtraData && args.mTextKeys)
                {
                    const Nif::NiTextKeyExtraData* tk = static_cast<const Nif::NiTextKeyExtraData*>(e.getPtr());
                    extractTextKeys(tk, *args.mTextKeys);
                }
                else if (e->recType == Nif::RC_NiStringExtraData)
                {
                    const Nif::NiStringExtraData* sd = static_cast<const Nif::NiStringExtraData*>(e.getPtr());

                    constexpr std::string_view extraDataIdentifer = "omw:data";

                    // String markers may contain important information
                    // affecting the entire subtree of this obj
                    if (sd->mData == "MRK")
                    {
                        // Marker objects. These meshes are only visible in the editor.
                        if (!Loader::getShowMarkers() && args.mRootNode == node)
                            args.mHasMarkers = true;
                    }
                    else if (sd->mData == "BONE")
                    {
                        node->getOrCreateUserDataContainer()->addDescription("CustomBone");
                    }
                    else if (sd->mData.rfind(extraDataIdentifer, 0) == 0)
                    {
                        node->setUserValue(
                            Misc::OsgUserValues::sExtraData, sd->mData.substr(extraDataIdentifer.length()));
                    }
                }
                else if (e->recType == Nif::RC_BSXFlags)
                {
                    if (args.mRootNode != node)
                        continue;

                    auto bsxFlags = static_cast<const Nif::NiIntegerExtraData*>(e.getPtr());
                    // Marker objects.
                    if (!Loader::getShowMarkers() && (bsxFlags->mData & 32))
                        args.mHasMarkers = true;
                }
            }

            if (nifNode->recType == Nif::RC_NiBSAnimationNode || nifNode->recType == Nif::RC_NiBSParticleNode)
                args.mAnimFlags = nifNode->mFlags;

            if (nifNode->recType == Nif::RC_NiSortAdjustNode)
            {
                auto sortNode = static_cast<const Nif::NiSortAdjustNode*>(nifNode);

                if (sortNode->mSubSorter.empty())
                {
                    Log(Debug::Warning) << "Empty accumulator found in '" << nifNode->recName << "' node "
                                        << nifNode->recIndex;
                }
                else
                {
                    if (mPushedSorter && !mPushedSorter->mSubSorter.empty()
                        && mPushedSorter->mMode != Nif::NiSortAdjustNode::SortingMode::Inherit)
                        mLastAppliedNoInheritSorter = mPushedSorter;
                    mPushedSorter = sortNode;
                }
            }

            // Hide collision shapes, but don't skip the subgraph
            // We still need to animate the hidden bones so the physics system can access them
            if (nifNode->recType == Nif::RC_RootCollisionNode)
            {
                args.mSkipMeshes = true;
                node->setNodeMask(Loader::getHiddenNodeMask());
            }

            // We can skip creating meshes for hidden nodes if they don't have a VisController that
            // might make them visible later
            if (nifNode->isHidden())
            {
                bool hasVisController = false;
                for (Nif::NiTimeControllerPtr ctrl = nifNode->mController; !ctrl.empty(); ctrl = ctrl->mNext)
                {
                    hasVisController |= (ctrl->recType == Nif::RC_NiVisController);
                    if (hasVisController)
                        break;
                }

                if (!hasVisController)
                    args.mSkipMeshes = true; // skip child meshes, but still create the child node hierarchy for
                                             // animating collision shapes

                node->setNodeMask(Loader::getHiddenNodeMask());
            }

            if (nifNode->recType == Nif::RC_NiCollisionSwitch && !nifNode->collisionActive())
            {
                node->setNodeMask(Loader::getIntersectionDisabledNodeMask());
                // Don't let the optimizer mess with this node
                node->setDataVariance(osg::Object::DYNAMIC);
            }

            osg::ref_ptr<SceneUtil::CompositeStateSetUpdater> composite = new SceneUtil::CompositeStateSetUpdater;

            applyNodeProperties(nifNode, node, composite, args.mBoundTextures, args.mAnimFlags);

            const bool isNiGeometry = isTypeNiGeometry(nifNode->recType);
            const bool isBSGeometry = isTypeBSGeometry(nifNode->recType);
            const bool isGeometry = isNiGeometry || isBSGeometry;

            if (isGeometry && !args.mSkipMeshes)
            {
                bool skip = false;
                if (args.mNifVersion <= Nif::NIFFile::NIFVersion::VER_MW)
                {
                    skip = (args.mHasMarkers && Misc::StringUtils::ciStartsWith(nifNode->mName, "tri editormarker"))
                        || Misc::StringUtils::ciStartsWith(nifNode->mName, "shadow")
                        || Misc::StringUtils::ciStartsWith(nifNode->mName, "tri shadow");
                }
                else
                {
                    if (args.mHasMarkers)
                        skip = Misc::StringUtils::ciStartsWith(nifNode->mName, "EditorMarker")
                            || Misc::StringUtils::ciStartsWith(nifNode->mName, "VisibilityEditorMarker");
                }
                if (!skip)
                {
                    if (isNiGeometry)
                        handleNiGeometry(nifNode, parent, node, composite, args.mBoundTextures, args.mAnimFlags);
                    else // isBSGeometry
                        handleBSGeometry(nifNode, parent, node, composite, args.mBoundTextures, args.mAnimFlags);

                    if (!nifNode->mController.empty())
                        handleMeshControllers(nifNode, node, composite, args.mBoundTextures, args.mAnimFlags);
                }
            }

            if (nifNode->recType == Nif::RC_NiParticles)
                handleParticleSystem(nifNode, parent, node, composite, args.mAnimFlags);

            if (composite->getNumControllers() > 0)
            {
                osg::Callback* cb = composite;
                if (composite->getNumControllers() == 1)
                    cb = composite->getController(0);
                if (args.mAnimFlags & Nif::NiNode::AnimFlag_AutoPlay)
                    node->addCullCallback(cb);
                else
                    node->addUpdateCallback(
                        cb); // have to remain as UpdateCallback so AssignControllerSourcesVisitor can find it.
            }

            bool isAnimated = false;
            handleNodeControllers(nifNode, node, args.mAnimFlags, isAnimated);
            args.mHasAnimatedParents |= isAnimated;
            // Make sure empty nodes and animated shapes are not optimized away so the physics system can find them.
            if (isAnimated || (args.mHasAnimatedParents && ((args.mSkipMeshes || args.mHasMarkers) || isGeometry)))
                node->setDataVariance(osg::Object::DYNAMIC);

            // LOD and Switch nodes must be wrapped by a transform (the current node) to support transformations
            // properly and we need to attach their children to the osg::LOD/osg::Switch nodes but we must return that
            // transform to the caller of handleNode instead of the actual LOD/Switch nodes.
            osg::ref_ptr<osg::Group> currentNode = node;

            if (nifNode->recType == Nif::RC_NiSwitchNode)
            {
                const Nif::NiSwitchNode* niSwitchNode = static_cast<const Nif::NiSwitchNode*>(nifNode);
                osg::ref_ptr<osg::Switch> switchNode = handleSwitchNode(niSwitchNode);
                node->addChild(switchNode);
                if (niSwitchNode->mName == Constants::NightDayLabel)
                    mHasNightDayLabel = true;
                else if (niSwitchNode->mName == Constants::HerbalismLabel)
                    mHasHerbalismLabel = true;

                currentNode = switchNode;
            }
            else if (nifNode->recType == Nif::RC_NiLODNode)
            {
                const Nif::NiLODNode* niLodNode = static_cast<const Nif::NiLODNode*>(nifNode);
                osg::ref_ptr<osg::LOD> lodNode = handleLodNode(niLodNode);
                node->addChild(lodNode);
                currentNode = lodNode;
            }
            else if (nifNode->recType == Nif::RC_NiFltAnimationNode)
            {
                osg::ref_ptr<osg::Sequence> sequenceNode = prepareSequenceNode(nifNode);
                node->addChild(sequenceNode);
                currentNode = sequenceNode;
            }

            const Nif::NiNode* ninode = dynamic_cast<const Nif::NiNode*>(nifNode);
            if (ninode)
            {
                const Nif::NiAVObjectList& children = ninode->mChildren;
                const Nif::Parent currentParent{ *ninode, parent };
                for (const auto& child : children)
                    if (!child.empty())
                        handleNode(child.getPtr(), &currentParent, currentNode, args);

                // Propagate effects to the the direct subgraph instead of the node itself
                // This simulates their "affected node list" which Morrowind appears to replace with the subgraph (?)
                // Note that the serialized affected node list is actually unused
                for (const auto& effect : ninode->mEffects)
                    if (!effect.empty())
                    {
                        osg::ref_ptr<osg::StateSet> effectStateSet = new osg::StateSet;
                        if (handleEffect(effect.getPtr(), effectStateSet))
                            for (unsigned int i = 0; i < currentNode->getNumChildren(); ++i)
                                currentNode->getChild(i)->getOrCreateStateSet()->merge(*effectStateSet);
                    }
            }

            if (nifNode->recType == Nif::RC_NiFltAnimationNode)
                activateSequenceNode(currentNode, nifNode);

            return node;
        }

        static void handleMeshControllers(const Nif::NiAVObject* nifNode, osg::Node* node,
            SceneUtil::CompositeStateSetUpdater* composite, const std::vector<unsigned int>& boundTextures,
            int animflags)
        {
            for (Nif::NiTimeControllerPtr ctrl = nifNode->mController; !ctrl.empty(); ctrl = ctrl->mNext)
            {
                if (!ctrl->isActive())
                    continue;
                if (ctrl->recType == Nif::RC_NiUVController)
                {
                    const Nif::NiUVController* niuvctrl = static_cast<const Nif::NiUVController*>(ctrl.getPtr());
                    if (niuvctrl->mData.empty())
                        continue;
                    std::set<unsigned int> texUnits;
                    // UVController should only work for textures which use the given UV Set.
                    for (unsigned int i = 0; i < boundTextures.size(); ++i)
                    {
                        if (boundTextures[i] == niuvctrl->mUvSet)
                            texUnits.insert(i);
                    }

                    osg::ref_ptr<UVController> uvctrl = new UVController(niuvctrl->mData.getPtr(), texUnits);
                    setupController(niuvctrl, uvctrl, animflags);
                    composite->addController(uvctrl);
                }
            }
        }

        void handleNodeControllers(
            const Nif::NiAVObject* nifNode, osg::Node* node, int animflags, bool& isAnimated) const
        {
            for (Nif::NiTimeControllerPtr ctrl = nifNode->mController; !ctrl.empty(); ctrl = ctrl->mNext)
            {
                if (!ctrl->isActive())
                    continue;
                if (ctrl->recType == Nif::RC_NiKeyframeController)
                {
                    const Nif::NiKeyframeController* key = static_cast<const Nif::NiKeyframeController*>(ctrl.getPtr());
                    if (key->mData.empty() && key->mInterpolator.empty())
                        continue;
                    if (!key->mInterpolator.empty() && key->mInterpolator->recType != Nif::RC_NiTransformInterpolator)
                    {
                        Log(Debug::Error) << "Unsupported interpolator type for NiKeyframeController " << key->recIndex
                                          << " in " << mFilename << ": " << key->mInterpolator->recName;
                        continue;
                    }
                    osg::ref_ptr<KeyframeController> callback = new KeyframeController(key);
                    setupController(key, callback, animflags);
                    node->addUpdateCallback(callback);
                    isAnimated = true;
                }
                else if (ctrl->recType == Nif::RC_NiPathController)
                {
                    const Nif::NiPathController* path = static_cast<const Nif::NiPathController*>(ctrl.getPtr());
                    if (path->mPathData.empty() || path->mPercentData.empty())
                        continue;
                    osg::ref_ptr<PathController> callback(new PathController(path));
                    setupController(path, callback, animflags);
                    node->addUpdateCallback(callback);
                    isAnimated = true;
                }
                else if (ctrl->recType == Nif::RC_NiVisController)
                {
                    const Nif::NiVisController* visctrl = static_cast<const Nif::NiVisController*>(ctrl.getPtr());
                    if (visctrl->mData.empty() && visctrl->mInterpolator.empty())
                        continue;
                    if (!visctrl->mInterpolator.empty()
                        && visctrl->mInterpolator->recType != Nif::RC_NiBoolInterpolator)
                    {
                        Log(Debug::Error) << "Unsupported interpolator type for NiVisController " << visctrl->recIndex
                                          << " in " << mFilename << ": " << visctrl->mInterpolator->recName;
                        continue;
                    }
                    osg::ref_ptr<VisController> callback(new VisController(visctrl, Loader::getHiddenNodeMask()));
                    setupController(visctrl, callback, animflags);
                    node->addUpdateCallback(callback);
                }
                else if (ctrl->recType == Nif::RC_NiRollController)
                {
                    const Nif::NiRollController* rollctrl = static_cast<const Nif::NiRollController*>(ctrl.getPtr());
                    if (rollctrl->mData.empty() && rollctrl->mInterpolator.empty())
                        continue;
                    if (!rollctrl->mInterpolator.empty()
                        && rollctrl->mInterpolator->recType != Nif::RC_NiFloatInterpolator)
                    {
                        Log(Debug::Error) << "Unsupported interpolator type for NiRollController " << rollctrl->recIndex
                                          << " in " << mFilename << ": " << rollctrl->mInterpolator->recName;
                        continue;
                    }
                    osg::ref_ptr<RollController> callback = new RollController(rollctrl);
                    setupController(rollctrl, callback, animflags);
                    node->addUpdateCallback(callback);
                    isAnimated = true;
                }
                else if (ctrl->recType == Nif::RC_NiGeomMorpherController
                    || ctrl->recType == Nif::RC_NiParticleSystemController
                    || ctrl->recType == Nif::RC_NiBSPArrayController || ctrl->recType == Nif::RC_NiUVController)
                {
                    // These controllers are handled elsewhere
                }
                else
                    Log(Debug::Info) << "Unhandled controller " << ctrl->recName << " on node " << nifNode->recIndex
                                     << " in " << mFilename;
            }
        }

        void handleMaterialControllers(const Nif::NiProperty* materialProperty,
            SceneUtil::CompositeStateSetUpdater* composite, int animflags, const osg::Material* baseMaterial) const
        {
            for (Nif::NiTimeControllerPtr ctrl = materialProperty->mController; !ctrl.empty(); ctrl = ctrl->mNext)
            {
                if (!ctrl->isActive())
                    continue;
                if (ctrl->recType == Nif::RC_NiAlphaController)
                {
                    const Nif::NiAlphaController* alphactrl = static_cast<const Nif::NiAlphaController*>(ctrl.getPtr());
                    if (alphactrl->mData.empty() && alphactrl->mInterpolator.empty())
                        continue;
                    if (!alphactrl->mInterpolator.empty()
                        && alphactrl->mInterpolator->recType != Nif::RC_NiFloatInterpolator)
                    {
                        Log(Debug::Error)
                            << "Unsupported interpolator type for NiAlphaController " << alphactrl->recIndex << " in "
                            << mFilename << ": " << alphactrl->mInterpolator->recName;
                        continue;
                    }
                    osg::ref_ptr<AlphaController> osgctrl = new AlphaController(alphactrl, baseMaterial);
                    setupController(alphactrl, osgctrl, animflags);
                    composite->addController(osgctrl);
                }
                else if (ctrl->recType == Nif::RC_NiMaterialColorController)
                {
                    const Nif::NiMaterialColorController* matctrl
                        = static_cast<const Nif::NiMaterialColorController*>(ctrl.getPtr());
                    Nif::NiInterpolatorPtr interp = matctrl->mInterpolator;
                    if (matctrl->mData.empty() && interp.empty())
                        continue;
                    if (mVersion <= Nif::NIFFile::VER_MW
                        && matctrl->mTargetColor == Nif::NiMaterialColorController::TargetColor::Specular)
                        continue;
                    if (!interp.empty() && interp->recType != Nif::RC_NiPoint3Interpolator)
                    {
                        Log(Debug::Error) << "Unsupported interpolator type for NiMaterialColorController "
                                          << matctrl->recIndex << " in " << mFilename << ": " << interp->recName;
                        continue;
                    }
                    osg::ref_ptr<MaterialColorController> osgctrl = new MaterialColorController(matctrl, baseMaterial);
                    setupController(matctrl, osgctrl, animflags);
                    composite->addController(osgctrl);
                }
                else
                    Log(Debug::Info) << "Unexpected material controller " << ctrl->recType << " in " << mFilename;
            }
        }

        osg::ref_ptr<osg::Image> getTextureImage(std::string_view path) const
        {
            if (!mImageManager)
                return nullptr;

            return mImageManager->getImage(
                VFS::Path::toNormalized(Misc::ResourceHelpers::correctTexturePath(path, mImageManager->getVFS())));
        }

        static osg::ref_ptr<osg::Texture2D> attachTexture(const std::string& name, osg::ref_ptr<osg::Image> image,
            bool wrapS, bool wrapT, unsigned int uvSet, osg::StateSet* stateset,
            std::vector<unsigned int>& boundTextures)
        {
            osg::ref_ptr<osg::Texture2D> texture2d = new osg::Texture2D(image);
            if (image)
                texture2d->setTextureSize(image->s(), image->t());
            texture2d->setWrap(osg::Texture::WRAP_S, wrapS ? osg::Texture::REPEAT : osg::Texture::CLAMP_TO_EDGE);
            texture2d->setWrap(osg::Texture::WRAP_T, wrapT ? osg::Texture::REPEAT : osg::Texture::CLAMP_TO_EDGE);
            unsigned int texUnit = boundTextures.size();
            if (stateset)
            {
                stateset->setTextureAttributeAndModes(texUnit, texture2d, osg::StateAttribute::ON);
                osg::ref_ptr<SceneUtil::TextureType> textureType = new SceneUtil::TextureType(name);
                textureType = shareAttribute(textureType);
                stateset->setTextureAttributeAndModes(texUnit, textureType, osg::StateAttribute::ON);
            }
            boundTextures.emplace_back(uvSet);
            return texture2d;
        }

        osg::ref_ptr<osg::Texture2D> attachExternalTexture(const std::string& name, const std::string& path, bool wrapS,
            bool wrapT, unsigned int uvSet, osg::StateSet* stateset, std::vector<unsigned int>& boundTextures) const
        {
            return attachTexture(name, getTextureImage(path), wrapS, wrapT, uvSet, stateset, boundTextures);
        }

        osg::ref_ptr<osg::Texture2D> attachNiSourceTexture(const std::string& name, const Nif::NiSourceTexture* st,
            bool wrapS, bool wrapT, unsigned int uvSet, osg::StateSet* stateset,
            std::vector<unsigned int>& boundTextures) const
        {
            return attachTexture(name, handleSourceTexture(st), wrapS, wrapT, uvSet, stateset, boundTextures);
        }

        static void clearBoundTextures(osg::StateSet* stateset, std::vector<unsigned int>& boundTextures)
        {
            if (!boundTextures.empty())
            {
                for (unsigned int i = 0; i < boundTextures.size(); ++i)
                    stateset->setTextureMode(i, GL_TEXTURE_2D, osg::StateAttribute::OFF);
                boundTextures.clear();
            }
        }

        void handleTextureControllers(const Nif::NiProperty* texProperty,
            SceneUtil::CompositeStateSetUpdater* composite, osg::StateSet* stateset, int animflags) const
        {
            for (Nif::NiTimeControllerPtr ctrl = texProperty->mController; !ctrl.empty(); ctrl = ctrl->mNext)
            {
                if (!ctrl->isActive())
                    continue;
                if (ctrl->recType == Nif::RC_NiFlipController)
                {
                    const Nif::NiFlipController* flipctrl = static_cast<const Nif::NiFlipController*>(ctrl.getPtr());
                    if (!flipctrl->mInterpolator.empty()
                        && flipctrl->mInterpolator->recType != Nif::RC_NiFloatInterpolator)
                    {
                        Log(Debug::Error) << "Unsupported interpolator type for NiFlipController " << flipctrl->recIndex
                                          << " in " << mFilename << ": " << flipctrl->mInterpolator->recName;
                        continue;
                    }
                    std::vector<osg::ref_ptr<osg::Texture2D>> textures;

                    // inherit wrap settings from the target slot
                    osg::Texture2D* inherit
                        = dynamic_cast<osg::Texture2D*>(stateset->getTextureAttribute(0, osg::StateAttribute::TEXTURE));
                    osg::Texture2D::WrapMode wrapS = osg::Texture2D::REPEAT;
                    osg::Texture2D::WrapMode wrapT = osg::Texture2D::REPEAT;
                    if (inherit)
                    {
                        wrapS = inherit->getWrap(osg::Texture2D::WRAP_S);
                        wrapT = inherit->getWrap(osg::Texture2D::WRAP_T);
                    }

                    const unsigned int uvSet = 0;
                    std::vector<unsigned int> boundTextures; // Dummy list for attachTexture
                    for (const auto& source : flipctrl->mSources)
                    {
                        if (source.empty())
                            continue;

                        // NB: not changing the stateset
                        osg::ref_ptr<osg::Texture2D> texture
                            = attachNiSourceTexture({}, source.getPtr(), wrapS, wrapT, uvSet, nullptr, boundTextures);
                        textures.push_back(texture);
                    }
                    osg::ref_ptr<FlipController> callback(new FlipController(flipctrl, textures));
                    setupController(ctrl.getPtr(), callback, animflags);
                    composite->addController(callback);
                }
                else
                    Log(Debug::Info) << "Unexpected texture controller " << ctrl->recName << " in " << mFilename;
            }
        }

        void handleParticlePrograms(Nif::NiParticleModifierPtr modifier, Nif::NiParticleModifierPtr collider,
            osg::Group* attachTo, osgParticle::ParticleSystem* partsys,
            osgParticle::ParticleProcessor::ReferenceFrame rf) const
        {
            osgParticle::ModularProgram* program = new osgParticle::ModularProgram;
            attachTo->addChild(program);
            program->setParticleSystem(partsys);
            program->setReferenceFrame(rf);
            for (; !modifier.empty(); modifier = modifier->mNext)
            {
                if (modifier->recType == Nif::RC_NiParticleGrowFade)
                {
                    const Nif::NiParticleGrowFade* gf = static_cast<const Nif::NiParticleGrowFade*>(modifier.getPtr());
                    program->addOperator(new GrowFadeAffector(gf->mGrowTime, gf->mFadeTime));
                }
                else if (modifier->recType == Nif::RC_NiGravity)
                {
                    const Nif::NiGravity* gr = static_cast<const Nif::NiGravity*>(modifier.getPtr());
                    program->addOperator(new GravityAffector(gr));
                }
                else if (modifier->recType == Nif::RC_NiParticleBomb)
                {
                    auto bomb = static_cast<const Nif::NiParticleBomb*>(modifier.getPtr());
                    osg::ref_ptr<osgParticle::ModularProgram> bombProgram(new osgParticle::ModularProgram);
                    attachTo->addChild(bombProgram);
                    bombProgram->setParticleSystem(partsys);
                    bombProgram->setReferenceFrame(rf);
                    bombProgram->setStartTime(bomb->mStartTime);
                    bombProgram->setLifeTime(bomb->mDuration);
                    bombProgram->setEndless(false);
                    bombProgram->addOperator(new ParticleBomb(bomb));
                }
                else if (modifier->recType == Nif::RC_NiParticleColorModifier)
                {
                    const Nif::NiParticleColorModifier* cl
                        = static_cast<const Nif::NiParticleColorModifier*>(modifier.getPtr());
                    if (cl->mData.empty())
                        continue;
                    const Nif::NiColorData* clrdata = cl->mData.getPtr();
                    program->addOperator(new ParticleColorAffector(clrdata));
                }
                else if (modifier->recType == Nif::RC_NiParticleRotation)
                {
                    // unused
                }
                else
                    Log(Debug::Info) << "Unhandled particle modifier " << modifier->recName << " in " << mFilename;
            }
            for (; !collider.empty(); collider = collider->mNext)
            {
                if (collider->recType == Nif::RC_NiPlanarCollider)
                {
                    const Nif::NiPlanarCollider* planarcollider
                        = static_cast<const Nif::NiPlanarCollider*>(collider.getPtr());
                    program->addOperator(new PlanarCollider(planarcollider));
                }
                else if (collider->recType == Nif::RC_NiSphericalCollider)
                {
                    const Nif::NiSphericalCollider* sphericalcollider
                        = static_cast<const Nif::NiSphericalCollider*>(collider.getPtr());
                    program->addOperator(new SphericalCollider(sphericalcollider));
                }
                else
                    Log(Debug::Info) << "Unhandled particle collider " << collider->recName << " in " << mFilename;
            }
        }

        // Load the initial state of the particle system, i.e. the initial particles and their positions, velocity and
        // colors.
        static void handleParticleInitialState(
            const Nif::NiAVObject* nifNode, ParticleSystem* partsys, const Nif::NiParticleSystemController* partctrl)
        {
            auto particleNode = static_cast<const Nif::NiParticles*>(nifNode);
            if (particleNode->mData.empty())
            {
                partsys->setQuota(partctrl->mParticles.size());
                return;
            }

            auto particledata = static_cast<const Nif::NiParticlesData*>(particleNode->mData.getPtr());
            partsys->setQuota(particledata->mNumParticles);

            osg::BoundingBox box;

            int i = 0;
            for (const auto& particle : partctrl->mParticles)
            {
                if (i++ >= particledata->mActiveCount)
                    break;

                if (particle.mLifespan <= 0)
                    continue;

                if (particle.mCode >= particledata->mVertices.size())
                    continue;

                ParticleAgeSetter particletemplate(std::max(0.f, particle.mAge));

                osgParticle::Particle* created = partsys->createParticle(&particletemplate);
                created->setLifeTime(particle.mLifespan);

                // Note this position and velocity is not correct for a particle system with absolute reference frame,
                // which can not be done in this loader since we are not attached to the scene yet. Will be fixed up
                // post-load in the SceneManager.
                created->setVelocity(particle.mVelocity);
                const osg::Vec3f& position = particledata->mVertices[particle.mCode];
                created->setPosition(position);

                created->setColorRange(osgParticle::rangev4(partctrl->mInitialColor, partctrl->mInitialColor));
                created->setAlphaRange(osgParticle::rangef(1.f, 1.f));

                float size = partctrl->mInitialSize;
                if (particle.mCode < particledata->mSizes.size())
                    size *= particledata->mSizes[particle.mCode];

                created->setSizeRange(osgParticle::rangef(size, size));
                box.expandBy(osg::BoundingSphere(position, size));
            }

            // radius may be used to force a larger bounding box
            box.expandBy(osg::BoundingSphere(osg::Vec3(0, 0, 0), particledata->mBoundingSphere.radius()));

            partsys->setInitialBound(box);
        }

        static osg::ref_ptr<Emitter> handleParticleEmitter(const Nif::NiParticleSystemController* partctrl)
        {
            std::vector<int> targets;
            if (partctrl->recType == Nif::RC_NiBSPArrayController && !partctrl->emitAtVertex())
            {
                getAllNiNodes(partctrl->mEmitter.getPtr(), targets);
            }

            osg::ref_ptr<Emitter> emitter = new Emitter(targets);

            osgParticle::ConstantRateCounter* counter = new osgParticle::ConstantRateCounter;
            if (partctrl->noAutoAdjust())
                counter->setNumberOfParticlesPerSecondToCreate(partctrl->mBirthRate);
            else if (partctrl->mLifetime == 0 && partctrl->mLifetimeVariation == 0)
                counter->setNumberOfParticlesPerSecondToCreate(0);
            else
                counter->setNumberOfParticlesPerSecondToCreate(
                    partctrl->mParticles.size() / (partctrl->mLifetime + partctrl->mLifetimeVariation / 2));

            emitter->setCounter(counter);

            ParticleShooter* shooter = new ParticleShooter(partctrl->mSpeed - partctrl->mSpeedVariation * 0.5f,
                partctrl->mSpeed + partctrl->mSpeedVariation * 0.5f, partctrl->mPlanarAngle,
                partctrl->mPlanarAngleVariation, partctrl->mDeclination, partctrl->mDeclinationVariation,
                partctrl->mLifetime, partctrl->mLifetimeVariation);
            emitter->setShooter(shooter);
            emitter->setFlags(partctrl->mFlags);

            if (partctrl->recType == Nif::RC_NiBSPArrayController && partctrl->emitAtVertex())
            {
                emitter->setGeometryEmitterTarget(partctrl->mEmitter->recIndex);
            }
            else
            {
                osgParticle::BoxPlacer* placer = new osgParticle::BoxPlacer;
                placer->setXRange(-partctrl->mEmitterDimensions.x() / 2.f, partctrl->mEmitterDimensions.x() / 2.f);
                placer->setYRange(-partctrl->mEmitterDimensions.y() / 2.f, partctrl->mEmitterDimensions.y() / 2.f);
                placer->setZRange(-partctrl->mEmitterDimensions.z() / 2.f, partctrl->mEmitterDimensions.z() / 2.f);
                emitter->setPlacer(placer);
            }

            return emitter;
        }

        void handleQueuedParticleEmitters(osg::Group* rootNode, Nif::FileView nif)
        {
            for (const auto& emitterPair : mEmitterQueue)
            {
                size_t recIndex = emitterPair.first;
                FindGroupByRecIndex findEmitterNode(recIndex);
                rootNode->accept(findEmitterNode);
                osg::Group* emitterNode = findEmitterNode.mFound;
                if (!emitterNode)
                {
                    Log(Debug::Warning)
                        << "NIFFile Warning: Failed to find particle emitter emitter node (node record index "
                        << recIndex << "). File: " << nif.getFilename();
                    continue;
                }

                // Emitter attached to the emitter node. Note one side effect of the emitter using the CullVisitor is
                // that hiding its node actually causes the emitter to stop firing. Convenient, because MW behaves this
                // way too!
                emitterNode->addChild(emitterPair.second);

                DisableOptimizer disableOptimizer;
                emitterNode->accept(disableOptimizer);
            }
            mEmitterQueue.clear();
        }

        void handleParticleSystem(const Nif::NiAVObject* nifNode, const Nif::Parent* parent, osg::Group* parentNode,
            SceneUtil::CompositeStateSetUpdater* composite, int animflags)
        {
            osg::ref_ptr<ParticleSystem> partsys(new ParticleSystem);
            partsys->setSortMode(osgParticle::ParticleSystem::SORT_BACK_TO_FRONT);

            const Nif::NiParticleSystemController* partctrl = nullptr;
            for (Nif::NiTimeControllerPtr ctrl = nifNode->mController; !ctrl.empty(); ctrl = ctrl->mNext)
            {
                if (!ctrl->isActive())
                    continue;
                if (ctrl->recType == Nif::RC_NiParticleSystemController
                    || ctrl->recType == Nif::RC_NiBSPArrayController)
                    partctrl = static_cast<Nif::NiParticleSystemController*>(ctrl.getPtr());
            }
            if (!partctrl)
            {
                Log(Debug::Info) << "No particle controller found in " << mFilename;
                return;
            }

            osgParticle::ParticleProcessor::ReferenceFrame rf = (animflags & Nif::NiNode::ParticleFlag_LocalSpace)
                ? osgParticle::ParticleProcessor::RELATIVE_RF
                : osgParticle::ParticleProcessor::ABSOLUTE_RF;

            // HACK: ParticleSystem has no setReferenceFrame method
            if (rf == osgParticle::ParticleProcessor::ABSOLUTE_RF)
            {
                partsys->getOrCreateUserDataContainer()->addDescription("worldspace");
            }

            partsys->setParticleScaleReferenceFrame(osgParticle::ParticleSystem::LOCAL_COORDINATES);

            handleParticleInitialState(nifNode, partsys, partctrl);

            partsys->getDefaultParticleTemplate().setSizeRange(
                osgParticle::rangef(partctrl->mInitialSize, partctrl->mInitialSize));
            partsys->getDefaultParticleTemplate().setColorRange(
                osgParticle::rangev4(partctrl->mInitialColor, partctrl->mInitialColor));
            partsys->getDefaultParticleTemplate().setAlphaRange(osgParticle::rangef(1.f, 1.f));

            if (!partctrl->mEmitter.empty())
            {
                osg::ref_ptr<Emitter> emitter = handleParticleEmitter(partctrl);
                emitter->setParticleSystem(partsys);
                emitter->setReferenceFrame(osgParticle::ParticleProcessor::RELATIVE_RF);

                // The emitter node may not actually be handled yet, so let's delay attaching the emitter to a later
                // moment. If the emitter node is placed later than the particle node, it'll have a single frame delay
                // in particle processing. But that shouldn't be a game-breaking issue.
                mEmitterQueue.emplace_back(partctrl->mEmitter->recIndex, emitter);

                osg::ref_ptr<ParticleSystemController> callback(new ParticleSystemController(partctrl));
                setupController(partctrl, callback, animflags);
                emitter->setUpdateCallback(callback);

                if (!(animflags & Nif::NiNode::ParticleFlag_AutoPlay))
                {
                    partsys->setFrozen(true);
                }

                // Due to odd code in the ParticleSystemUpdater, particle systems will not be updated in the first frame
                // So do that update manually
                osg::NodeVisitor nv;
                partsys->update(0.0, nv);
            }

            // modifiers should be attached *after* the emitter in the scene graph for correct update order
            // attach to same node as the ParticleSystem, we need osgParticle Operators to get the correct
            // localToWorldMatrix for transforming to particle space
            handleParticlePrograms(partctrl->mModifier, partctrl->mCollider, parentNode, partsys.get(), rf);

            std::vector<const Nif::NiProperty*> drawableProps;
            collectDrawableProperties(nifNode, parent, drawableProps);
            applyDrawableProperties(parentNode, drawableProps, composite, true, animflags);

            // particle system updater (after the emitters and modifiers in the scene graph)
            // I think for correct culling needs to be *before* the ParticleSystem, though osg examples do it the other
            // way
            osg::ref_ptr<osgParticle::ParticleSystemUpdater> updater = new osgParticle::ParticleSystemUpdater;
            updater->addParticleSystem(partsys);
            parentNode->addChild(updater);

            osg::Node* toAttach = partsys.get();

            if (rf == osgParticle::ParticleProcessor::RELATIVE_RF)
                parentNode->addChild(toAttach);
            else
            {
                osg::MatrixTransform* trans = new osg::MatrixTransform;
                trans->setUpdateCallback(new InverseWorldMatrix);
                trans->addChild(toAttach);
                parentNode->addChild(trans);
            }
        }

        void handleNiGeometryData(const Nif::NiAVObject* nifNode, const Nif::Parent* parent, osg::Geometry* geometry,
            osg::Node* parentNode, SceneUtil::CompositeStateSetUpdater* composite,
            const std::vector<unsigned int>& boundTextures, int animflags)
        {
            const Nif::NiGeometry* niGeometry = static_cast<const Nif::NiGeometry*>(nifNode);
            if (niGeometry->mData.empty())
                return;

            bool hasPartitions = false;
            if (!niGeometry->mSkin.empty())
            {
                const Nif::NiSkinInstance* skin = niGeometry->mSkin.getPtr();
                const Nif::NiSkinPartition* partitions = skin->getPartitions();
                hasPartitions = partitions != nullptr;
                if (hasPartitions)
                {
                    for (const Nif::NiSkinPartition::Partition& partition : partitions->mPartitions)
                    {
                        const std::vector<unsigned short>& trueTriangles = partition.mTrueTriangles;
                        if (!trueTriangles.empty())
                        {
                            geometry->addPrimitiveSet(new osg::DrawElementsUShort(
                                osg::PrimitiveSet::TRIANGLES, trueTriangles.size(), trueTriangles.data()));
                        }
                        for (const auto& strip : partition.mTrueStrips)
                        {
                            if (strip.size() < 3)
                                continue;
                            geometry->addPrimitiveSet(new osg::DrawElementsUShort(
                                osg::PrimitiveSet::TRIANGLE_STRIP, strip.size(), strip.data()));
                        }
                    }
                }
            }

            const Nif::NiGeometryData* niGeometryData = niGeometry->mData.getPtr();
            if (!hasPartitions)
            {
                if (niGeometry->recType == Nif::RC_NiTriShape || nifNode->recType == Nif::RC_BSLODTriShape)
                {
                    auto data = static_cast<const Nif::NiTriShapeData*>(niGeometryData);
                    const std::vector<unsigned short>& triangles = data->mTriangles;
                    if (triangles.empty())
                        return;
                    geometry->addPrimitiveSet(
                        new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLES, triangles.size(), triangles.data()));
                }
                else if (niGeometry->recType == Nif::RC_NiTriStrips)
                {
                    auto data = static_cast<const Nif::NiTriStripsData*>(niGeometryData);
                    bool hasGeometry = false;
                    for (const std::vector<unsigned short>& strip : data->mStrips)
                    {
                        if (strip.size() < 3)
                            continue;
                        geometry->addPrimitiveSet(
                            new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLE_STRIP, strip.size(), strip.data()));
                        hasGeometry = true;
                    }
                    if (!hasGeometry)
                        return;
                }
                else if (niGeometry->recType == Nif::RC_NiLines)
                {
                    auto data = static_cast<const Nif::NiLinesData*>(niGeometryData);
                    const auto& line = data->mLines;
                    if (line.empty())
                        return;
                    geometry->addPrimitiveSet(
                        new osg::DrawElementsUShort(osg::PrimitiveSet::LINES, line.size(), line.data()));
                }
            }

            const auto& vertices = niGeometryData->mVertices;
            const auto& normals = niGeometryData->mNormals;
            const auto& colors = niGeometryData->mColors;
            if (!vertices.empty())
                geometry->setVertexArray(new osg::Vec3Array(vertices.size(), vertices.data()));
            if (!normals.empty())
                geometry->setNormalArray(
                    new osg::Vec3Array(normals.size(), normals.data()), osg::Array::BIND_PER_VERTEX);
            if (!colors.empty())
                geometry->setColorArray(new osg::Vec4Array(colors.size(), colors.data()), osg::Array::BIND_PER_VERTEX);

            const auto& uvlist = niGeometryData->mUVList;
            int textureStage = 0;
            for (std::vector<unsigned int>::const_iterator it = boundTextures.begin(); it != boundTextures.end();
                 ++it, ++textureStage)
            {
                unsigned int uvSet = *it;
                if (uvSet >= uvlist.size())
                {
                    Log(Debug::Verbose) << "Out of bounds UV set " << uvSet << " on shape \"" << nifNode->mName
                                        << "\" in " << mFilename;
                    if (uvlist.empty())
                        continue;
                    uvSet = 0;
                }

                geometry->setTexCoordArray(textureStage, new osg::Vec2Array(uvlist[uvSet].size(), uvlist[uvSet].data()),
                    osg::Array::BIND_PER_VERTEX);
            }

            // osg::Material properties are handled here for two reasons:
            // - if there are no vertex colors, we need to disable colorMode.
            // - there are 3 "overlapping" nif properties that all affect the osg::Material, handling them
            //   above the actual renderable would be tedious.
            std::vector<const Nif::NiProperty*> drawableProps;
            collectDrawableProperties(nifNode, parent, drawableProps);
            if (!niGeometry->mShaderProperty.empty())
                drawableProps.emplace_back(niGeometry->mShaderProperty.getPtr());
            if (!niGeometry->mAlphaProperty.empty())
                drawableProps.emplace_back(niGeometry->mAlphaProperty.getPtr());
            applyDrawableProperties(parentNode, drawableProps, composite, !niGeometryData->mColors.empty(), animflags);
        }

        void handleNiGeometry(const Nif::NiAVObject* nifNode, const Nif::Parent* parent, osg::Group* parentNode,
            SceneUtil::CompositeStateSetUpdater* composite, const std::vector<unsigned int>& boundTextures,
            int animflags)
        {
            assert(isTypeNiGeometry(nifNode->recType));

            osg::ref_ptr<osg::Geometry> geom(new osg::Geometry);
            handleNiGeometryData(nifNode, parent, geom, parentNode, composite, boundTextures, animflags);
            // If the record had no valid geometry data in it, early-out
            if (geom->empty())
                return;

            osg::ref_ptr<osg::Drawable> drawable = geom;

            auto niGeometry = static_cast<const Nif::NiGeometry*>(nifNode);
            if (!niGeometry->mSkin.empty())
            {
                osg::ref_ptr<SceneUtil::RigGeometry> rig(new SceneUtil::RigGeometry);
                rig->setSourceGeometry(geom);

                const Nif::NiSkinInstance* skin = niGeometry->mSkin.getPtr();
                const Nif::NiSkinData* data = skin->mData.getPtr();
                const Nif::NiAVObjectList& bones = skin->mBones;

                // Assign bone weights
                std::vector<SceneUtil::RigGeometry::BoneInfo> boneInfo;
                std::vector<SceneUtil::RigGeometry::VertexWeights> influences;
                boneInfo.resize(bones.size());
                influences.resize(bones.size());
                for (std::size_t i = 0; i < bones.size(); ++i)
                {
                    boneInfo[i].mName = Misc::StringUtils::lowerCase(bones[i].getPtr()->mName);
                    boneInfo[i].mInvBindMatrix = data->mBones[i].mTransform.toMatrix();
                    boneInfo[i].mBoundSphere = data->mBones[i].mBoundSphere;
                    influences[i] = data->mBones[i].mWeights;
                }
                rig->setBoneInfo(std::move(boneInfo));
                rig->setInfluences(influences);

                drawable = rig;
            }

            for (Nif::NiTimeControllerPtr ctrl = nifNode->mController; !ctrl.empty(); ctrl = ctrl->mNext)
            {
                if (!ctrl->isActive())
                    continue;
                if (ctrl->recType == Nif::RC_NiGeomMorpherController)
                {
                    if (!niGeometry->mSkin.empty())
                        continue;

                    auto nimorphctrl = static_cast<const Nif::NiGeomMorpherController*>(ctrl.getPtr());
                    if (nimorphctrl->mData.empty())
                        continue;

                    const std::vector<Nif::NiMorphData::MorphData>& morphs = nimorphctrl->mData.getPtr()->mMorphs;
                    if (morphs.empty()
                        || morphs[0].mVertices.size()
                            != static_cast<const osg::Vec3Array*>(geom->getVertexArray())->size())
                        continue;

                    osg::ref_ptr<SceneUtil::MorphGeometry> morphGeom = new SceneUtil::MorphGeometry;
                    morphGeom->setSourceGeometry(geom);
                    for (unsigned int i = 0; i < morphs.size(); ++i)
                        morphGeom->addMorphTarget(
                            new osg::Vec3Array(morphs[i].mVertices.size(), morphs[i].mVertices.data()), 0.f);

                    osg::ref_ptr<GeomMorpherController> morphctrl = new GeomMorpherController(nimorphctrl);
                    setupController(ctrl.getPtr(), morphctrl, animflags);
                    morphGeom->setUpdateCallback(morphctrl);

                    drawable = morphGeom;
                    break;
                }
            }

            drawable->setName(nifNode->mName);
            parentNode->addChild(drawable);
        }

        void handleBSGeometry(const Nif::NiAVObject* nifNode, const Nif::Parent* parent, osg::Group* parentNode,
            SceneUtil::CompositeStateSetUpdater* composite, const std::vector<unsigned int>& boundTextures,
            int animflags)
        {
            assert(isTypeBSGeometry(nifNode->recType));

            auto bsTriShape = static_cast<const Nif::BSTriShape*>(nifNode);
            const std::vector<unsigned short>& triangles = bsTriShape->mTriangles;
            if (triangles.empty())
                return;

            osg::ref_ptr<osg::Geometry> geometry(new osg::Geometry);
            geometry->addPrimitiveSet(
                new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLES, triangles.size(), triangles.data()));

            osg::ref_ptr<osg::Drawable> drawable = geometry;

            // Some input geometry may not be used as is so it needs to be converted.
            // Normals, tangents and bitangents use a special normal map-like format not equivalent to snorm8 or unorm8
            auto normbyteToFloat = [](uint8_t value) { return value / 255.f * 2.f - 1.f; };
            // Vertices and UV sets may be half-precision.
            // OSG doesn't have a way to pass half-precision data at the moment.
            auto halfToFloat = [](uint16_t value) {
                uint32_t bits = static_cast<uint32_t>(value & 0x8000) << 16;

                const uint32_t exp16 = (value & 0x7c00) >> 10;
                uint32_t frac16 = value & 0x3ff;
                if (exp16)
                    bits |= (exp16 + 0x70) << 23;
                else if (frac16)
                {
                    uint8_t offset = 0;
                    do
                    {
                        ++offset;
                        frac16 <<= 1;
                    } while ((frac16 & 0x400) != 0x400);
                    frac16 &= 0x3ff;
                    bits |= (0x71 - offset) << 23;
                }
                bits |= frac16 << 13;

                float result;
                std::memcpy(&result, &bits, sizeof(float));
                return result;
            };

            const bool fullPrec = bsTriShape->mVertDesc.mFlags & Nif::BSVertexDesc::VertexAttribute::Full_Precision;
            const bool hasVertices = bsTriShape->mVertDesc.mFlags & Nif::BSVertexDesc::VertexAttribute::Vertex;
            const bool hasNormals = bsTriShape->mVertDesc.mFlags & Nif::BSVertexDesc::VertexAttribute::Normals;
            const bool hasColors = bsTriShape->mVertDesc.mFlags & Nif::BSVertexDesc::VertexAttribute::Vertex_Colors;
            const bool hasUV = bsTriShape->mVertDesc.mFlags & Nif::BSVertexDesc::VertexAttribute::UVs;

            std::vector<osg::Vec3f> vertices;
            std::vector<osg::Vec3f> normals;
            std::vector<osg::Vec4ub> colors;
            std::vector<osg::Vec2f> uvlist;
            for (auto& elem : bsTriShape->mVertData)
            {
                if (hasVertices)
                {
                    if (fullPrec)
                        vertices.emplace_back(elem.mVertex.x(), elem.mVertex.y(), elem.mVertex.z());
                    else
                        vertices.emplace_back(halfToFloat(elem.mHalfVertex[0]), halfToFloat(elem.mHalfVertex[1]),
                            halfToFloat(elem.mHalfVertex[2]));
                }
                if (hasNormals)
                    normals.emplace_back(normbyteToFloat(elem.mNormal[0]), normbyteToFloat(elem.mNormal[1]),
                        normbyteToFloat(elem.mNormal[2]));
                if (hasColors)
                    colors.emplace_back(elem.mVertColor[0], elem.mVertColor[1], elem.mVertColor[2], elem.mVertColor[3]);
                if (hasUV)
                    uvlist.emplace_back(halfToFloat(elem.mUV[0]), 1.0 - halfToFloat(elem.mUV[1]));
            }

            if (!vertices.empty())
                geometry->setVertexArray(new osg::Vec3Array(vertices.size(), vertices.data()));
            if (!normals.empty())
                geometry->setNormalArray(
                    new osg::Vec3Array(normals.size(), normals.data()), osg::Array::BIND_PER_VERTEX);
            if (!colors.empty())
                geometry->setColorArray(
                    new osg::Vec4ubArray(colors.size(), colors.data()), osg::Array::BIND_PER_VERTEX);
            if (!uvlist.empty())
                geometry->setTexCoordArray(
                    0, new osg::Vec2Array(uvlist.size(), uvlist.data()), osg::Array::BIND_PER_VERTEX);

            // This is the skinning data Fallout 4 provides
            // TODO: support Skyrim SE skinning data
            if (!bsTriShape->mSkin.empty() && bsTriShape->mSkin->recType == Nif::RC_BSSkinInstance
                && bsTriShape->mVertDesc.mFlags & Nif::BSVertexDesc::VertexAttribute::Skinned)
            {
                osg::ref_ptr<SceneUtil::RigGeometry> rig(new SceneUtil::RigGeometry);
                rig->setSourceGeometry(std::move(geometry));

                const Nif::BSSkinInstance* skin = static_cast<const Nif::BSSkinInstance*>(bsTriShape->mSkin.getPtr());
                const Nif::BSSkinBoneData* data = skin->mData.getPtr();
                const Nif::NiAVObjectList& bones = skin->mBones;

                std::vector<SceneUtil::RigGeometry::BoneInfo> boneInfo;
                std::vector<SceneUtil::RigGeometry::BoneWeights> influences;
                boneInfo.resize(bones.size());
                influences.resize(vertices.size());
                for (std::size_t i = 0; i < bones.size(); ++i)
                {
                    boneInfo[i].mName = Misc::StringUtils::lowerCase(bones[i].getPtr()->mName);
                    boneInfo[i].mInvBindMatrix = data->mBones[i].mTransform.toMatrix();
                    boneInfo[i].mBoundSphere = data->mBones[i].mBoundSphere;
                }

                for (size_t i = 0; i < vertices.size(); i++)
                {
                    const Nif::BSVertexData& vertData = bsTriShape->mVertData[i];
                    for (int j = 0; j < 4; j++)
                        influences[i].emplace_back(vertData.mBoneIndices[j], halfToFloat(vertData.mBoneWeights[j]));
                }
                rig->setBoneInfo(std::move(boneInfo));
                rig->setInfluences(influences);

                drawable = rig;
            }

            std::vector<const Nif::NiProperty*> drawableProps;
            collectDrawableProperties(nifNode, parent, drawableProps);
            if (!bsTriShape->mShaderProperty.empty())
                drawableProps.emplace_back(bsTriShape->mShaderProperty.getPtr());
            if (!bsTriShape->mAlphaProperty.empty())
                drawableProps.emplace_back(bsTriShape->mAlphaProperty.getPtr());
            applyDrawableProperties(parentNode, drawableProps, composite, !colors.empty(), animflags);

            drawable->setName(nifNode->mName);
            parentNode->addChild(drawable);
        }

        osg::BlendFunc::BlendFuncMode getBlendMode(int mode) const
        {
            switch (mode)
            {
                case 0:
                    return osg::BlendFunc::ONE;
                case 1:
                    return osg::BlendFunc::ZERO;
                case 2:
                    return osg::BlendFunc::SRC_COLOR;
                case 3:
                    return osg::BlendFunc::ONE_MINUS_SRC_COLOR;
                case 4:
                    return osg::BlendFunc::DST_COLOR;
                case 5:
                    return osg::BlendFunc::ONE_MINUS_DST_COLOR;
                case 6:
                    return osg::BlendFunc::SRC_ALPHA;
                case 7:
                    return osg::BlendFunc::ONE_MINUS_SRC_ALPHA;
                case 8:
                    return osg::BlendFunc::DST_ALPHA;
                case 9:
                    return osg::BlendFunc::ONE_MINUS_DST_ALPHA;
                case 10:
                    return osg::BlendFunc::SRC_ALPHA_SATURATE;
                default:
                    Log(Debug::Info) << "Unexpected blend mode: " << mode << " in " << mFilename;
                    return osg::BlendFunc::SRC_ALPHA;
            }
        }

        osg::AlphaFunc::ComparisonFunction getTestMode(int mode) const
        {
            switch (mode)
            {
                case 0:
                    return osg::AlphaFunc::ALWAYS;
                case 1:
                    return osg::AlphaFunc::LESS;
                case 2:
                    return osg::AlphaFunc::EQUAL;
                case 3:
                    return osg::AlphaFunc::LEQUAL;
                case 4:
                    return osg::AlphaFunc::GREATER;
                case 5:
                    return osg::AlphaFunc::NOTEQUAL;
                case 6:
                    return osg::AlphaFunc::GEQUAL;
                case 7:
                    return osg::AlphaFunc::NEVER;
                default:
                    Log(Debug::Info) << "Unexpected blend mode: " << mode << " in " << mFilename;
                    return osg::AlphaFunc::LEQUAL;
            }
        }

        osg::Stencil::Function getStencilFunction(Nif::NiStencilProperty::TestFunc func) const
        {
            using TestFunc = Nif::NiStencilProperty::TestFunc;
            switch (func)
            {
                case TestFunc::Never:
                    return osg::Stencil::NEVER;
                case TestFunc::Less:
                    return osg::Stencil::LESS;
                case TestFunc::Equal:
                    return osg::Stencil::EQUAL;
                case TestFunc::LessEqual:
                    return osg::Stencil::LEQUAL;
                case TestFunc::Greater:
                    return osg::Stencil::GREATER;
                case TestFunc::NotEqual:
                    return osg::Stencil::NOTEQUAL;
                case TestFunc::GreaterEqual:
                    return osg::Stencil::GEQUAL;
                case TestFunc::Always:
                    return osg::Stencil::ALWAYS;
                default:
                    Log(Debug::Info) << "Unexpected stencil function: " << static_cast<uint32_t>(func) << " in "
                                     << mFilename;
                    return osg::Stencil::NEVER;
            }
        }

        osg::Stencil::Operation getStencilOperation(Nif::NiStencilProperty::Action op) const
        {
            using Action = Nif::NiStencilProperty::Action;
            switch (op)
            {
                case Action::Keep:
                    return osg::Stencil::KEEP;
                case Action::Zero:
                    return osg::Stencil::ZERO;
                case Action::Replace:
                    return osg::Stencil::REPLACE;
                case Action::Increment:
                    return osg::Stencil::INCR;
                case Action::Decrement:
                    return osg::Stencil::DECR;
                case Action::Invert:
                    return osg::Stencil::INVERT;
                default:
                    Log(Debug::Info) << "Unexpected stencil operation: " << static_cast<uint32_t>(op) << " in "
                                     << mFilename;
                    return osg::Stencil::KEEP;
            }
        }

        osg::ref_ptr<osg::Image> handleInternalTexture(const Nif::NiPixelData* pixelData) const
        {
            if (pixelData->mMipmaps.empty())
                return nullptr;

            // Not fatal, but warn the user
            if (pixelData->mNumFaces != 1)
                Log(Debug::Info) << "Unsupported multifaceted internal texture in " << mFilename;

            using Nif::NiPixelFormat;
            NiPixelFormat niPixelFormat = pixelData->mPixelFormat;
            GLenum pixelformat = 0;
            // Pixel row alignment. Defining it to be consistent with OSG DDS plugin
            int packing = 1;
            switch (niPixelFormat.mFormat)
            {
                case NiPixelFormat::Format::RGB:
                    pixelformat = GL_RGB;
                    break;
                case NiPixelFormat::Format::RGBA:
                    pixelformat = GL_RGBA;
                    break;
                case NiPixelFormat::Format::Palette:
                case NiPixelFormat::Format::PaletteAlpha:
                    pixelformat = GL_RED; // Each color is defined by a byte.
                    break;
                case NiPixelFormat::Format::BGR:
                    pixelformat = GL_BGR;
                    break;
                case NiPixelFormat::Format::BGRA:
                    pixelformat = GL_BGRA;
                    break;
                case NiPixelFormat::Format::DXT1:
                    pixelformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                    packing = 2;
                    break;
                case NiPixelFormat::Format::DXT3:
                    pixelformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                    packing = 4;
                    break;
                case NiPixelFormat::Format::DXT5:
                    pixelformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                    packing = 4;
                    break;
                default:
                    Log(Debug::Info) << "Unhandled internal pixel format "
                                     << static_cast<uint32_t>(niPixelFormat.mFormat) << " in " << mFilename;
                    return nullptr;
            }

            int width = 0;
            int height = 0;

            std::vector<unsigned int> mipmapOffsets;
            for (unsigned int i = 0; i < pixelData->mMipmaps.size(); ++i)
            {
                const Nif::NiPixelData::Mipmap& mip = pixelData->mMipmaps[i];

                size_t mipSize = osg::Image::computeImageSizeInBytes(
                    mip.mWidth, mip.mHeight, 1, pixelformat, GL_UNSIGNED_BYTE, packing);
                if (mipSize + mip.mOffset > pixelData->mData.size())
                {
                    Log(Debug::Info) << "Internal texture's mipmap data out of bounds, ignoring texture";
                    return nullptr;
                }

                if (i != 0)
                    mipmapOffsets.push_back(mip.mOffset);
                else
                {
                    width = mip.mWidth;
                    height = mip.mHeight;
                }
            }

            if (width <= 0 || height <= 0)
            {
                Log(Debug::Info) << "Internal Texture Width and height must be non zero, ignoring texture";
                return nullptr;
            }

            osg::ref_ptr<osg::Image> image(new osg::Image);
            const std::vector<unsigned char>& pixels = pixelData->mData;
            switch (niPixelFormat.mFormat)
            {
                case NiPixelFormat::Format::RGB:
                case NiPixelFormat::Format::RGBA:
                case NiPixelFormat::Format::BGR:
                case NiPixelFormat::Format::BGRA:
                case NiPixelFormat::Format::DXT1:
                case NiPixelFormat::Format::DXT3:
                case NiPixelFormat::Format::DXT5:
                {
                    unsigned char* data = new unsigned char[pixels.size()];
                    memcpy(data, pixels.data(), pixels.size());
                    image->setImage(width, height, 1, pixelformat, pixelformat, GL_UNSIGNED_BYTE, data,
                        osg::Image::USE_NEW_DELETE, packing);
                    break;
                }
                case NiPixelFormat::Format::Palette:
                case NiPixelFormat::Format::PaletteAlpha:
                {
                    if (pixelData->mPalette.empty() || niPixelFormat.mBitsPerPixel != 8)
                    {
                        Log(Debug::Info) << "Palettized texture in " << mFilename << " is invalid, ignoring";
                        return nullptr;
                    }
                    pixelformat = niPixelFormat.mFormat == NiPixelFormat::Format::PaletteAlpha ? GL_RGBA : GL_RGB;
                    // We're going to convert the indices that pixel data contains
                    // into real colors using the palette.
                    const auto& palette = pixelData->mPalette->mColors;
                    const int numChannels = pixelformat == GL_RGBA ? 4 : 3;
                    unsigned char* data = new unsigned char[pixels.size() * numChannels];
                    unsigned char* pixel = data;
                    for (unsigned char index : pixels)
                    {
                        memcpy(pixel, &palette[index], sizeof(unsigned char) * numChannels);
                        pixel += numChannels;
                    }
                    for (unsigned int& offset : mipmapOffsets)
                        offset *= numChannels;
                    image->setImage(width, height, 1, pixelformat, pixelformat, GL_UNSIGNED_BYTE, data,
                        osg::Image::USE_NEW_DELETE, packing);
                    break;
                }
                default:
                    return nullptr;
            }

            image->setMipmapLevels(mipmapOffsets);
            image->flipVertical();

            return image;
        }

        static osg::ref_ptr<osg::TexEnvCombine> createEmissiveTexEnv()
        {
            osg::ref_ptr<osg::TexEnvCombine> texEnv(new osg::TexEnvCombine);
            // Sum the previous colour and the emissive colour.
            texEnv->setCombine_RGB(osg::TexEnvCombine::ADD);
            texEnv->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
            texEnv->setSource1_RGB(osg::TexEnvCombine::TEXTURE);
            // Keep the previous alpha.
            texEnv->setCombine_Alpha(osg::TexEnvCombine::REPLACE);
            texEnv->setSource0_Alpha(osg::TexEnvCombine::PREVIOUS);
            texEnv->setOperand0_Alpha(osg::TexEnvCombine::SRC_ALPHA);
            return texEnv;
        }

        static void handleDepthFlags(osg::StateSet* stateset, bool depthTest, bool depthWrite)
        {
            if (!depthWrite && !depthTest)
            {
                stateset->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
                return;
            }
            osg::ref_ptr<osg::Depth> depth = new SceneUtil::AutoDepth;
            depth->setWriteMask(depthWrite);
            if (!depthTest)
                depth->setFunction(osg::Depth::ALWAYS);
            depth = shareAttribute(depth);
            stateset->setAttributeAndModes(depth, osg::StateAttribute::ON);
        }

        void handleTextureProperty(const Nif::NiTexturingProperty* texprop, const std::string& nodeName,
            osg::StateSet* stateset, SceneUtil::CompositeStateSetUpdater* composite,
            std::vector<unsigned int>& boundTextures, int animflags) const
        {
            // overriding a parent NiTexturingProperty, so remove what was previously bound
            clearBoundTextures(stateset, boundTextures);

            // If this loop is changed such that the base texture isn't guaranteed to end up in texture unit 0, the
            // shadow casting shader will need to be updated accordingly.
            for (size_t i = 0; i < texprop->mTextures.size(); ++i)
            {
                const Nif::NiTexturingProperty::Texture& tex = texprop->mTextures[i];
                if (tex.mEnabled || (i == Nif::NiTexturingProperty::BaseTexture && !texprop->mController.empty()))
                {
                    std::string textureName;
                    switch (i)
                    {
                        // These are handled later on
                        case Nif::NiTexturingProperty::BaseTexture:
                            textureName = "diffuseMap";
                            break;
                        case Nif::NiTexturingProperty::GlowTexture:
                            textureName = "emissiveMap";
                            break;
                        case Nif::NiTexturingProperty::DarkTexture:
                            textureName = "darkMap";
                            break;
                        case Nif::NiTexturingProperty::BumpTexture:
                            textureName = "bumpMap";
                            break;
                        case Nif::NiTexturingProperty::DetailTexture:
                            textureName = "detailMap";
                            break;
                        case Nif::NiTexturingProperty::DecalTexture:
                            textureName = "decalMap";
                            break;
                        case Nif::NiTexturingProperty::GlossTexture:
                            textureName = "glossMap";
                            break;
                        default:
                        {
                            Log(Debug::Info) << "Unhandled texture stage " << i << " on shape \"" << nodeName
                                             << "\" in " << mFilename;
                            continue;
                        }
                    }

                    const unsigned int texUnit = boundTextures.size();
                    if (tex.mEnabled)
                    {
                        if (tex.mSourceTexture.empty() && texprop->mController.empty())
                        {
                            if (i == 0)
                                Log(Debug::Warning) << "Base texture is in use but empty on shape \"" << nodeName
                                                    << "\" in " << mFilename;
                            continue;
                        }

                        if (!tex.mSourceTexture.empty())
                            attachNiSourceTexture(textureName, tex.mSourceTexture.getPtr(), tex.wrapS(), tex.wrapT(),
                                tex.mUVSet, stateset, boundTextures);
                        else
                            attachTexture(
                                textureName, nullptr, tex.wrapS(), tex.wrapT(), tex.mUVSet, stateset, boundTextures);
                    }
                    else
                    {
                        // Texture only comes from NiFlipController, so tex is ignored, set defaults
                        attachTexture(textureName, nullptr, true, true, 0, stateset, boundTextures);
                    }

                    if (i == Nif::NiTexturingProperty::GlowTexture)
                    {
                        stateset->setTextureAttributeAndModes(texUnit, createEmissiveTexEnv(), osg::StateAttribute::ON);
                    }
                    else if (i == Nif::NiTexturingProperty::DarkTexture)
                    {
                        osg::TexEnv* texEnv = new osg::TexEnv;
                        // Modulate both the colour and the alpha with the dark map.
                        texEnv->setMode(osg::TexEnv::MODULATE);
                        stateset->setTextureAttributeAndModes(texUnit, texEnv, osg::StateAttribute::ON);
                    }
                    else if (i == Nif::NiTexturingProperty::DetailTexture)
                    {
                        osg::TexEnvCombine* texEnv = new osg::TexEnvCombine;
                        // Modulate previous colour...
                        texEnv->setCombine_RGB(osg::TexEnvCombine::MODULATE);
                        texEnv->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
                        texEnv->setOperand0_RGB(osg::TexEnvCombine::SRC_COLOR);
                        // with the detail map's colour,
                        texEnv->setSource1_RGB(osg::TexEnvCombine::TEXTURE);
                        texEnv->setOperand1_RGB(osg::TexEnvCombine::SRC_COLOR);
                        // and a twist:
                        texEnv->setScale_RGB(2.f);
                        // Keep the previous alpha.
                        texEnv->setCombine_Alpha(osg::TexEnvCombine::REPLACE);
                        texEnv->setSource0_Alpha(osg::TexEnvCombine::PREVIOUS);
                        texEnv->setOperand0_Alpha(osg::TexEnvCombine::SRC_ALPHA);
                        stateset->setTextureAttributeAndModes(texUnit, texEnv, osg::StateAttribute::ON);
                    }
                    else if (i == Nif::NiTexturingProperty::BumpTexture)
                    {
                        // Bump maps offset the environment map.
                        // Set this texture to Off by default since we can't render it with the fixed-function pipeline
                        stateset->setTextureMode(texUnit, GL_TEXTURE_2D, osg::StateAttribute::OFF);
                        osg::Matrix2 bumpMapMatrix(texprop->mBumpMapMatrix.x(), texprop->mBumpMapMatrix.y(),
                            texprop->mBumpMapMatrix.z(), texprop->mBumpMapMatrix.w());
                        stateset->addUniform(new osg::Uniform("bumpMapMatrix", bumpMapMatrix));
                        stateset->addUniform(new osg::Uniform("envMapLumaBias", texprop->mEnvMapLumaBias));
                    }
                    else if (i == Nif::NiTexturingProperty::GlossTexture)
                    {
                        // A gloss map is an environment map mask.
                        // Gloss maps are only implemented in the object shaders as well.
                        stateset->setTextureMode(texUnit, GL_TEXTURE_2D, osg::StateAttribute::OFF);
                    }
                    else if (i == Nif::NiTexturingProperty::DecalTexture)
                    {
                        // This is only an inaccurate imitation of the original implementation,
                        // see https://github.com/niftools/nifskope/issues/184

                        osg::TexEnvCombine* texEnv = new osg::TexEnvCombine;
                        // Interpolate to the decal texture's colour...
                        texEnv->setCombine_RGB(osg::TexEnvCombine::INTERPOLATE);
                        texEnv->setSource0_RGB(osg::TexEnvCombine::TEXTURE);
                        texEnv->setOperand0_RGB(osg::TexEnvCombine::SRC_COLOR);
                        // ...from the previous colour...
                        texEnv->setSource1_RGB(osg::TexEnvCombine::PREVIOUS);
                        texEnv->setOperand1_RGB(osg::TexEnvCombine::SRC_COLOR);
                        // using the decal texture's alpha as the factor.
                        texEnv->setSource2_RGB(osg::TexEnvCombine::TEXTURE);
                        texEnv->setOperand2_RGB(osg::TexEnvCombine::SRC_ALPHA);
                        // Keep the previous alpha.
                        texEnv->setCombine_Alpha(osg::TexEnvCombine::REPLACE);
                        texEnv->setSource0_Alpha(osg::TexEnvCombine::PREVIOUS);
                        texEnv->setOperand0_Alpha(osg::TexEnvCombine::SRC_ALPHA);
                        stateset->setTextureAttributeAndModes(texUnit, texEnv, osg::StateAttribute::ON);
                    }
                }
            }
            handleTextureControllers(texprop, composite, stateset, animflags);
        }

        static Bgsm::MaterialFilePtr getShaderMaterial(
            std::string_view path, Resource::BgsmFileManager* materialManager)
        {
            if (!materialManager)
                return nullptr;

            if (!Misc::StringUtils::ciEndsWith(path, ".bgem") && !Misc::StringUtils::ciEndsWith(path, ".bgsm"))
                return nullptr;

            std::string normalizedPath = Misc::ResourceHelpers::correctMaterialPath(path, materialManager->getVFS());
            try
            {
                return materialManager->get(VFS::Path::Normalized(normalizedPath));
            }
            catch (std::exception& e)
            {
                Log(Debug::Error) << "Failed to load shader material: " << e.what();
                return nullptr;
            }
        }

        void handleShaderMaterialNodeProperties(
            const Bgsm::MaterialFile* material, osg::StateSet* stateset, std::vector<unsigned int>& boundTextures) const
        {
            const unsigned int uvSet = 0;
            const bool wrapS = material->wrapS();
            const bool wrapT = material->wrapT();
            if (material->mShaderType == Bgsm::ShaderType::Lighting)
            {
                const Bgsm::BGSMFile* bgsm = static_cast<const Bgsm::BGSMFile*>(material);

                if (!bgsm->mDiffuseMap.empty())
                    attachExternalTexture(
                        "diffuseMap", bgsm->mDiffuseMap, wrapS, wrapT, uvSet, stateset, boundTextures);

                if (!bgsm->mNormalMap.empty())
                    attachExternalTexture("normalMap", bgsm->mNormalMap, wrapS, wrapT, uvSet, stateset, boundTextures);

                if (bgsm->mGlowMapEnabled && !bgsm->mGlowMap.empty())
                    attachExternalTexture("emissiveMap", bgsm->mGlowMap, wrapS, wrapT, uvSet, stateset, boundTextures);

                if (bgsm->mTree)
                    stateset->addUniform(new osg::Uniform("useTreeAnim", true));
            }
            else if (material->mShaderType == Bgsm::ShaderType::Effect)
            {
                const Bgsm::BGEMFile* bgem = static_cast<const Bgsm::BGEMFile*>(material);

                if (!bgem->mBaseMap.empty())
                    attachExternalTexture("diffuseMap", bgem->mBaseMap, wrapS, wrapT, uvSet, stateset, boundTextures);

                bool useFalloff = bgem->mFalloff;
                stateset->addUniform(new osg::Uniform("useFalloff", useFalloff));
                if (useFalloff)
                    stateset->addUniform(new osg::Uniform("falloffParams", bgem->mFalloffParams));
            }

            if (material->mTwoSided)
                stateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
            handleDepthFlags(stateset, material->mDepthTest, material->mDepthWrite);
        }

        void handleDecal(bool enabled, bool hasSortAlpha, osg::Node& node) const
        {
            if (!enabled)
                return;
            osg::ref_ptr<osg::StateSet> stateset = node.getOrCreateStateSet();
            osg::ref_ptr<osg::PolygonOffset> polygonOffset(new osg::PolygonOffset);
            polygonOffset->setUnits(SceneUtil::AutoDepth::isReversed() ? 1.f : -1.f);
            polygonOffset->setFactor(SceneUtil::AutoDepth::isReversed() ? 0.65f : -0.65f);
            polygonOffset = shareAttribute(polygonOffset);
            stateset->setAttributeAndModes(polygonOffset, osg::StateAttribute::ON);
            if (!mPushedSorter && !hasSortAlpha)
                stateset->setRenderBinDetails(1, "SORT_BACK_TO_FRONT");
        }

        static void handleAlphaTesting(
            bool enabled, osg::AlphaFunc::ComparisonFunction function, int threshold, osg::Node& node)
        {
            if (enabled)
            {
                osg::ref_ptr<osg::AlphaFunc> alphaFunc(new osg::AlphaFunc(function, threshold / 255.f));
                alphaFunc = shareAttribute(alphaFunc);
                node.getOrCreateStateSet()->setAttributeAndModes(alphaFunc, osg::StateAttribute::ON);
            }
            else if (osg::StateSet* stateset = node.getStateSet())
            {
                stateset->removeAttribute(osg::StateAttribute::ALPHAFUNC);
                stateset->removeMode(GL_ALPHA_TEST);
            }
        }

        void handleAlphaBlending(
            bool enabled, int sourceMode, int destMode, bool sort, bool& hasSortAlpha, osg::Node& node) const
        {
            if (enabled)
            {
                osg::ref_ptr<osg::StateSet> stateset = node.getOrCreateStateSet();
                osg::ref_ptr<osg::BlendFunc> blendFunc(
                    new osg::BlendFunc(getBlendMode(sourceMode), getBlendMode(destMode)));
                // on AMD hardware, alpha still seems to be stored with an RGBA framebuffer with OpenGL.
                // This might be mandated by the OpenGL 2.1 specification section 2.14.9, or might be a bug.
                // Either way, D3D8.1 doesn't do that, so adapt the destination factor.
                if (blendFunc->getDestination() == GL_DST_ALPHA)
                    blendFunc->setDestination(GL_ONE);
                blendFunc = shareAttribute(blendFunc);
                stateset->setAttributeAndModes(blendFunc, osg::StateAttribute::ON);

                if (sort)
                {
                    hasSortAlpha = true;
                    if (!mPushedSorter)
                        stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
                }
                else if (!mPushedSorter)
                {
                    stateset->setRenderBinToInherit();
                }
            }
            else if (osg::ref_ptr<osg::StateSet> stateset = node.getStateSet())
            {
                stateset->removeAttribute(osg::StateAttribute::BLENDFUNC);
                stateset->removeMode(GL_BLEND);
                if (!mPushedSorter)
                    stateset->setRenderBinToInherit();
            }
        }

        void handleShaderMaterialDrawableProperties(const Bgsm::MaterialFile* shaderMat,
            osg::ref_ptr<osg::Material> mat, osg::Node& node, bool& hasSortAlpha) const
        {
            mat->setAlpha(osg::Material::FRONT_AND_BACK, shaderMat->mTransparency);
            handleAlphaTesting(shaderMat->mAlphaTest, osg::AlphaFunc::GREATER, shaderMat->mAlphaTestThreshold, node);
            handleAlphaBlending(shaderMat->mAlphaBlend, shaderMat->mSourceBlendMode, shaderMat->mDestinationBlendMode,
                true, hasSortAlpha, node);
            handleDecal(shaderMat->mDecal, hasSortAlpha, node);
            if (shaderMat->mShaderType == Bgsm::ShaderType::Lighting)
            {
                auto bgsm = static_cast<const Bgsm::BGSMFile*>(shaderMat);
                mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(bgsm->mEmittanceColor, 1.f));
                mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(bgsm->mSpecularColor, 1.f));
            }
            else if (shaderMat->mShaderType == Bgsm::ShaderType::Effect)
            {
                auto bgem = static_cast<const Bgsm::BGEMFile*>(shaderMat);
                mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(bgem->mEmittanceColor, 1.f));
                if (bgem->mSoft)
                    SceneUtil::setupSoftEffect(node, bgem->mSoftDepth, true, bgem->mSoftDepth);
            }
        }

        void handleTextureSet(const Nif::BSShaderTextureSet* textureSet, bool wrapS, bool wrapT,
            const std::string& nodeName, osg::StateSet* stateset, std::vector<unsigned int>& boundTextures) const
        {
            const unsigned int uvSet = 0;

            for (size_t i = 0; i < textureSet->mTextures.size(); ++i)
            {
                if (textureSet->mTextures[i].empty())
                    continue;
                switch (static_cast<Nif::BSShaderTextureSet::TextureType>(i))
                {
                    case Nif::BSShaderTextureSet::TextureType::Base:
                        attachExternalTexture(
                            "diffuseMap", textureSet->mTextures[i], wrapS, wrapT, uvSet, stateset, boundTextures);
                        break;
                    case Nif::BSShaderTextureSet::TextureType::Normal:
                        attachExternalTexture(
                            "normalMap", textureSet->mTextures[i], wrapS, wrapT, uvSet, stateset, boundTextures);
                        break;
                    case Nif::BSShaderTextureSet::TextureType::Glow:
                        attachExternalTexture(
                            "emissiveMap", textureSet->mTextures[i], wrapS, wrapT, uvSet, stateset, boundTextures);
                        break;
                    default:
                    {
                        Log(Debug::Info) << "Unhandled texture stage " << i << " on shape \"" << nodeName << "\" in "
                                         << mFilename;
                        continue;
                    }
                }
            }
        }

        std::string_view getBSShaderPrefix(unsigned int type) const
        {
            switch (static_cast<Nif::BSShaderType>(type))
            {
                case Nif::BSShaderType::ShaderType_Default:
                    return "bs/default";
                case Nif::BSShaderType::ShaderType_NoLighting:
                    return "bs/nolighting";
                case Nif::BSShaderType::ShaderType_TallGrass:
                case Nif::BSShaderType::ShaderType_Sky:
                case Nif::BSShaderType::ShaderType_Skin:
                case Nif::BSShaderType::ShaderType_Water:
                case Nif::BSShaderType::ShaderType_Lighting30:
                case Nif::BSShaderType::ShaderType_Tile:
                    Log(Debug::Warning) << "Unhandled BSShaderType " << type << " in " << mFilename;
                    return "bs/default";
            }
            Log(Debug::Warning) << "Unknown BSShaderType " << type << " in " << mFilename;
            return "bs/default";
        }

        std::string_view getBSLightingShaderPrefix(unsigned int type) const
        {
            switch (static_cast<Nif::BSLightingShaderType>(type))
            {
                case Nif::BSLightingShaderType::ShaderType_Default:
                    return "bs/default";
                case Nif::BSLightingShaderType::ShaderType_EnvMap:
                case Nif::BSLightingShaderType::ShaderType_Glow:
                case Nif::BSLightingShaderType::ShaderType_Parallax:
                case Nif::BSLightingShaderType::ShaderType_FaceTint:
                case Nif::BSLightingShaderType::ShaderType_SkinTint:
                case Nif::BSLightingShaderType::ShaderType_HairTint:
                case Nif::BSLightingShaderType::ShaderType_ParallaxOcc:
                case Nif::BSLightingShaderType::ShaderType_MultitexLand:
                case Nif::BSLightingShaderType::ShaderType_LODLand:
                case Nif::BSLightingShaderType::ShaderType_Snow:
                case Nif::BSLightingShaderType::ShaderType_MultiLayerParallax:
                case Nif::BSLightingShaderType::ShaderType_TreeAnim:
                case Nif::BSLightingShaderType::ShaderType_LODObjects:
                case Nif::BSLightingShaderType::ShaderType_SparkleSnow:
                case Nif::BSLightingShaderType::ShaderType_LODObjectsHD:
                case Nif::BSLightingShaderType::ShaderType_EyeEnvmap:
                case Nif::BSLightingShaderType::ShaderType_Cloud:
                case Nif::BSLightingShaderType::ShaderType_LODNoise:
                case Nif::BSLightingShaderType::ShaderType_MultitexLandLODBlend:
                case Nif::BSLightingShaderType::ShaderType_Dismemberment:
                case Nif::BSLightingShaderType::ShaderType_Terrain:
                    Log(Debug::Warning) << "Unhandled BSLightingShaderType " << type << " in " << mFilename;
                    return "bs/default";
            }
            Log(Debug::Warning) << "Unknown BSLightingShaderType " << type << " in " << mFilename;
            return "bs/default";
        }

        void handleProperty(const Nif::NiProperty* property, osg::Node* node,
            SceneUtil::CompositeStateSetUpdater* composite, std::vector<unsigned int>& boundTextures, int animflags,
            bool hasStencilProperty)
        {
            switch (property->recType)
            {
                case Nif::RC_NiStencilProperty:
                {
                    const Nif::NiStencilProperty* stencilprop = static_cast<const Nif::NiStencilProperty*>(property);

                    osg::ref_ptr<osg::FrontFace> frontFace = new osg::FrontFace;
                    using DrawMode = Nif::NiStencilProperty::DrawMode;
                    switch (stencilprop->mDrawMode)
                    {
                        case DrawMode::Clockwise:
                            frontFace->setMode(osg::FrontFace::CLOCKWISE);
                            break;
                        case DrawMode::Default:
                        case DrawMode::CounterClockwise:
                        case DrawMode::Both:
                        default:
                            frontFace->setMode(osg::FrontFace::COUNTER_CLOCKWISE);
                            break;
                    }
                    frontFace = shareAttribute(frontFace);

                    osg::StateSet* stateset = node->getOrCreateStateSet();
                    stateset->setAttribute(frontFace, osg::StateAttribute::ON);
                    if (stencilprop->mDrawMode == DrawMode::Both)
                        stateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
                    else
                        stateset->setMode(GL_CULL_FACE, osg::StateAttribute::ON);

                    if (stencilprop->mEnabled)
                    {
                        mHasStencilProperty = true;
                        osg::ref_ptr<osg::Stencil> stencil = new osg::Stencil;
                        stencil->setFunction(getStencilFunction(stencilprop->mTestFunction), stencilprop->mStencilRef,
                            stencilprop->mStencilMask);
                        stencil->setStencilFailOperation(getStencilOperation(stencilprop->mFailAction));
                        stencil->setStencilPassAndDepthFailOperation(getStencilOperation(stencilprop->mZFailAction));
                        stencil->setStencilPassAndDepthPassOperation(getStencilOperation(stencilprop->mPassAction));
                        stencil = shareAttribute(stencil);

                        stateset->setAttributeAndModes(stencil, osg::StateAttribute::ON);
                    }
                    break;
                }
                case Nif::RC_NiWireframeProperty:
                {
                    const Nif::NiWireframeProperty* wireprop = static_cast<const Nif::NiWireframeProperty*>(property);
                    osg::ref_ptr<osg::PolygonMode> mode = new osg::PolygonMode;
                    mode->setMode(osg::PolygonMode::FRONT_AND_BACK,
                        wireprop->mEnable ? osg::PolygonMode::LINE : osg::PolygonMode::FILL);
                    mode = shareAttribute(mode);
                    node->getOrCreateStateSet()->setAttributeAndModes(mode, osg::StateAttribute::ON);
                    break;
                }
                case Nif::RC_NiZBufferProperty:
                {
                    const Nif::NiZBufferProperty* zprop = static_cast<const Nif::NiZBufferProperty*>(property);
                    osg::StateSet* stateset = node->getOrCreateStateSet();
                    // The test function from this property seems to be ignored.
                    handleDepthFlags(stateset, zprop->depthTest(), zprop->depthWrite());
                    break;
                }
                // OSG groups the material properties that NIFs have separate, so we have to parse them all again when
                // one changed
                case Nif::RC_NiMaterialProperty:
                case Nif::RC_NiVertexColorProperty:
                case Nif::RC_NiSpecularProperty:
                {
                    // Handled on drawable level so we know whether vertex colors are available
                    break;
                }
                case Nif::RC_NiAlphaProperty:
                {
                    // Handled on drawable level to prevent RenderBin nesting issues
                    break;
                }
                case Nif::RC_NiTexturingProperty:
                {
                    const Nif::NiTexturingProperty* texprop = static_cast<const Nif::NiTexturingProperty*>(property);
                    osg::StateSet* stateset = node->getOrCreateStateSet();
                    handleTextureProperty(texprop, node->getName(), stateset, composite, boundTextures, animflags);
                    node->setUserValue("applyMode", static_cast<int>(texprop->mApplyMode));
                    break;
                }
                case Nif::RC_BSShaderPPLightingProperty:
                {
                    auto texprop = static_cast<const Nif::BSShaderPPLightingProperty*>(property);
                    bool shaderRequired = true;
                    node->setUserValue("shaderPrefix", std::string(getBSShaderPrefix(texprop->mType)));
                    node->setUserValue("shaderRequired", shaderRequired);
                    osg::StateSet* stateset = node->getOrCreateStateSet();
                    clearBoundTextures(stateset, boundTextures);
                    if (!texprop->mTextureSet.empty())
                        handleTextureSet(texprop->mTextureSet.getPtr(), texprop->wrapS(), texprop->wrapT(),
                            node->getName(), stateset, boundTextures);
                    handleTextureControllers(texprop, composite, stateset, animflags);
                    if (texprop->refraction())
                        SceneUtil::setupDistortion(*node, texprop->mRefraction.mStrength);
                    break;
                }
                case Nif::RC_BSShaderNoLightingProperty:
                {
                    auto texprop = static_cast<const Nif::BSShaderNoLightingProperty*>(property);
                    bool shaderRequired = true;
                    bool useFalloff = false;
                    node->setUserValue("shaderPrefix", std::string(getBSShaderPrefix(texprop->mType)));
                    node->setUserValue("shaderRequired", shaderRequired);
                    osg::StateSet* stateset = node->getOrCreateStateSet();
                    clearBoundTextures(stateset, boundTextures);
                    if (!texprop->mFilename.empty())
                    {
                        const unsigned int uvSet = 0;
                        attachExternalTexture("diffuseMap", texprop->mFilename, texprop->wrapS(), texprop->wrapT(),
                            uvSet, stateset, boundTextures);
                    }
                    if (mBethVersion >= 27)
                    {
                        useFalloff = true;
                        stateset->addUniform(new osg::Uniform("falloffParams", texprop->mFalloffParams));
                    }
                    stateset->addUniform(new osg::Uniform("useFalloff", useFalloff));
                    handleTextureControllers(texprop, composite, stateset, animflags);
                    handleDepthFlags(stateset, texprop->depthTest(), texprop->depthWrite());
                    break;
                }
                case Nif::RC_BSLightingShaderProperty:
                {
                    auto texprop = static_cast<const Nif::BSLightingShaderProperty*>(property);
                    bool shaderRequired = true;
                    node->setUserValue("shaderPrefix", std::string(getBSLightingShaderPrefix(texprop->mType)));
                    node->setUserValue("shaderRequired", shaderRequired);
                    osg::StateSet* stateset = node->getOrCreateStateSet();
                    clearBoundTextures(stateset, boundTextures);
                    if (Bgsm::MaterialFilePtr material = getShaderMaterial(texprop->mName, mMaterialManager))
                    {
                        handleShaderMaterialNodeProperties(material.get(), stateset, boundTextures);
                        break;
                    }
                    if (!texprop->mTextureSet.empty())
                        handleTextureSet(texprop->mTextureSet.getPtr(), texprop->wrapS(), texprop->wrapT(),
                            node->getName(), stateset, boundTextures);
                    handleTextureControllers(texprop, composite, stateset, animflags);
                    if (texprop->doubleSided())
                        stateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
                    if (texprop->treeAnim())
                        stateset->addUniform(new osg::Uniform("useTreeAnim", true));
                    handleDepthFlags(stateset, texprop->depthTest(), texprop->depthWrite());
                    if (texprop->refraction())
                        SceneUtil::setupDistortion(*node, texprop->mRefractionStrength);
                    break;
                }
                case Nif::RC_BSEffectShaderProperty:
                {
                    auto texprop = static_cast<const Nif::BSEffectShaderProperty*>(property);
                    bool shaderRequired = true;
                    // TODO: implement BSEffectShader as a shader
                    node->setUserValue("shaderPrefix", std::string("bs/nolighting"));
                    node->setUserValue("shaderRequired", shaderRequired);
                    osg::StateSet* stateset = node->getOrCreateStateSet();
                    clearBoundTextures(stateset, boundTextures);
                    if (Bgsm::MaterialFilePtr material = getShaderMaterial(texprop->mName, mMaterialManager))
                    {
                        handleShaderMaterialNodeProperties(material.get(), stateset, boundTextures);
                        break;
                    }
                    if (!texprop->mSourceTexture.empty())
                    {
                        const unsigned int uvSet = 0;
                        unsigned int texUnit = boundTextures.size();
                        attachExternalTexture("diffuseMap", texprop->mSourceTexture, texprop->wrapS(), texprop->wrapT(),
                            uvSet, stateset, boundTextures);
                        {
                            osg::ref_ptr<osg::TexMat> texMat(new osg::TexMat);
                            // This handles 20.2.0.7 UV settings like 4.0.0.2 UV settings (see NifOsg::UVController)
                            // TODO: verify
                            osg::Vec3f uvOrigin(0.5f, 0.5f, 0.f);
                            osg::Vec3f uvScale(texprop->mUVScale.x(), texprop->mUVScale.y(), 1.f);
                            osg::Vec3f uvTrans(-texprop->mUVOffset.x(), -texprop->mUVOffset.y(), 0.f);

                            osg::Matrixf mat = osg::Matrixf::translate(uvOrigin);
                            mat.preMultScale(uvScale);
                            mat.preMultTranslate(-uvOrigin);
                            mat.setTrans(mat.getTrans() + uvTrans);

                            texMat->setMatrix(mat);
                            stateset->setTextureAttributeAndModes(texUnit, texMat, osg::StateAttribute::ON);
                        }
                    }
                    bool useFalloff = texprop->useFalloff();
                    stateset->addUniform(new osg::Uniform("useFalloff", useFalloff));
                    if (useFalloff)
                        stateset->addUniform(new osg::Uniform("falloffParams", texprop->mFalloffParams));
                    handleTextureControllers(texprop, composite, stateset, animflags);
                    if (texprop->doubleSided())
                        stateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
                    handleDepthFlags(stateset, texprop->depthTest(), texprop->depthWrite());
                    break;
                }
                case Nif::RC_NiFogProperty:
                {
                    const Nif::NiFogProperty* fogprop = static_cast<const Nif::NiFogProperty*>(property);
                    osg::StateSet* stateset = node->getOrCreateStateSet();
                    // Vertex alpha mode appears to be broken
                    if (!fogprop->vertexAlpha() && fogprop->enabled())
                    {
                        osg::ref_ptr<NifOsg::Fog> fog = new NifOsg::Fog;
                        fog->setMode(osg::Fog::LINEAR);
                        fog->setColor(osg::Vec4f(fogprop->mColour, 1.f));
                        fog->setDepth(fogprop->mFogDepth);
                        fog = shareAttribute(fog);
                        stateset->setAttributeAndModes(fog, osg::StateAttribute::ON);
                        // Intentionally ignoring radial fog flag
                        // We don't really want to override the global setting
                    }
                    else
                    {
                        osg::ref_ptr<osg::Fog> fog = new osg::Fog;
                        // Shaders don't respect glDisable(GL_FOG)
                        fog->setMode(osg::Fog::LINEAR);
                        fog->setStart(10000000);
                        fog->setEnd(10000000);
                        fog = shareAttribute(fog);
                        stateset->setAttributeAndModes(fog, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
                    }
                    break;
                }
                // unused by mw
                case Nif::RC_NiShadeProperty:
                case Nif::RC_NiDitherProperty:
                {
                    break;
                }
                default:
                    Log(Debug::Info) << "Unhandled " << property->recName << " in " << mFilename;
                    break;
            }
        }

        struct CompareStateAttribute
        {
            bool operator()(
                const osg::ref_ptr<osg::StateAttribute>& left, const osg::ref_ptr<osg::StateAttribute>& right) const
            {
                return left->compare(*right) < 0;
            }
        };

        // global sharing of State Attributes will reduce the number of GL calls as the osg::State will check by pointer
        // to see if state is the same
        template <class Attribute>
        static Attribute* shareAttribute(const osg::ref_ptr<Attribute>& attr)
        {
            using Cache = std::set<osg::ref_ptr<Attribute>, CompareStateAttribute>;
            static Cache sCache;
            static std::mutex sMutex;
            std::lock_guard<std::mutex> lock(sMutex);
            typename Cache::iterator found = sCache.find(attr);
            if (found == sCache.end())
                found = sCache.insert(attr).first;
            return *found;
        }

        void applyDrawableProperties(osg::Node* node, const std::vector<const Nif::NiProperty*>& properties,
            SceneUtil::CompositeStateSetUpdater* composite, bool hasVertexColors, int animflags)
        {
            // Specular lighting is enabled by default, but there's a quirk...
            bool specEnabled = true;
            osg::ref_ptr<osg::Material> mat(new osg::Material);
            mat->setColorMode(hasVertexColors ? osg::Material::AMBIENT_AND_DIFFUSE : osg::Material::OFF);

            // NIF material defaults don't match OpenGL defaults
            mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(1, 1, 1, 1));
            mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1, 1, 1, 1));

            bool hasMatCtrl = false;
            bool hasSortAlpha = false;

            auto setBin_BackToFront = [](osg::StateSet* ss) { ss->setRenderBinDetails(0, "SORT_BACK_TO_FRONT"); };
            auto setBin_Traversal = [](osg::StateSet* ss) { ss->setRenderBinDetails(2, "TraversalOrderBin"); };

            auto lightmode = Nif::NiVertexColorProperty::LightMode::LightMode_EmiAmbDif;
            float emissiveMult = 1.f;
            float specStrength = 1.f;

            for (const Nif::NiProperty* property : properties)
            {
                switch (property->recType)
                {
                    case Nif::RC_NiSpecularProperty:
                    {
                        // Specular property can turn specular lighting off.
                        // FIXME: NiMaterialColorController doesn't care about this.
                        auto specprop = static_cast<const Nif::NiSpecularProperty*>(property);
                        specEnabled = specprop->mEnable;
                        break;
                    }
                    case Nif::RC_NiMaterialProperty:
                    {
                        const Nif::NiMaterialProperty* matprop = static_cast<const Nif::NiMaterialProperty*>(property);

                        mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(matprop->mDiffuse, matprop->mAlpha));
                        mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(matprop->mAmbient, 1.f));
                        mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(matprop->mEmissive, 1.f));
                        emissiveMult = matprop->mEmissiveMult;

                        mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(matprop->mSpecular, 1.f));
                        // NIFs may provide specular exponents way above OpenGL's limit.
                        // They can't be used properly, but we don't need OSG to constantly harass us about it.
                        float glossiness = std::clamp(matprop->mGlossiness, 0.f, 128.f);
                        mat->setShininess(osg::Material::FRONT_AND_BACK, glossiness);

                        if (!matprop->mController.empty())
                        {
                            hasMatCtrl = true;
                            handleMaterialControllers(matprop, composite, animflags, mat);
                        }

                        break;
                    }
                    case Nif::RC_NiVertexColorProperty:
                    {
                        const Nif::NiVertexColorProperty* vertprop
                            = static_cast<const Nif::NiVertexColorProperty*>(property);

                        using VertexMode = Nif::NiVertexColorProperty::VertexMode;
                        switch (vertprop->mVertexMode)
                        {
                            case VertexMode::VertMode_SrcIgnore:
                            {
                                mat->setColorMode(osg::Material::OFF);
                                break;
                            }
                            case VertexMode::VertMode_SrcEmissive:
                            {
                                mat->setColorMode(osg::Material::EMISSION);
                                break;
                            }
                            case VertexMode::VertMode_SrcAmbDif:
                            {
                                lightmode = vertprop->mLightingMode;
                                using LightMode = Nif::NiVertexColorProperty::LightMode;
                                switch (lightmode)
                                {
                                    case LightMode::LightMode_Emissive:
                                    {
                                        mat->setColorMode(osg::Material::OFF);
                                        break;
                                    }
                                    case LightMode::LightMode_EmiAmbDif:
                                    default:
                                    {
                                        mat->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
                                        break;
                                    }
                                }
                                break;
                            }
                        }

                        break;
                    }
                    case Nif::RC_NiAlphaProperty:
                    {
                        const Nif::NiAlphaProperty* alphaprop = static_cast<const Nif::NiAlphaProperty*>(property);
                        handleAlphaBlending(alphaprop->useAlphaBlending(), alphaprop->sourceBlendMode(),
                            alphaprop->destinationBlendMode(), !alphaprop->noSorter(), hasSortAlpha, *node);
                        handleAlphaTesting(alphaprop->useAlphaTesting(), getTestMode(alphaprop->alphaTestMode()),
                            alphaprop->mThreshold, *node);
                        break;
                    }
                    case Nif::RC_BSShaderPPLightingProperty:
                    {
                        auto shaderprop = static_cast<const Nif::BSShaderPPLightingProperty*>(property);
                        specEnabled = shaderprop->specular();
                        break;
                    }
                    case Nif::RC_BSLightingShaderProperty:
                    {
                        auto shaderprop = static_cast<const Nif::BSLightingShaderProperty*>(property);
                        if (Bgsm::MaterialFilePtr shaderMat = getShaderMaterial(shaderprop->mName, mMaterialManager))
                        {
                            handleShaderMaterialDrawableProperties(shaderMat.get(), mat, *node, hasSortAlpha);
                            if (shaderMat->mShaderType == Bgsm::ShaderType::Lighting)
                            {
                                auto bgsm = static_cast<const Bgsm::BGSMFile*>(shaderMat.get());
                                specEnabled = false; // bgsm->mSpecularEnabled; TODO: PBR specular lighting
                                specStrength = 1.f; // bgsm->mSpecularMult;
                                emissiveMult = bgsm->mEmittanceMult;
                            }
                            break;
                        }
                        mat->setAlpha(osg::Material::FRONT_AND_BACK, shaderprop->mAlpha);
                        mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(shaderprop->mEmissive, 1.f));
                        mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(shaderprop->mSpecular, 1.f));
                        float glossiness = std::clamp(shaderprop->mGlossiness, 0.f, 128.f);
                        mat->setShininess(osg::Material::FRONT_AND_BACK, glossiness);
                        emissiveMult = shaderprop->mEmissiveMult;
                        specStrength = shaderprop->mSpecStrength;
                        specEnabled = shaderprop->specular();
                        handleDecal(shaderprop->decal(), hasSortAlpha, *node);
                        break;
                    }
                    case Nif::RC_BSEffectShaderProperty:
                    {
                        auto shaderprop = static_cast<const Nif::BSEffectShaderProperty*>(property);
                        if (Bgsm::MaterialFilePtr shaderMat = getShaderMaterial(shaderprop->mName, mMaterialManager))
                        {
                            handleShaderMaterialDrawableProperties(shaderMat.get(), mat, *node, hasSortAlpha);
                            break;
                        }
                        handleDecal(shaderprop->decal(), hasSortAlpha, *node);
                        if (shaderprop->softEffect())
                            SceneUtil::setupSoftEffect(
                                *node, shaderprop->mFalloffDepth, true, shaderprop->mFalloffDepth);
                        break;
                    }
                    default:
                        break;
                }
            }

            // While NetImmerse and Gamebryo support specular lighting, Morrowind has its support disabled.
            if (mVersion <= Nif::NIFFile::VER_MW || !specEnabled)
            {
                mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f, 0.f, 0.f, 0.f));
                mat->setShininess(osg::Material::FRONT_AND_BACK, 0.f);
                specStrength = 1.f;
            }

            if (lightmode == Nif::NiVertexColorProperty::LightMode::LightMode_Emissive)
            {
                osg::Vec4f diffuse = mat->getDiffuse(osg::Material::FRONT_AND_BACK);
                diffuse = osg::Vec4f(0, 0, 0, diffuse.a());
                mat->setDiffuse(osg::Material::FRONT_AND_BACK, diffuse);
                mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f());
            }

            // If we're told to use vertex colors but there are none to use, use a default color instead.
            if (!hasVertexColors)
            {
                switch (mat->getColorMode())
                {
                    case osg::Material::AMBIENT:
                        mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1, 1, 1, 1));
                        break;
                    case osg::Material::AMBIENT_AND_DIFFUSE:
                        mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1, 1, 1, 1));
                        mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(1, 1, 1, 1));
                        break;
                    case osg::Material::EMISSION:
                        mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(1, 1, 1, 1));
                        break;
                    default:
                        break;
                }
                mat->setColorMode(osg::Material::OFF);
            }

            if (hasMatCtrl || mat->getColorMode() != osg::Material::OFF
                || mat->getEmission(osg::Material::FRONT_AND_BACK) != osg::Vec4f(0, 0, 0, 1)
                || mat->getDiffuse(osg::Material::FRONT_AND_BACK) != osg::Vec4f(1, 1, 1, 1)
                || mat->getAmbient(osg::Material::FRONT_AND_BACK) != osg::Vec4f(1, 1, 1, 1)
                || mat->getShininess(osg::Material::FRONT_AND_BACK) != 0
                || mat->getSpecular(osg::Material::FRONT_AND_BACK) != osg::Vec4f(0.f, 0.f, 0.f, 0.f))
            {
                mat = shareAttribute(mat);
                node->getOrCreateStateSet()->setAttributeAndModes(mat, osg::StateAttribute::ON);
            }

            if (emissiveMult != 1.f)
                node->getOrCreateStateSet()->addUniform(new osg::Uniform("emissiveMult", emissiveMult));

            if (specStrength != 1.f)
                node->getOrCreateStateSet()->addUniform(new osg::Uniform("specStrength", specStrength));

            if (!mPushedSorter)
            {
                if (!hasSortAlpha && mHasStencilProperty)
                    setBin_Traversal(node->getOrCreateStateSet());
                return;
            }

            osg::StateSet* stateset = node->getOrCreateStateSet();
            auto assignBin = [&](Nif::NiSortAdjustNode::SortingMode mode, int type) {
                if (mode == Nif::NiSortAdjustNode::SortingMode::Off)
                {
                    setBin_Traversal(stateset);
                    return;
                }

                if (type == Nif::RC_NiAlphaAccumulator)
                {
                    if (hasSortAlpha)
                        setBin_BackToFront(stateset);
                    else
                        setBin_Traversal(stateset);
                }
                else if (type == Nif::RC_NiClusterAccumulator)
                    setBin_BackToFront(stateset);
                else
                    Log(Debug::Error) << "Unrecognized NiAccumulator in " << mFilename;
            };

            switch (mPushedSorter->mMode)
            {
                case Nif::NiSortAdjustNode::SortingMode::Inherit:
                {
                    if (mLastAppliedNoInheritSorter)
                        assignBin(mLastAppliedNoInheritSorter->mMode, mLastAppliedNoInheritSorter->mSubSorter->recType);
                    else
                        assignBin(mPushedSorter->mMode, Nif::RC_NiAlphaAccumulator);
                    break;
                }
                case Nif::NiSortAdjustNode::SortingMode::Off:
                {
                    setBin_Traversal(stateset);
                    break;
                }
                case Nif::NiSortAdjustNode::SortingMode::Subsort:
                {
                    assignBin(mPushedSorter->mMode, mPushedSorter->mSubSorter->recType);
                    break;
                }
            }
        }
    };

    osg::ref_ptr<osg::Node> Loader::load(
        Nif::FileView file, Resource::ImageManager* imageManager, Resource::BgsmFileManager* materialManager)
    {
        LoaderImpl impl(file.getFilename(), file.getVersion(), file.getUserVersion(), file.getBethVersion());
        impl.mMaterialManager = materialManager;
        impl.mImageManager = imageManager;
        return impl.load(file);
    }

    void Loader::loadKf(Nif::FileView kf, SceneUtil::KeyframeHolder& target)
    {
        LoaderImpl impl(kf.getFilename(), kf.getVersion(), kf.getUserVersion(), kf.getBethVersion());
        impl.loadKf(kf, target);
    }

}
