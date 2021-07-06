#include "nifloader.hpp"

#include <mutex>

#include <osg/Matrixf>
#include <osg/Geometry>
#include <osg/Array>
#include <osg/LOD>
#include <osg/Switch>
#include <osg/TexGen>
#include <osg/ValueObject>

// resource
#include <components/debug/debuglog.hpp>
#include <components/misc/constants.hpp>
#include <components/misc/stringops.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/imagemanager.hpp>
#include <components/sceneutil/util.hpp>

// particle
#include <osgParticle/ParticleSystem>
#include <osgParticle/ParticleSystemUpdater>
#include <osgParticle/ConstantRateCounter>
#include <osgParticle/BoxPlacer>
#include <osgParticle/ModularProgram>

#include <osg/BlendFunc>
#include <osg/AlphaFunc>
#include <osg/Depth>
#include <osg/PolygonMode>
#include <osg/FrontFace>
#include <osg/Stencil>
#include <osg/Material>
#include <osg/Texture2D>
#include <osg/TexEnv>
#include <osg/TexEnvCombine>

#include <components/nif/node.hpp>
#include <components/nif/effect.hpp>
#include <components/sceneutil/skeleton.hpp>
#include <components/sceneutil/riggeometry.hpp>
#include <components/sceneutil/morphgeometry.hpp>

#include "matrixtransform.hpp"
#include "particle.hpp"

namespace
{

    void getAllNiNodes(const Nif::Node* node, std::vector<int>& outIndices)
    {
        const Nif::NiNode* ninode = dynamic_cast<const Nif::NiNode*>(node);
        if (ninode)
        {
            outIndices.push_back(ninode->recIndex);
            for (unsigned int i=0; i<ninode->children.length(); ++i)
                if (!ninode->children[i].empty())
                    getAllNiNodes(ninode->children[i].getPtr(), outIndices);
        }
    }

    bool isTypeGeometry(int type)
    {
        switch (type)
        {
            case Nif::RC_NiTriShape:
            case Nif::RC_NiTriStrips:
            case Nif::RC_NiLines:
            case Nif::RC_BSLODTriShape:
                return true;
        }
        return false;
    }

    // Collect all properties affecting the given drawable that should be handled on drawable basis rather than on the node hierarchy above it.
    void collectDrawableProperties(const Nif::Node* nifNode, std::vector<const Nif::Property*>& out)
    {
        if (nifNode->parent)
            collectDrawableProperties(nifNode->parent, out);
        const Nif::PropertyList& props = nifNode->props;
        for (size_t i = 0; i <props.length();++i)
        {
            if (!props[i].empty())
            {
                switch (props[i]->recType)
                {
                case Nif::RC_NiMaterialProperty:
                case Nif::RC_NiVertexColorProperty:
                case Nif::RC_NiSpecularProperty:
                case Nif::RC_NiAlphaProperty:
                    out.push_back(props[i].getPtr());
                    break;
                default:
                    break;
                }
            }
        }

        auto geometry = dynamic_cast<const Nif::NiGeometry*>(nifNode);
        if (geometry)
        {
            if (!geometry->shaderprop.empty())
                out.emplace_back(geometry->shaderprop.getPtr());
            if (!geometry->alphaprop.empty())
                out.emplace_back(geometry->alphaprop.getPtr());
        }
    }

    // NodeCallback used to have a node always oriented towards the camera. The node can have translation and scale
    // set just like a regular MatrixTransform, but the rotation set will be overridden in order to face the camera.
    // Must be set as a cull callback.
    class BillboardCallback : public osg::NodeCallback
    {
    public:
        BillboardCallback()
        {
        }
        BillboardCallback(const BillboardCallback& copy, const osg::CopyOp& copyop)
            : osg::NodeCallback(copy, copyop)
        {
        }

        META_Object(NifOsg, BillboardCallback)

        void operator()(osg::Node* node, osg::NodeVisitor* nv) override
        {
            osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);

            osg::Matrix modelView = *cv->getModelViewMatrix();

            // attempt to preserve scale
            float mag[3];
            for (int i=0;i<3;++i)
            {
                mag[i] = std::sqrt(modelView(0,i) * modelView(0,i) + modelView(1,i) * modelView(1,i) + modelView(2,i) * modelView(2,i));
            }

            modelView.setRotate(osg::Quat());
            modelView(0,0) = mag[0];
            modelView(1,1) = mag[1];
            modelView(2,2) = mag[2];

            cv->pushModelViewMatrix(new osg::RefMatrix(modelView), osg::Transform::RELATIVE_RF);

            traverse(node, nv);

            cv->popModelViewMatrix();
        }
    };

    void extractTextKeys(const Nif::NiTextKeyExtraData *tk, SceneUtil::TextKeyMap &textkeys)
    {
        for(size_t i = 0;i < tk->list.size();i++)
        {
            const std::string &str = tk->list[i].text;
            std::string::size_type pos = 0;
            while(pos < str.length())
            {
                if(::isspace(str[pos]))
                {
                    pos++;
                    continue;
                }

                std::string::size_type nextpos = std::min(str.find('\r', pos), str.find('\n', pos));
                if(nextpos != std::string::npos)
                {
                    do {
                        nextpos--;
                    } while(nextpos > pos && ::isspace(str[nextpos]));
                    nextpos++;
                }
                else if(::isspace(*str.rbegin()))
                {
                    std::string::const_iterator last = str.end();
                    do {
                        --last;
                    } while(last != str.begin() && ::isspace(*last));
                    nextpos = std::distance(str.begin(), ++last);
                }
                std::string result = str.substr(pos, nextpos-pos);
                Misc::StringUtils::lowerCaseInPlace(result);
                textkeys.emplace(tk->list[i].time, std::move(result));

                pos = nextpos;
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
        LoaderImpl(const std::string& filename, unsigned int ver, unsigned int userver, unsigned int bethver)
            : mFilename(filename), mVersion(ver), mUserVersion(userver), mBethVersion(bethver)
        {

        }
        std::string mFilename;
        unsigned int mVersion, mUserVersion, mBethVersion;

        size_t mFirstRootTextureIndex{~0u};
        bool mFoundFirstRootTexturingProperty = false;

        bool mHasNightDayLabel = false;
        bool mHasHerbalismLabel = false;

        // This is used to queue emitters that weren't attached to their node yet.
        std::vector<std::pair<size_t, osg::ref_ptr<Emitter>>> mEmitterQueue;

        static void loadKf(Nif::NIFFilePtr nif, SceneUtil::KeyframeHolder& target)
        {
            const Nif::NiSequenceStreamHelper *seq = nullptr;
            const size_t numRoots = nif->numRoots();
            for (size_t i = 0; i < numRoots; ++i)
            {
                const Nif::Record *r = nif->getRoot(i);
                if (r && r->recType == Nif::RC_NiSequenceStreamHelper)
                {
                    seq = static_cast<const Nif::NiSequenceStreamHelper*>(r);
                    break;
                }
            }

            if (!seq)
            {
                nif->warn("Found no NiSequenceStreamHelper root record");
                return;
            }

            Nif::ExtraPtr extra = seq->extra;
            if(extra.empty() || extra->recType != Nif::RC_NiTextKeyExtraData)
            {
                nif->warn("First extra data was not a NiTextKeyExtraData, but a "+
                          (extra.empty() ? std::string("nil") : extra->recName)+".");
                return;
            }

            extractTextKeys(static_cast<const Nif::NiTextKeyExtraData*>(extra.getPtr()), target.mTextKeys);

            extra = extra->next;
            Nif::ControllerPtr ctrl = seq->controller;
            for(;!extra.empty() && !ctrl.empty();(extra=extra->next),(ctrl=ctrl->next))
            {
                if(extra->recType != Nif::RC_NiStringExtraData || ctrl->recType != Nif::RC_NiKeyframeController)
                {
                    nif->warn("Unexpected extra data "+extra->recName+" with controller "+ctrl->recName);
                    continue;
                }

                // Vanilla seems to ignore the "active" flag for NiKeyframeController,
                // so we don't want to skip inactive controllers here.

                const Nif::NiStringExtraData *strdata = static_cast<const Nif::NiStringExtraData*>(extra.getPtr());
                const Nif::NiKeyframeController *key = static_cast<const Nif::NiKeyframeController*>(ctrl.getPtr());

                if (key->data.empty() && key->interpolator.empty())
                    continue;

                osg::ref_ptr<SceneUtil::KeyframeController> callback(handleKeyframeController(key));
                callback->setFunction(std::shared_ptr<NifOsg::ControllerFunction>(new NifOsg::ControllerFunction(key)));

                if (!target.mKeyframeControllers.emplace(strdata->string, callback).second)
                    Log(Debug::Verbose) << "Controller " << strdata->string << " present more than once in " << nif->getFilename() << ", ignoring later version";
            }
        }

        osg::ref_ptr<osg::Node> load(Nif::NIFFilePtr nif, Resource::ImageManager* imageManager)
        {
            const size_t numRoots = nif->numRoots();
            std::vector<const Nif::Node*> roots;
            for (size_t i = 0; i < numRoots; ++i)
            {
                const Nif::Record* r = nif->getRoot(i);
                if (!r)
                    continue;
                const Nif::Node* nifNode = dynamic_cast<const Nif::Node*>(r);
                if (nifNode)
                    roots.emplace_back(nifNode);
            }
            if (roots.empty())
                nif->fail("Found no root nodes");

            osg::ref_ptr<SceneUtil::TextKeyMapHolder> textkeys (new SceneUtil::TextKeyMapHolder);

            osg::ref_ptr<osg::Group> created(new osg::Group);
            created->setDataVariance(osg::Object::STATIC);
            for (const Nif::Node* root : roots)
            {
                auto node = handleNode(root, nullptr, imageManager, std::vector<unsigned int>(), 0, false, false, false, &textkeys->mTextKeys);
                created->addChild(node);
            }
            if (mHasNightDayLabel)
                created->getOrCreateUserDataContainer()->addDescription(Constants::NightDayLabel);
            if (mHasHerbalismLabel)
                created->getOrCreateUserDataContainer()->addDescription(Constants::HerbalismLabel);

            // Attach particle emitters to their nodes which should all be loaded by now.
            handleQueuedParticleEmitters(created, nif);

            if (nif->getUseSkinning())
            {
                osg::ref_ptr<SceneUtil::Skeleton> skel = new SceneUtil::Skeleton;
                skel->setStateSet(created->getStateSet());
                skel->setName(created->getName());
                for (unsigned int i=0; i < created->getNumChildren(); ++i)
                    skel->addChild(created->getChild(i));
                created->removeChildren(0, created->getNumChildren());
                created = skel;
            }

            if (!textkeys->mTextKeys.empty())
                created->getOrCreateUserDataContainer()->addUserObject(textkeys);

            return created;
        }

        void applyNodeProperties(const Nif::Node *nifNode, osg::Node *applyTo, SceneUtil::CompositeStateSetUpdater* composite, Resource::ImageManager* imageManager, std::vector<unsigned int>& boundTextures, int animflags)
        {
            const Nif::PropertyList& props = nifNode->props;
            for (size_t i = 0; i <props.length(); ++i)
            {
                if (!props[i].empty())
                {
                    // Get the lowest numbered recIndex of the NiTexturingProperty root node.
                    // This is what is overridden when a spell effect "particle texture" is used.
                    if (nifNode->parent == nullptr && !mFoundFirstRootTexturingProperty && props[i].getPtr()->recType == Nif::RC_NiTexturingProperty)
                    {
                        mFirstRootTextureIndex = props[i].getPtr()->recIndex;
                        mFoundFirstRootTexturingProperty = true;
                    }
                    else if (props[i].getPtr()->recType == Nif::RC_NiTexturingProperty)
                    {
                        if (props[i].getPtr()->recIndex == mFirstRootTextureIndex)
                            applyTo->setUserValue("overrideFx", 1);
                    }
                    handleProperty(props[i].getPtr(), applyTo, composite, imageManager, boundTextures, animflags);
                }
            }

            auto geometry = dynamic_cast<const Nif::NiGeometry*>(nifNode);
            // NiGeometry's NiAlphaProperty doesn't get handled here because it's a drawable property
            if (geometry && !geometry->shaderprop.empty())
                handleProperty(geometry->shaderprop.getPtr(), applyTo, composite, imageManager, boundTextures, animflags);
        }

        void setupController(const Nif::Controller* ctrl, SceneUtil::Controller* toSetup, int animflags)
        {
            bool autoPlay = animflags & Nif::NiNode::AnimFlag_AutoPlay;
            if (autoPlay)
                toSetup->setSource(std::shared_ptr<SceneUtil::ControllerSource>(new SceneUtil::FrameTimeSource));

            toSetup->setFunction(std::shared_ptr<ControllerFunction>(new ControllerFunction(ctrl)));
        }

        osg::ref_ptr<osg::LOD> handleLodNode(const Nif::NiLODNode* niLodNode)
        {
            osg::ref_ptr<osg::LOD> lod (new osg::LOD);
            lod->setName(niLodNode->name);
            lod->setCenterMode(osg::LOD::USER_DEFINED_CENTER);
            lod->setCenter(niLodNode->lodCenter);
            for (unsigned int i=0; i<niLodNode->lodLevels.size(); ++i)
            {
                const Nif::NiLODNode::LODRange& range = niLodNode->lodLevels[i];
                lod->setRange(i, range.minRange, range.maxRange);
            }
            lod->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
            return lod;
        }

        osg::ref_ptr<osg::Switch> handleSwitchNode(const Nif::NiSwitchNode* niSwitchNode)
        {
            osg::ref_ptr<osg::Switch> switchNode (new osg::Switch);
            switchNode->setName(niSwitchNode->name);
            switchNode->setNewChildDefaultValue(false);
            switchNode->setSingleChildOn(niSwitchNode->initialIndex);
            return switchNode;
        }

        osg::ref_ptr<osg::Image> handleSourceTexture(const Nif::NiSourceTexture* st, Resource::ImageManager* imageManager)
        {
            if (!st)
                return nullptr;

            osg::ref_ptr<osg::Image> image;
            if (!st->external && !st->data.empty())
            {
                image = handleInternalTexture(st->data.getPtr());
            }
            else
            {
                std::string filename = Misc::ResourceHelpers::correctTexturePath(st->filename, imageManager->getVFS());
                image = imageManager->getImage(filename);
            }
            return image;
        }

        void handleEffect(const Nif::Node* nifNode, osg::Node* node, Resource::ImageManager* imageManager)
        {
            if (nifNode->recType != Nif::RC_NiTextureEffect)
            {
                Log(Debug::Info) << "Unhandled effect " << nifNode->recName << " in " << mFilename;
                return;
            }

            const Nif::NiTextureEffect* textureEffect = static_cast<const Nif::NiTextureEffect*>(nifNode);
            if (textureEffect->textureType != Nif::NiTextureEffect::Environment_Map)
            {
                Log(Debug::Info) << "Unhandled NiTextureEffect type " << textureEffect->textureType << " in " << mFilename;
                return;
            }

            if (textureEffect->texture.empty())
            {
                Log(Debug::Info) << "NiTextureEffect missing source texture in " << mFilename;
                return;
            }

            osg::ref_ptr<osg::TexGen> texGen (new osg::TexGen);
            switch (textureEffect->coordGenType)
            {
            case Nif::NiTextureEffect::World_Parallel:
                texGen->setMode(osg::TexGen::OBJECT_LINEAR);
                break;
            case Nif::NiTextureEffect::World_Perspective:
                texGen->setMode(osg::TexGen::EYE_LINEAR);
                break;
            case Nif::NiTextureEffect::Sphere_Map:
                texGen->setMode(osg::TexGen::SPHERE_MAP);
                break;
            default:
                Log(Debug::Info) << "Unhandled NiTextureEffect coordGenType " << textureEffect->coordGenType << " in " << mFilename;
                return;
            }

            osg::ref_ptr<osg::Image> image (handleSourceTexture(textureEffect->texture.getPtr(), imageManager));
            osg::ref_ptr<osg::Texture2D> texture2d (new osg::Texture2D(image));
            if (image)
                texture2d->setTextureSize(image->s(), image->t());
            texture2d->setName("envMap");
            bool wrapT = textureEffect->clamp & 0x1;
            bool wrapS = (textureEffect->clamp >> 1) & 0x1;
            texture2d->setWrap(osg::Texture::WRAP_S, wrapS ? osg::Texture::REPEAT : osg::Texture::CLAMP_TO_EDGE);
            texture2d->setWrap(osg::Texture::WRAP_T, wrapT ? osg::Texture::REPEAT : osg::Texture::CLAMP_TO_EDGE);

            int texUnit = 3; // FIXME

            osg::StateSet* stateset = node->getOrCreateStateSet();
            stateset->setTextureAttributeAndModes(texUnit, texture2d, osg::StateAttribute::ON);
            stateset->setTextureAttributeAndModes(texUnit, texGen, osg::StateAttribute::ON);
            stateset->setTextureAttributeAndModes(texUnit, createEmissiveTexEnv(), osg::StateAttribute::ON);

            stateset->addUniform(new osg::Uniform("envMapColor", osg::Vec4f(1,1,1,1)));
        }

        // Get a default dataVariance for this node to be used as a hint by optimization (post)routines
        osg::ref_ptr<osg::Group> createNode(const Nif::Node* nifNode)
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
                // loaded from an external .kf file (original engine just throws "can't find node" errors if you try).
                if (!nifNode->parent && nifNode->controller.empty() && nifNode->trafo.isIdentity())
                    node = new osg::Group;

                dataVariance = nifNode->isBone ? osg::Object::DYNAMIC : osg::Object::STATIC;

                break;
            }
            if (!node)
                node = new NifOsg::MatrixTransform(nifNode->trafo);

            if (nifNode->recType == Nif::RC_NiCollisionSwitch && !(nifNode->flags & Nif::NiNode::Flag_ActiveCollision))
            {
                node->setNodeMask(Loader::getIntersectionDisabledNodeMask());
                // This node must not be combined with another node.
                dataVariance = osg::Object::DYNAMIC;
            }

            node->setDataVariance(dataVariance);

            return node;
        }

        osg::ref_ptr<osg::Node> handleNode(const Nif::Node* nifNode, osg::Group* parentNode, Resource::ImageManager* imageManager,
                                std::vector<unsigned int> boundTextures, int animflags, bool skipMeshes, bool hasMarkers, bool hasAnimatedParents, SceneUtil::TextKeyMap* textKeys, osg::Node* rootNode=nullptr)
        {
            if (rootNode != nullptr && Misc::StringUtils::ciEqual(nifNode->name, "Bounding Box"))
                return nullptr;

            osg::ref_ptr<osg::Group> node = createNode(nifNode);

            if (nifNode->recType == Nif::RC_NiBillboardNode)
            {
                node->addCullCallback(new BillboardCallback);
            }

            node->setName(nifNode->name);

            if (parentNode)
                parentNode->addChild(node);

            if (!rootNode)
                rootNode = node;

            // The original NIF record index is used for a variety of features:
            // - finding the correct emitter node for a particle system
            // - establishing connections to the animated collision shapes, which are handled in a separate loader
            // - finding a random child NiNode in NiBspArrayController
            node->setUserValue("recIndex", nifNode->recIndex);

            std::vector<Nif::ExtraPtr> extraCollection;

            for (Nif::ExtraPtr e = nifNode->extra; !e.empty(); e = e->next)
                extraCollection.emplace_back(e);

            for (size_t i = 0; i < nifNode->extralist.length(); ++i)
            {
                Nif::ExtraPtr e = nifNode->extralist[i];
                if (!e.empty())
                    extraCollection.emplace_back(e);
            }

            for (const auto& e : extraCollection)
            {
                if(e->recType == Nif::RC_NiTextKeyExtraData && textKeys)
                {
                    const Nif::NiTextKeyExtraData *tk = static_cast<const Nif::NiTextKeyExtraData*>(e.getPtr());
                    extractTextKeys(tk, *textKeys);
                }
                else if(e->recType == Nif::RC_NiStringExtraData)
                {
                    const Nif::NiStringExtraData *sd = static_cast<const Nif::NiStringExtraData*>(e.getPtr());
                    // String markers may contain important information
                    // affecting the entire subtree of this obj
                    if(sd->string == "MRK" && !Loader::getShowMarkers())
                    {
                        // Marker objects. These meshes are only visible in the editor.
                        hasMarkers = true;
                    }
                    else if(sd->string == "BONE")
                    {
                        node->getOrCreateUserDataContainer()->addDescription("CustomBone");
                    }
                }
            }

            if (nifNode->recType == Nif::RC_NiBSAnimationNode || nifNode->recType == Nif::RC_NiBSParticleNode)
                animflags = nifNode->flags;

            // Hide collision shapes, but don't skip the subgraph
            // We still need to animate the hidden bones so the physics system can access them
            if (nifNode->recType == Nif::RC_RootCollisionNode)
            {
                skipMeshes = true;
                node->setNodeMask(Loader::getHiddenNodeMask());
            }

            // We can skip creating meshes for hidden nodes if they don't have a VisController that
            // might make them visible later
            if (nifNode->flags & Nif::NiNode::Flag_Hidden)
            {
                bool hasVisController = false;
                for (Nif::ControllerPtr ctrl = nifNode->controller; !ctrl.empty(); ctrl = ctrl->next)
                {
                    hasVisController |= (ctrl->recType == Nif::RC_NiVisController);
                    if (hasVisController)
                        break;
                }

                if (!hasVisController)
                    skipMeshes = true; // skip child meshes, but still create the child node hierarchy for animating collision shapes

                node->setNodeMask(Loader::getHiddenNodeMask());
            }

            osg::ref_ptr<SceneUtil::CompositeStateSetUpdater> composite = new SceneUtil::CompositeStateSetUpdater;

            applyNodeProperties(nifNode, node, composite, imageManager, boundTextures, animflags);

            const bool isGeometry = isTypeGeometry(nifNode->recType);

            if (isGeometry && !skipMeshes)
            {
                const std::string nodeName = Misc::StringUtils::lowerCase(nifNode->name);
                static const std::string markerName = "tri editormarker";
                static const std::string shadowName = "shadow";
                static const std::string shadowName2 = "tri shadow";
                const bool isMarker = hasMarkers && !nodeName.compare(0, markerName.size(), markerName);
                if (!isMarker && nodeName.compare(0, shadowName.size(), shadowName) && nodeName.compare(0, shadowName2.size(), shadowName2))
                {
                    Nif::NiSkinInstancePtr skin = static_cast<const Nif::NiGeometry*>(nifNode)->skin;

                    if (skin.empty())
                        handleGeometry(nifNode, node, composite, boundTextures, animflags);
                    else
                        handleSkinnedGeometry(nifNode, node, composite, boundTextures, animflags);

                    if (!nifNode->controller.empty())
                        handleMeshControllers(nifNode, node, composite, boundTextures, animflags);
                }
            }

            if (nifNode->recType == Nif::RC_NiParticles)
                handleParticleSystem(nifNode, node, composite, animflags);

            if (composite->getNumControllers() > 0)
            {
                osg::Callback *cb = composite;
                if (composite->getNumControllers() == 1)
                    cb = composite->getController(0);
                if (animflags & Nif::NiNode::AnimFlag_AutoPlay)
                    node->addCullCallback(cb);
                else
                    node->addUpdateCallback(cb); // have to remain as UpdateCallback so AssignControllerSourcesVisitor can find it.
            }

            bool isAnimated = false;
            handleNodeControllers(nifNode, node, animflags, isAnimated);
            hasAnimatedParents |= isAnimated;
            // Make sure empty nodes and animated shapes are not optimized away so the physics system can find them.
            if (isAnimated || (hasAnimatedParents && ((skipMeshes || hasMarkers) || isGeometry)))
                node->setDataVariance(osg::Object::DYNAMIC);

            // LOD and Switch nodes must be wrapped by a transform (the current node) to support transformations properly
            // and we need to attach their children to the osg::LOD/osg::Switch nodes
            // but we must return that transform to the caller of handleNode instead of the actual LOD/Switch nodes.
            osg::ref_ptr<osg::Group> currentNode = node;

            if (nifNode->recType == Nif::RC_NiSwitchNode)
            {
                const Nif::NiSwitchNode* niSwitchNode = static_cast<const Nif::NiSwitchNode*>(nifNode);
                osg::ref_ptr<osg::Switch> switchNode = handleSwitchNode(niSwitchNode);
                node->addChild(switchNode);
                if (niSwitchNode->name == Constants::NightDayLabel)
                    mHasNightDayLabel = true;
                else if (niSwitchNode->name == Constants::HerbalismLabel)
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

            const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(nifNode);
            if(ninode)
            {
                const Nif::NodeList &effects = ninode->effects;
                for (size_t i = 0; i < effects.length(); ++i)
                {
                    if (!effects[i].empty())
                        handleEffect(effects[i].getPtr(), currentNode, imageManager);
                }

                const Nif::NodeList &children = ninode->children;
                for(size_t i = 0;i < children.length();++i)
                {
                    if(!children[i].empty())
                        handleNode(children[i].getPtr(), currentNode, imageManager, boundTextures, animflags, skipMeshes, hasMarkers, hasAnimatedParents, textKeys, rootNode);
                }
            }

            return node;
        }

        void handleMeshControllers(const Nif::Node *nifNode, osg::Node* node, SceneUtil::CompositeStateSetUpdater* composite, const std::vector<unsigned int> &boundTextures, int animflags)
        {
            for (Nif::ControllerPtr ctrl = nifNode->controller; !ctrl.empty(); ctrl = ctrl->next)
            {
                if (!(ctrl->flags & Nif::NiNode::ControllerFlag_Active))
                    continue;
                if (ctrl->recType == Nif::RC_NiUVController)
                {
                    const Nif::NiUVController *niuvctrl = static_cast<const Nif::NiUVController*>(ctrl.getPtr());
                    if (niuvctrl->data.empty())
                        continue;
                    const unsigned int uvSet = niuvctrl->uvSet;
                    std::set<int> texUnits;
                    // UVController should work only for textures which use a given UV Set, usually 0.
                    for (unsigned int i=0; i<boundTextures.size(); ++i)
                    {
                        if (boundTextures[i] == uvSet)
                            texUnits.insert(i);
                    }

                    osg::ref_ptr<UVController> uvctrl = new UVController(niuvctrl->data.getPtr(), texUnits);
                    setupController(niuvctrl, uvctrl, animflags);
                    composite->addController(uvctrl);
                }
            }
        }

        static osg::ref_ptr<KeyframeController> handleKeyframeController(const Nif::NiKeyframeController* keyctrl)
        {
            osg::ref_ptr<NifOsg::KeyframeController> ctrl;
            if (!keyctrl->interpolator.empty())
            {
                const Nif::NiTransformInterpolator* interp = keyctrl->interpolator.getPtr();
                if (!interp->data.empty())
                    ctrl = new NifOsg::KeyframeController(interp);
                else
                    ctrl = new NifOsg::KeyframeController(interp->defaultScale, interp->defaultPos, interp->defaultRot);
            }
            else if (!keyctrl->data.empty())
            {
                ctrl = new NifOsg::KeyframeController(keyctrl->data.getPtr());
            }
            return ctrl;
        }

        void handleNodeControllers(const Nif::Node* nifNode, osg::Node* node, int animflags, bool& isAnimated)
        {
            for (Nif::ControllerPtr ctrl = nifNode->controller; !ctrl.empty(); ctrl = ctrl->next)
            {
                if (!(ctrl->flags & Nif::NiNode::ControllerFlag_Active))
                    continue;
                if (ctrl->recType == Nif::RC_NiKeyframeController)
                {
                    const Nif::NiKeyframeController *key = static_cast<const Nif::NiKeyframeController*>(ctrl.getPtr());
                    if (key->data.empty() && key->interpolator.empty())
                        continue;
                    osg::ref_ptr<KeyframeController> callback(handleKeyframeController(key));
                    setupController(key, callback, animflags);
                    node->addUpdateCallback(callback);
                    isAnimated = true;
                }
                else if (ctrl->recType == Nif::RC_NiPathController)
                {
                    const Nif::NiPathController *path = static_cast<const Nif::NiPathController*>(ctrl.getPtr());
                    if (path->posData.empty() || path->floatData.empty())
                        continue;
                    osg::ref_ptr<PathController> callback(new PathController(path));
                    setupController(path, callback, animflags);
                    node->addUpdateCallback(callback);
                    isAnimated = true;
                }
                else if (ctrl->recType == Nif::RC_NiVisController)
                {
                    const Nif::NiVisController *visctrl = static_cast<const Nif::NiVisController*>(ctrl.getPtr());
                    if (visctrl->data.empty())
                        continue;
                    osg::ref_ptr<VisController> callback(new VisController(visctrl->data.getPtr(), Loader::getHiddenNodeMask()));
                    setupController(visctrl, callback, animflags);
                    node->addUpdateCallback(callback);
                }
                else if (ctrl->recType == Nif::RC_NiRollController)
                {
                    const Nif::NiRollController *rollctrl = static_cast<const Nif::NiRollController*>(ctrl.getPtr());
                    if (rollctrl->data.empty() && rollctrl->interpolator.empty())
                        continue;
                    osg::ref_ptr<RollController> callback;
                    if (!rollctrl->interpolator.empty())
                        callback = new RollController(rollctrl->interpolator.getPtr());
                    else // if (!rollctrl->data.empty())
                        callback = new RollController(rollctrl->data.getPtr());
                    setupController(rollctrl, callback, animflags);
                    node->addUpdateCallback(callback);
                    isAnimated = true;
                }
                else if (ctrl->recType == Nif::RC_NiGeomMorpherController
                      || ctrl->recType == Nif::RC_NiParticleSystemController
                      || ctrl->recType == Nif::RC_NiBSPArrayController
                      || ctrl->recType == Nif::RC_NiUVController)
                {
                    // These controllers are handled elsewhere
                }
                else
                    Log(Debug::Info) << "Unhandled controller " << ctrl->recName << " on node " << nifNode->recIndex << " in " << mFilename;
            }
        }

        void handleMaterialControllers(const Nif::Property *materialProperty, SceneUtil::CompositeStateSetUpdater* composite, int animflags, const osg::Material* baseMaterial)
        {
            for (Nif::ControllerPtr ctrl = materialProperty->controller; !ctrl.empty(); ctrl = ctrl->next)
            {
                if (!(ctrl->flags & Nif::NiNode::ControllerFlag_Active))
                    continue;
                if (ctrl->recType == Nif::RC_NiAlphaController)
                {
                    const Nif::NiAlphaController* alphactrl = static_cast<const Nif::NiAlphaController*>(ctrl.getPtr());
                    if (alphactrl->data.empty() && alphactrl->interpolator.empty())
                        continue;
                    osg::ref_ptr<AlphaController> osgctrl;
                    if (!alphactrl->interpolator.empty())
                        osgctrl = new AlphaController(alphactrl->interpolator.getPtr(), baseMaterial);
                    else // if (!alphactrl->data.empty())
                        osgctrl = new AlphaController(alphactrl->data.getPtr(), baseMaterial);
                    setupController(alphactrl, osgctrl, animflags);
                    composite->addController(osgctrl);
                }
                else if (ctrl->recType == Nif::RC_NiMaterialColorController)
                {
                    const Nif::NiMaterialColorController* matctrl = static_cast<const Nif::NiMaterialColorController*>(ctrl.getPtr());
                    if (matctrl->data.empty() && matctrl->interpolator.empty())
                        continue;
                    osg::ref_ptr<MaterialColorController> osgctrl;
                    auto targetColor = static_cast<MaterialColorController::TargetColor>(matctrl->targetColor);
                    if (!matctrl->interpolator.empty())
                        osgctrl = new MaterialColorController(matctrl->interpolator.getPtr(), targetColor, baseMaterial);
                    else // if (!matctrl->data.empty())
                        osgctrl = new MaterialColorController(matctrl->data.getPtr(), targetColor, baseMaterial);
                    setupController(matctrl, osgctrl, animflags);
                    composite->addController(osgctrl);
                }
                else
                    Log(Debug::Info) << "Unexpected material controller " << ctrl->recType << " in " << mFilename;
            }
        }

        void handleTextureControllers(const Nif::Property *texProperty, SceneUtil::CompositeStateSetUpdater* composite, Resource::ImageManager* imageManager, osg::StateSet *stateset, int animflags)
        {
            for (Nif::ControllerPtr ctrl = texProperty->controller; !ctrl.empty(); ctrl = ctrl->next)
            {
                if (!(ctrl->flags & Nif::NiNode::ControllerFlag_Active))
                    continue;
                if (ctrl->recType == Nif::RC_NiFlipController)
                {
                    const Nif::NiFlipController* flipctrl = static_cast<const Nif::NiFlipController*>(ctrl.getPtr());
                    std::vector<osg::ref_ptr<osg::Texture2D> > textures;

                    // inherit wrap settings from the target slot
                    osg::Texture2D* inherit = dynamic_cast<osg::Texture2D*>(stateset->getTextureAttribute(0, osg::StateAttribute::TEXTURE));
                    osg::Texture2D::WrapMode wrapS = osg::Texture2D::REPEAT;
                    osg::Texture2D::WrapMode wrapT = osg::Texture2D::REPEAT;
                    if (inherit)
                    {
                        wrapS = inherit->getWrap(osg::Texture2D::WRAP_S);
                        wrapT = inherit->getWrap(osg::Texture2D::WRAP_T);
                    }

                    for (unsigned int i=0; i<flipctrl->mSources.length(); ++i)
                    {
                        Nif::NiSourceTexturePtr st = flipctrl->mSources[i];
                        if (st.empty())
                            continue;

                        osg::ref_ptr<osg::Image> image (handleSourceTexture(st.getPtr(), imageManager));
                        osg::ref_ptr<osg::Texture2D> texture (new osg::Texture2D(image));
                        if (image)
                            texture->setTextureSize(image->s(), image->t());
                        texture->setWrap(osg::Texture::WRAP_S, wrapS);
                        texture->setWrap(osg::Texture::WRAP_T, wrapT);
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

        void handleParticlePrograms(Nif::NiParticleModifierPtr affectors, Nif::NiParticleModifierPtr colliders, osg::Group *attachTo, osgParticle::ParticleSystem* partsys, osgParticle::ParticleProcessor::ReferenceFrame rf)
        {
            osgParticle::ModularProgram* program = new osgParticle::ModularProgram;
            attachTo->addChild(program);
            program->setParticleSystem(partsys);
            program->setReferenceFrame(rf);
            for (; !affectors.empty(); affectors = affectors->next)
            {
                if (affectors->recType == Nif::RC_NiParticleGrowFade)
                {
                    const Nif::NiParticleGrowFade *gf = static_cast<const Nif::NiParticleGrowFade*>(affectors.getPtr());
                    program->addOperator(new GrowFadeAffector(gf->growTime, gf->fadeTime));
                }
                else if (affectors->recType == Nif::RC_NiGravity)
                {
                    const Nif::NiGravity* gr = static_cast<const Nif::NiGravity*>(affectors.getPtr());
                    program->addOperator(new GravityAffector(gr));
                }
                else if (affectors->recType == Nif::RC_NiParticleColorModifier)
                {
                    const Nif::NiParticleColorModifier *cl = static_cast<const Nif::NiParticleColorModifier*>(affectors.getPtr());
                    if (cl->data.empty())
                        continue;
                    const Nif::NiColorData *clrdata = cl->data.getPtr();
                    program->addOperator(new ParticleColorAffector(clrdata));
                }
                else if (affectors->recType == Nif::RC_NiParticleRotation)
                {
                    // unused
                }
                else
                    Log(Debug::Info) << "Unhandled particle modifier " << affectors->recName << " in " << mFilename;
            }
            for (; !colliders.empty(); colliders = colliders->next)
            {
                if (colliders->recType == Nif::RC_NiPlanarCollider)
                {
                    const Nif::NiPlanarCollider* planarcollider = static_cast<const Nif::NiPlanarCollider*>(colliders.getPtr());
                    program->addOperator(new PlanarCollider(planarcollider));
                }
                else if (colliders->recType == Nif::RC_NiSphericalCollider)
                {
                    const Nif::NiSphericalCollider* sphericalcollider = static_cast<const Nif::NiSphericalCollider*>(colliders.getPtr());
                    program->addOperator(new SphericalCollider(sphericalcollider));
                }
                else
                    Log(Debug::Info) << "Unhandled particle collider " << colliders->recName << " in " << mFilename;
            }
        }

        // Load the initial state of the particle system, i.e. the initial particles and their positions, velocity and colors.
        void handleParticleInitialState(const Nif::Node* nifNode, ParticleSystem* partsys, const Nif::NiParticleSystemController* partctrl)
        {
            auto particleNode = static_cast<const Nif::NiParticles*>(nifNode);
            if (particleNode->data.empty() || particleNode->data->recType != Nif::RC_NiParticlesData)
            {
                partsys->setQuota(partctrl->numParticles);
                return;
            }

            auto particledata = static_cast<const Nif::NiParticlesData*>(particleNode->data.getPtr());
            partsys->setQuota(particledata->numParticles);

            osg::BoundingBox box;

            int i=0;
            for (const auto& particle : partctrl->particles)
            {
                if (i++ >= particledata->activeCount)
                    break;

                if (particle.lifespan <= 0)
                    continue;

                if (particle.vertex >= particledata->vertices.size())
                    continue;

                ParticleAgeSetter particletemplate(std::max(0.f, particle.lifetime));

                osgParticle::Particle* created = partsys->createParticle(&particletemplate);
                created->setLifeTime(particle.lifespan);

                // Note this position and velocity is not correct for a particle system with absolute reference frame,
                // which can not be done in this loader since we are not attached to the scene yet. Will be fixed up post-load in the SceneManager.
                created->setVelocity(particle.velocity);
                const osg::Vec3f& position = particledata->vertices[particle.vertex];
                created->setPosition(position);

                osg::Vec4f partcolor (1.f,1.f,1.f,1.f);
                if (particle.vertex < particledata->colors.size())
                    partcolor = particledata->colors[particle.vertex];

                float size = partctrl->size;
                if (particle.vertex < particledata->sizes.size())
                    size *= particledata->sizes[particle.vertex];

                created->setSizeRange(osgParticle::rangef(size, size));
                box.expandBy(osg::BoundingSphere(position, size));
            }

            // radius may be used to force a larger bounding box
            box.expandBy(osg::BoundingSphere(osg::Vec3(0,0,0), particledata->radius));

            partsys->setInitialBound(box);
        }

        osg::ref_ptr<Emitter> handleParticleEmitter(const Nif::NiParticleSystemController* partctrl)
        {
            std::vector<int> targets;
            if (partctrl->recType == Nif::RC_NiBSPArrayController)
            {
                getAllNiNodes(partctrl->emitter.getPtr(), targets);
            }

            osg::ref_ptr<Emitter> emitter = new Emitter(targets);

            osgParticle::ConstantRateCounter* counter = new osgParticle::ConstantRateCounter;
            if (partctrl->emitFlags & Nif::NiParticleSystemController::NoAutoAdjust)
                counter->setNumberOfParticlesPerSecondToCreate(partctrl->emitRate);
            else if (partctrl->lifetime == 0 && partctrl->lifetimeRandom == 0)
                counter->setNumberOfParticlesPerSecondToCreate(0);
            else
                counter->setNumberOfParticlesPerSecondToCreate(partctrl->numParticles / (partctrl->lifetime + partctrl->lifetimeRandom/2));

            emitter->setCounter(counter);

            ParticleShooter* shooter = new ParticleShooter(partctrl->velocity - partctrl->velocityRandom*0.5f,
                                                           partctrl->velocity + partctrl->velocityRandom*0.5f,
                                                           partctrl->horizontalDir, partctrl->horizontalAngle,
                                                           partctrl->verticalDir, partctrl->verticalAngle,
                                                           partctrl->lifetime, partctrl->lifetimeRandom);
            emitter->setShooter(shooter);

            osgParticle::BoxPlacer* placer = new osgParticle::BoxPlacer;
            placer->setXRange(-partctrl->offsetRandom.x() / 2.f, partctrl->offsetRandom.x() / 2.f);
            placer->setYRange(-partctrl->offsetRandom.y() / 2.f, partctrl->offsetRandom.y() / 2.f);
            placer->setZRange(-partctrl->offsetRandom.z() / 2.f, partctrl->offsetRandom.z() / 2.f);

            emitter->setPlacer(placer);
            return emitter;
        }

        void handleQueuedParticleEmitters(osg::Group* rootNode, Nif::NIFFilePtr nif)
        {
            for (const auto& emitterPair : mEmitterQueue)
            {
                size_t recIndex = emitterPair.first;
                FindGroupByRecIndex findEmitterNode(recIndex);
                rootNode->accept(findEmitterNode);
                osg::Group* emitterNode = findEmitterNode.mFound;
                if (!emitterNode)
                {
                    nif->warn("Failed to find particle emitter emitter node (node record index " + std::to_string(recIndex) + ")");
                    continue;
                }

                // Emitter attached to the emitter node. Note one side effect of the emitter using the CullVisitor is that hiding its node
                // actually causes the emitter to stop firing. Convenient, because MW behaves this way too!
                emitterNode->addChild(emitterPair.second);
            }
            mEmitterQueue.clear();
        }

        void handleParticleSystem(const Nif::Node *nifNode, osg::Group *parentNode, SceneUtil::CompositeStateSetUpdater* composite, int animflags)
        {
            osg::ref_ptr<ParticleSystem> partsys (new ParticleSystem);
            partsys->setSortMode(osgParticle::ParticleSystem::SORT_BACK_TO_FRONT);

            const Nif::NiParticleSystemController* partctrl = nullptr;
            for (Nif::ControllerPtr ctrl = nifNode->controller; !ctrl.empty(); ctrl = ctrl->next)
            {
                if (!(ctrl->flags & Nif::NiNode::ControllerFlag_Active))
                    continue;
                if(ctrl->recType == Nif::RC_NiParticleSystemController || ctrl->recType == Nif::RC_NiBSPArrayController)
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

            partsys->getDefaultParticleTemplate().setSizeRange(osgParticle::rangef(partctrl->size, partctrl->size));
            partsys->getDefaultParticleTemplate().setColorRange(osgParticle::rangev4(osg::Vec4f(1.f,1.f,1.f,1.f), osg::Vec4f(1.f,1.f,1.f,1.f)));
            partsys->getDefaultParticleTemplate().setAlphaRange(osgParticle::rangef(1.f, 1.f));

            partsys->setFreezeOnCull(true);

            if (!partctrl->emitter.empty())
            {
                osg::ref_ptr<Emitter> emitter = handleParticleEmitter(partctrl);
                emitter->setParticleSystem(partsys);
                emitter->setReferenceFrame(osgParticle::ParticleProcessor::RELATIVE_RF);

                // The emitter node may not actually be handled yet, so let's delay attaching the emitter to a later moment.
                // If the emitter node is placed later than the particle node, it'll have a single frame delay in particle processing.
                // But that shouldn't be a game-breaking issue.
                mEmitterQueue.emplace_back(partctrl->emitter->recIndex, emitter);

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

            // affectors should be attached *after* the emitter in the scene graph for correct update order
            // attach to same node as the ParticleSystem, we need osgParticle Operators to get the correct
            // localToWorldMatrix for transforming to particle space
            handleParticlePrograms(partctrl->affectors, partctrl->colliders, parentNode, partsys.get(), rf);

            std::vector<const Nif::Property*> drawableProps;
            collectDrawableProperties(nifNode, drawableProps);
            applyDrawableProperties(parentNode, drawableProps, composite, true, animflags);

            // particle system updater (after the emitters and affectors in the scene graph)
            // I think for correct culling needs to be *before* the ParticleSystem, though osg examples do it the other way
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
            // create partsys stateset in order to pass in ShaderVisitor like all other Drawables
            partsys->getOrCreateStateSet();
        }

        void handleNiGeometryData(osg::Geometry *geometry, const Nif::NiGeometryData* data, const std::vector<unsigned int>& boundTextures, const std::string& name)
        {
            const auto& vertices = data->vertices;
            const auto& normals = data->normals;
            const auto& colors = data->colors;
            if (!vertices.empty())
                geometry->setVertexArray(new osg::Vec3Array(vertices.size(), vertices.data()));
            if (!normals.empty())
                geometry->setNormalArray(new osg::Vec3Array(normals.size(), normals.data()), osg::Array::BIND_PER_VERTEX);
            if (!colors.empty())
                geometry->setColorArray(new osg::Vec4Array(colors.size(), colors.data()), osg::Array::BIND_PER_VERTEX);

            const auto& uvlist = data->uvlist;
            int textureStage = 0;
            for (const unsigned int uvSet : boundTextures)
            {
                if (uvSet >= uvlist.size())
                {
                    Log(Debug::Verbose) << "Out of bounds UV set " << uvSet << " on shape \"" << name << "\" in " << mFilename;
                    if (!uvlist.empty())
                        geometry->setTexCoordArray(textureStage, new osg::Vec2Array(uvlist[0].size(), uvlist[0].data()), osg::Array::BIND_PER_VERTEX);
                    continue;
                }

                geometry->setTexCoordArray(textureStage, new osg::Vec2Array(uvlist[uvSet].size(), uvlist[uvSet].data()), osg::Array::BIND_PER_VERTEX);
                textureStage++;
            }
        }

        void handleNiGeometry(const Nif::Node *nifNode, osg::Geometry *geometry, osg::Node* parentNode, SceneUtil::CompositeStateSetUpdater* composite, const std::vector<unsigned int>& boundTextures, int animflags)
        {
            const Nif::NiGeometry* niGeometry = static_cast<const Nif::NiGeometry*>(nifNode);
            if (niGeometry->data.empty())
                return;
            const Nif::NiGeometryData* niGeometryData = niGeometry->data.getPtr();

            if (niGeometry->recType == Nif::RC_NiTriShape || nifNode->recType == Nif::RC_BSLODTriShape)
            {
                if (niGeometryData->recType != Nif::RC_NiTriShapeData)
                    return;
                auto triangles = static_cast<const Nif::NiTriShapeData*>(niGeometryData)->triangles;
                if (triangles.empty())
                    return;
                geometry->addPrimitiveSet(new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLES, triangles.size(),
                                                                        (unsigned short*)triangles.data()));
            }
            else if (niGeometry->recType == Nif::RC_NiTriStrips)
            {
                if (niGeometryData->recType != Nif::RC_NiTriStripsData)
                    return;
                auto data = static_cast<const Nif::NiTriStripsData*>(niGeometryData);
                bool hasGeometry = false;
                for (const auto& strip : data->strips)
                {
                    if (strip.size() < 3)
                        continue;
                    geometry->addPrimitiveSet(new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLE_STRIP, strip.size(),
                                                                            (unsigned short*)strip.data()));
                    hasGeometry = true;
                }
                if (!hasGeometry)
                    return;
            }
            else if (niGeometry->recType == Nif::RC_NiLines)
            {
                if (niGeometryData->recType != Nif::RC_NiLinesData)
                    return;
                auto data = static_cast<const Nif::NiLinesData*>(niGeometryData);
                const auto& line = data->lines;
                if (line.empty())
                    return;
                geometry->addPrimitiveSet(new osg::DrawElementsUShort(osg::PrimitiveSet::LINES, line.size(),
                                                                        (unsigned short*)line.data()));
            }
            handleNiGeometryData(geometry, niGeometryData, boundTextures, nifNode->name);

            // osg::Material properties are handled here for two reasons:
            // - if there are no vertex colors, we need to disable colorMode.
            // - there are 3 "overlapping" nif properties that all affect the osg::Material, handling them
            //   above the actual renderable would be tedious.
            std::vector<const Nif::Property*> drawableProps;
            collectDrawableProperties(nifNode, drawableProps);
            applyDrawableProperties(parentNode, drawableProps, composite, !niGeometryData->colors.empty(), animflags);
        }

        void handleGeometry(const Nif::Node* nifNode, osg::Group* parentNode, SceneUtil::CompositeStateSetUpdater* composite, const std::vector<unsigned int>& boundTextures, int animflags)
        {
            assert(isTypeGeometry(nifNode->recType));
            osg::ref_ptr<osg::Geometry> geom (new osg::Geometry);
            handleNiGeometry(nifNode, geom, parentNode, composite, boundTextures, animflags);
            // If the record had no valid geometry data in it, early-out
            if (geom->empty())
                return;
            osg::ref_ptr<osg::Drawable> drawable;
            for (Nif::ControllerPtr ctrl = nifNode->controller; !ctrl.empty(); ctrl = ctrl->next)
            {
                if (!(ctrl->flags & Nif::NiNode::ControllerFlag_Active))
                    continue;
                if(ctrl->recType == Nif::RC_NiGeomMorpherController)
                {
                    const Nif::NiGeomMorpherController* nimorphctrl = static_cast<const Nif::NiGeomMorpherController*>(ctrl.getPtr());
                    if (nimorphctrl->data.empty())
                        continue;
                    drawable = handleMorphGeometry(nimorphctrl, geom, parentNode, composite, boundTextures, animflags);

                    osg::ref_ptr<GeomMorpherController> morphctrl = new GeomMorpherController(nimorphctrl);
                    setupController(ctrl.getPtr(), morphctrl, animflags);
                    drawable->setUpdateCallback(morphctrl);
                    break;
                }
            }
            if (!drawable.get())
                drawable = geom;
            drawable->setName(nifNode->name);
            parentNode->addChild(drawable);
        }

        osg::ref_ptr<osg::Drawable> handleMorphGeometry(const Nif::NiGeomMorpherController* morpher, osg::ref_ptr<osg::Geometry> sourceGeometry, osg::Node* parentNode, SceneUtil::CompositeStateSetUpdater* composite, const std::vector<unsigned int>& boundTextures, int animflags)
        {
            osg::ref_ptr<SceneUtil::MorphGeometry> morphGeom = new SceneUtil::MorphGeometry;
            morphGeom->setSourceGeometry(sourceGeometry);

            const std::vector<Nif::NiMorphData::MorphData>& morphs = morpher->data.getPtr()->mMorphs;
            if (morphs.empty())
                return morphGeom;
            // Note we are not interested in morph 0, which just contains the original vertices
            for (unsigned int i = 1; i < morphs.size(); ++i)
                morphGeom->addMorphTarget(new osg::Vec3Array(morphs[i].mVertices.size(), morphs[i].mVertices.data()), 0.f);

            return morphGeom;
        }

        void handleSkinnedGeometry(const Nif::Node *nifNode, osg::Group *parentNode, SceneUtil::CompositeStateSetUpdater* composite,
                                          const std::vector<unsigned int>& boundTextures, int animflags)
        {
            assert(isTypeGeometry(nifNode->recType));
            osg::ref_ptr<osg::Geometry> geometry (new osg::Geometry);
            handleNiGeometry(nifNode, geometry, parentNode, composite, boundTextures, animflags);
            if (geometry->empty())
                return;
            osg::ref_ptr<SceneUtil::RigGeometry> rig(new SceneUtil::RigGeometry);
            rig->setSourceGeometry(geometry);
            rig->setName(nifNode->name);

            // Assign bone weights
            osg::ref_ptr<SceneUtil::RigGeometry::InfluenceMap> map (new SceneUtil::RigGeometry::InfluenceMap);

            const Nif::NiSkinInstance *skin = static_cast<const Nif::NiGeometry*>(nifNode)->skin.getPtr();
            const Nif::NiSkinData *data = skin->data.getPtr();
            const Nif::NodeList &bones = skin->bones;
            for(size_t i = 0;i < bones.length();i++)
            {
                std::string boneName = Misc::StringUtils::lowerCase(bones[i].getPtr()->name);

                SceneUtil::RigGeometry::BoneInfluence influence;
                const std::vector<Nif::NiSkinData::VertWeight> &weights = data->bones[i].weights;
                for(size_t j = 0;j < weights.size();j++)
                {
                    influence.mWeights.emplace_back(weights[j].vertex, weights[j].weight);
                }
                influence.mInvBindMatrix = data->bones[i].trafo.toMatrix();
                influence.mBoundSphere = osg::BoundingSpheref(data->bones[i].boundSphereCenter, data->bones[i].boundSphereRadius);

                map->mData.emplace_back(boneName, influence);
            }
            rig->setInfluenceMap(map);

            parentNode->addChild(rig);
        }

        osg::BlendFunc::BlendFuncMode getBlendMode(int mode)
        {
            switch(mode)
            {
            case 0: return osg::BlendFunc::ONE;
            case 1: return osg::BlendFunc::ZERO;
            case 2: return osg::BlendFunc::SRC_COLOR;
            case 3: return osg::BlendFunc::ONE_MINUS_SRC_COLOR;
            case 4: return osg::BlendFunc::DST_COLOR;
            case 5: return osg::BlendFunc::ONE_MINUS_DST_COLOR;
            case 6: return osg::BlendFunc::SRC_ALPHA;
            case 7: return osg::BlendFunc::ONE_MINUS_SRC_ALPHA;
            case 8: return osg::BlendFunc::DST_ALPHA;
            case 9: return osg::BlendFunc::ONE_MINUS_DST_ALPHA;
            case 10: return osg::BlendFunc::SRC_ALPHA_SATURATE;
            default:
                Log(Debug::Info) << "Unexpected blend mode: "<< mode << " in " << mFilename;
                return osg::BlendFunc::SRC_ALPHA;
            }
        }

        osg::AlphaFunc::ComparisonFunction getTestMode(int mode)
        {
            switch (mode)
            {
            case 0: return osg::AlphaFunc::ALWAYS;
            case 1: return osg::AlphaFunc::LESS;
            case 2: return osg::AlphaFunc::EQUAL;
            case 3: return osg::AlphaFunc::LEQUAL;
            case 4: return osg::AlphaFunc::GREATER;
            case 5: return osg::AlphaFunc::NOTEQUAL;
            case 6: return osg::AlphaFunc::GEQUAL;
            case 7: return osg::AlphaFunc::NEVER;
            default:
                Log(Debug::Info) << "Unexpected blend mode: " << mode << " in " << mFilename;
                return osg::AlphaFunc::LEQUAL;
            }
        }

        osg::Stencil::Function getStencilFunction(int func)
        {
            switch (func)
            {
            case 0: return osg::Stencil::NEVER;
            case 1: return osg::Stencil::LESS;
            case 2: return osg::Stencil::EQUAL;
            case 3: return osg::Stencil::LEQUAL;
            case 4: return osg::Stencil::GREATER;
            case 5: return osg::Stencil::NOTEQUAL;
            case 6: return osg::Stencil::GEQUAL;
            case 7: return osg::Stencil::NEVER; // NifSkope says this is GL_ALWAYS, but in MW it's GL_NEVER
            default:
                Log(Debug::Info) << "Unexpected stencil function: " << func << " in " << mFilename;
                return osg::Stencil::NEVER;
            }
        }

        osg::Stencil::Operation getStencilOperation(int op)
        {
            switch (op)
            {
            case 0: return osg::Stencil::KEEP;
            case 1: return osg::Stencil::ZERO;
            case 2: return osg::Stencil::REPLACE;
            case 3: return osg::Stencil::INCR;
            case 4: return osg::Stencil::DECR;
            case 5: return osg::Stencil::INVERT;
            default:
                Log(Debug::Info) << "Unexpected stencil operation: " << op << " in " << mFilename;
                return osg::Stencil::KEEP;
            }
        }

        osg::ref_ptr<osg::Image> handleInternalTexture(const Nif::NiPixelData* pixelData)
        {
            osg::ref_ptr<osg::Image> image (new osg::Image);

            GLenum pixelformat = 0;
            switch (pixelData->fmt)
            {
            case Nif::NiPixelData::NIPXFMT_RGB8:
            case Nif::NiPixelData::NIPXFMT_PAL8:
                pixelformat = GL_RGB;
                break;
            case Nif::NiPixelData::NIPXFMT_RGBA8:
            case Nif::NiPixelData::NIPXFMT_PALA8:
                pixelformat = GL_RGBA;
                break;
            default:
                Log(Debug::Info) << "Unhandled internal pixel format " << pixelData->fmt << " in " << mFilename;
                return nullptr;
            }

            if (pixelData->mipmaps.empty())
                return nullptr;

            int width = 0;
            int height = 0;

            std::vector<unsigned int> mipmapVector;
            for (unsigned int i=0; i<pixelData->mipmaps.size(); ++i)
            {
                const Nif::NiPixelData::Mipmap& mip = pixelData->mipmaps[i];

                size_t mipSize = mip.height * mip.width * pixelData->bpp / 8;
                if (mipSize + mip.dataOffset > pixelData->data.size())
                {
                    Log(Debug::Info) << "Internal texture's mipmap data out of bounds, ignoring texture";
                    return nullptr;
                }

                if (i != 0)
                    mipmapVector.push_back(mip.dataOffset);
                else
                {
                    width = mip.width;
                    height = mip.height;
                }
            }

            if (width <= 0 || height <= 0)
            {
                Log(Debug::Info) << "Internal Texture Width and height must be non zero, ignoring texture";
                return nullptr;
            }

            const std::vector<unsigned char>& pixels = pixelData->data;
            switch (pixelData->fmt)
            {
            case Nif::NiPixelData::NIPXFMT_RGB8:
            case Nif::NiPixelData::NIPXFMT_RGBA8:
            {
                unsigned char* data = new unsigned char[pixels.size()];
                memcpy(data, pixels.data(), pixels.size());
                image->setImage(width, height, 1, pixelformat, pixelformat, GL_UNSIGNED_BYTE, data, osg::Image::USE_NEW_DELETE);
                break;
            }
            case Nif::NiPixelData::NIPXFMT_PAL8:
            case Nif::NiPixelData::NIPXFMT_PALA8:
            {
                if (pixelData->palette.empty() || pixelData->bpp != 8)
                {
                    Log(Debug::Info) << "Palettized texture in " << mFilename << " is invalid, ignoring";
                    return nullptr;
                }
                // We're going to convert the indices that pixel data contains
                // into real colors using the palette.
                const auto& palette = pixelData->palette->colors;
                const int numChannels = pixelformat == GL_RGBA ? 4 : 3;
                unsigned char* data = new unsigned char[pixels.size() * numChannels];
                unsigned char* pixel = data;
                for (unsigned char index : pixels)
                {
                    memcpy(pixel, &palette[index], sizeof(unsigned char) * numChannels);
                    pixel += numChannels;
                }
                image->setImage(width, height, 1, pixelformat, pixelformat, GL_UNSIGNED_BYTE, data, osg::Image::USE_NEW_DELETE);
                break;
            }
            default:
                return nullptr;
            }

            image->setMipmapLevels(mipmapVector);
            image->flipVertical();

            return image;
        }

        osg::ref_ptr<osg::TexEnvCombine> createEmissiveTexEnv()
        {
            osg::ref_ptr<osg::TexEnvCombine> texEnv(new osg::TexEnvCombine);
            texEnv->setCombine_Alpha(osg::TexEnvCombine::REPLACE);
            texEnv->setSource0_Alpha(osg::TexEnvCombine::PREVIOUS);
            texEnv->setCombine_RGB(osg::TexEnvCombine::ADD);
            texEnv->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
            texEnv->setSource1_RGB(osg::TexEnvCombine::TEXTURE);
            return texEnv;
        }

        void handleTextureProperty(const Nif::NiTexturingProperty* texprop, const std::string& nodeName, osg::StateSet* stateset, SceneUtil::CompositeStateSetUpdater* composite, Resource::ImageManager* imageManager, std::vector<unsigned int>& boundTextures, int animflags)
        {
            if (!boundTextures.empty())
            {
                // overriding a parent NiTexturingProperty, so remove what was previously bound
                for (unsigned int i=0; i<boundTextures.size(); ++i)
                    stateset->setTextureMode(i, GL_TEXTURE_2D, osg::StateAttribute::OFF);
                boundTextures.clear();
            }

            // If this loop is changed such that the base texture isn't guaranteed to end up in texture unit 0, the shadow casting shader will need to be updated accordingly.
            for (size_t i=0; i<texprop->textures.size(); ++i)
            {
                if (texprop->textures[i].inUse || (i == Nif::NiTexturingProperty::BaseTexture && !texprop->controller.empty()))
                {
                    switch(i)
                    {
                        //These are handled later on
                        case Nif::NiTexturingProperty::BaseTexture:
                        case Nif::NiTexturingProperty::GlowTexture:
                        case Nif::NiTexturingProperty::DarkTexture:
                        case Nif::NiTexturingProperty::BumpTexture:
                        case Nif::NiTexturingProperty::DetailTexture:
                        case Nif::NiTexturingProperty::DecalTexture:
                            break;
                        case Nif::NiTexturingProperty::GlossTexture:
                        {
                            // Not used by the vanilla engine. MCP (Morrowind Code Patch) adds an option to use Gloss maps:
                            // "- Gloss map fix. Morrowind removed gloss map entries from model files after loading them. This stops Morrowind from removing them."
                            // Log(Debug::Info) << "NiTexturingProperty::GlossTexture in " << mFilename << " not currently used.";
                            continue;
                        }
                        default:
                        {
                            Log(Debug::Info) << "Unhandled texture stage " << i << " on shape \"" << nodeName << "\" in " << mFilename;
                            continue;
                        }
                    }

                    unsigned int uvSet = 0;
                    // create a new texture, will later attempt to share using the SharedStateManager
                    osg::ref_ptr<osg::Texture2D> texture2d;
                    if (texprop->textures[i].inUse)
                    {
                        const Nif::NiTexturingProperty::Texture& tex = texprop->textures[i];
                        if(tex.texture.empty() && texprop->controller.empty())
                        {
                            if (i == 0)
                                Log(Debug::Warning) << "Base texture is in use but empty on shape \"" << nodeName << "\" in " << mFilename;
                            continue;
                        }

                        if (!tex.texture.empty())
                        {
                            const Nif::NiSourceTexture *st = tex.texture.getPtr();
                            osg::ref_ptr<osg::Image> image = handleSourceTexture(st, imageManager);
                            texture2d = new osg::Texture2D(image);
                            if (image)
                                texture2d->setTextureSize(image->s(), image->t());
                        }
                        else
                            texture2d = new osg::Texture2D;

                        bool wrapT = tex.clamp & 0x1;
                        bool wrapS = (tex.clamp >> 1) & 0x1;

                        texture2d->setWrap(osg::Texture::WRAP_S, wrapS ? osg::Texture::REPEAT : osg::Texture::CLAMP_TO_EDGE);
                        texture2d->setWrap(osg::Texture::WRAP_T, wrapT ? osg::Texture::REPEAT : osg::Texture::CLAMP_TO_EDGE);

                        uvSet = tex.uvSet;
                    }
                    else
                    {
                        // Texture only comes from NiFlipController, so tex is ignored, set defaults
                        texture2d = new osg::Texture2D;
                        texture2d->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
                        texture2d->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
                        uvSet = 0;
                    }

                    unsigned int texUnit = boundTextures.size();

                    stateset->setTextureAttributeAndModes(texUnit, texture2d, osg::StateAttribute::ON);

                    if (i == Nif::NiTexturingProperty::GlowTexture)
                    {
                        stateset->setTextureAttributeAndModes(texUnit, createEmissiveTexEnv(), osg::StateAttribute::ON);
                    }
                    else if (i == Nif::NiTexturingProperty::DarkTexture)
                    {
                        osg::TexEnv* texEnv = new osg::TexEnv;
                        texEnv->setMode(osg::TexEnv::MODULATE);
                        stateset->setTextureAttributeAndModes(texUnit, texEnv, osg::StateAttribute::ON);
                    }
                    else if (i == Nif::NiTexturingProperty::DetailTexture)
                    {
                        osg::TexEnvCombine* texEnv = new osg::TexEnvCombine;
                        texEnv->setScale_RGB(2.f);
                        texEnv->setCombine_Alpha(osg::TexEnvCombine::MODULATE);
                        texEnv->setOperand0_Alpha(osg::TexEnvCombine::SRC_ALPHA);
                        texEnv->setOperand1_Alpha(osg::TexEnvCombine::SRC_ALPHA);
                        texEnv->setSource0_Alpha(osg::TexEnvCombine::PREVIOUS);
                        texEnv->setSource1_Alpha(osg::TexEnvCombine::TEXTURE);
                        texEnv->setCombine_RGB(osg::TexEnvCombine::MODULATE);
                        texEnv->setOperand0_RGB(osg::TexEnvCombine::SRC_COLOR);
                        texEnv->setOperand1_RGB(osg::TexEnvCombine::SRC_COLOR);
                        texEnv->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
                        texEnv->setSource1_RGB(osg::TexEnvCombine::TEXTURE);
                        stateset->setTextureAttributeAndModes(texUnit, texEnv, osg::StateAttribute::ON);
                    }
                    else if (i == Nif::NiTexturingProperty::BumpTexture)
                    {
                        // Set this texture to Off by default since we can't render it with the fixed-function pipeline
                        stateset->setTextureMode(texUnit, GL_TEXTURE_2D, osg::StateAttribute::OFF);
                        osg::Matrix2 bumpMapMatrix(texprop->bumpMapMatrix.x(), texprop->bumpMapMatrix.y(),
                                                   texprop->bumpMapMatrix.z(), texprop->bumpMapMatrix.w());
                        stateset->addUniform(new osg::Uniform("bumpMapMatrix", bumpMapMatrix));
                        stateset->addUniform(new osg::Uniform("envMapLumaBias", texprop->envMapLumaBias));
                    }
                    else if (i == Nif::NiTexturingProperty::DecalTexture)
                    {
                         osg::TexEnvCombine* texEnv = new osg::TexEnvCombine;
                         texEnv->setCombine_RGB(osg::TexEnvCombine::INTERPOLATE);
                         texEnv->setSource0_RGB(osg::TexEnvCombine::TEXTURE);
                         texEnv->setOperand0_RGB(osg::TexEnvCombine::SRC_COLOR);
                         texEnv->setSource1_RGB(osg::TexEnvCombine::PREVIOUS);
                         texEnv->setOperand1_RGB(osg::TexEnvCombine::SRC_COLOR);
                         texEnv->setSource2_RGB(osg::TexEnvCombine::TEXTURE);
                         texEnv->setOperand2_RGB(osg::TexEnvCombine::SRC_ALPHA);
                         texEnv->setCombine_Alpha(osg::TexEnvCombine::REPLACE);
                         texEnv->setSource0_Alpha(osg::TexEnvCombine::PREVIOUS);
                         texEnv->setOperand0_Alpha(osg::TexEnvCombine::SRC_ALPHA);
                         stateset->setTextureAttributeAndModes(texUnit, texEnv, osg::StateAttribute::ON);
                    }

                    switch (i)
                    {
                    case Nif::NiTexturingProperty::BaseTexture:
                        texture2d->setName("diffuseMap");
                        break;
                    case Nif::NiTexturingProperty::BumpTexture:
                        texture2d->setName("bumpMap");
                        break;
                    case Nif::NiTexturingProperty::GlowTexture:
                        texture2d->setName("emissiveMap");
                        break;
                    case Nif::NiTexturingProperty::DarkTexture:
                        texture2d->setName("darkMap");
                        break;
                    case Nif::NiTexturingProperty::DetailTexture:
                        texture2d->setName("detailMap");
                        break;
                    case Nif::NiTexturingProperty::DecalTexture:
                        texture2d->setName("decalMap");
                        break;
                    default:
                        break;
                    }

                    boundTextures.push_back(uvSet);
                }
            }
            handleTextureControllers(texprop, composite, imageManager, stateset, animflags);
        }

        void handleTextureSet(const Nif::BSShaderTextureSet* textureSet, unsigned int clamp, const std::string& nodeName, osg::StateSet* stateset, Resource::ImageManager* imageManager, std::vector<unsigned int>& boundTextures)
        {
            if (!boundTextures.empty())
            {
                for (unsigned int i = 0; i < boundTextures.size(); ++i)
                    stateset->setTextureMode(i, GL_TEXTURE_2D, osg::StateAttribute::OFF);
                boundTextures.clear();
            }

            const unsigned int uvSet = 0;

            for (size_t i = 0; i < textureSet->textures.size(); ++i)
            {
                if (textureSet->textures[i].empty())
                    continue;
                switch(i)
                {
                    case Nif::BSShaderTextureSet::TextureType_Base:
                    case Nif::BSShaderTextureSet::TextureType_Normal:
                    case Nif::BSShaderTextureSet::TextureType_Glow:
                        break;
                    default:
                    {
                        Log(Debug::Info) << "Unhandled texture stage " << i << " on shape \"" << nodeName << "\" in " << mFilename;
                        continue;
                    }
                }
                std::string filename = Misc::ResourceHelpers::correctTexturePath(textureSet->textures[i], imageManager->getVFS());
                osg::ref_ptr<osg::Image> image = imageManager->getImage(filename);
                osg::ref_ptr<osg::Texture2D> texture2d = new osg::Texture2D(image);
                if (image)
                    texture2d->setTextureSize(image->s(), image->t());
                bool wrapT = clamp & 0x1;
                bool wrapS = (clamp >> 1) & 0x1;
                texture2d->setWrap(osg::Texture::WRAP_S, wrapS ? osg::Texture::REPEAT : osg::Texture::CLAMP_TO_EDGE);
                texture2d->setWrap(osg::Texture::WRAP_T, wrapT ? osg::Texture::REPEAT : osg::Texture::CLAMP_TO_EDGE);
                unsigned int texUnit = boundTextures.size();
                stateset->setTextureAttributeAndModes(texUnit, texture2d, osg::StateAttribute::ON);
                // BSShaderTextureSet presence means there's no need for FFP support for the affected node
                switch (i)
                {
                    case Nif::BSShaderTextureSet::TextureType_Base:
                        texture2d->setName("diffuseMap");
                        break;
                    case Nif::BSShaderTextureSet::TextureType_Normal:
                        texture2d->setName("normalMap");
                        break;
                    case Nif::BSShaderTextureSet::TextureType_Glow:
                        texture2d->setName("emissiveMap");
                        break;
                }
                boundTextures.emplace_back(uvSet);
            }
        }

        const std::string& getNVShaderPrefix(unsigned int type) const
        {
            static const std::map<unsigned int, std::string> mapping =
            {
                {Nif::BSShaderProperty::SHADER_TALL_GRASS, std::string()},
                {Nif::BSShaderProperty::SHADER_DEFAULT,    "nv_default"},
                {Nif::BSShaderProperty::SHADER_SKY,        std::string()},
                {Nif::BSShaderProperty::SHADER_SKIN,       std::string()},
                {Nif::BSShaderProperty::SHADER_WATER,      std::string()},
                {Nif::BSShaderProperty::SHADER_LIGHTING30, std::string()},
                {Nif::BSShaderProperty::SHADER_TILE,       std::string()},
                {Nif::BSShaderProperty::SHADER_NOLIGHTING, "nv_nolighting"},
            };
            auto prefix = mapping.find(type);
            if (prefix == mapping.end())
                Log(Debug::Warning) << "Unknown shader type " << type << " in " << mFilename;
            else if (prefix->second.empty())
                Log(Debug::Warning) << "Unhandled shader type " << type << " in " << mFilename;
            else
                return prefix->second;

            return mapping.at(Nif::BSShaderProperty::SHADER_DEFAULT);
        }

        void handleProperty(const Nif::Property *property,
                            osg::Node *node, SceneUtil::CompositeStateSetUpdater* composite, Resource::ImageManager* imageManager, std::vector<unsigned int>& boundTextures, int animflags)
        {
            switch (property->recType)
            {
            case Nif::RC_NiStencilProperty:
            {
                const Nif::NiStencilProperty* stencilprop = static_cast<const Nif::NiStencilProperty*>(property);
                osg::ref_ptr<osg::FrontFace> frontFace = new osg::FrontFace;
                switch (stencilprop->data.drawMode)
                {
                case 2:
                    frontFace->setMode(osg::FrontFace::CLOCKWISE);
                    break;
                case 0:
                case 1:
                default:
                    frontFace->setMode(osg::FrontFace::COUNTER_CLOCKWISE);
                    break;
                }
                frontFace = shareAttribute(frontFace);

                osg::StateSet* stateset = node->getOrCreateStateSet();
                stateset->setAttribute(frontFace, osg::StateAttribute::ON);
                stateset->setMode(GL_CULL_FACE, stencilprop->data.drawMode == 3 ? osg::StateAttribute::OFF
                                                                                : osg::StateAttribute::ON);

                if (stencilprop->data.enabled != 0)
                {
                    osg::ref_ptr<osg::Stencil> stencil = new osg::Stencil;
                    stencil->setFunction(getStencilFunction(stencilprop->data.compareFunc), stencilprop->data.stencilRef, stencilprop->data.stencilMask);
                    stencil->setStencilFailOperation(getStencilOperation(stencilprop->data.failAction));
                    stencil->setStencilPassAndDepthFailOperation(getStencilOperation(stencilprop->data.zFailAction));
                    stencil->setStencilPassAndDepthPassOperation(getStencilOperation(stencilprop->data.zPassAction));
                    stencil = shareAttribute(stencil);

                    stateset->setAttributeAndModes(stencil, osg::StateAttribute::ON);
                }
                break;
            }
            case Nif::RC_NiWireframeProperty:
            {
                const Nif::NiWireframeProperty* wireprop = static_cast<const Nif::NiWireframeProperty*>(property);
                osg::ref_ptr<osg::PolygonMode> mode = new osg::PolygonMode;
                mode->setMode(osg::PolygonMode::FRONT_AND_BACK, wireprop->flags == 0 ? osg::PolygonMode::FILL
                                                                                     : osg::PolygonMode::LINE);
                mode = shareAttribute(mode);
                node->getOrCreateStateSet()->setAttributeAndModes(mode, osg::StateAttribute::ON);
                break;
            }
            case Nif::RC_NiZBufferProperty:
            {
                const Nif::NiZBufferProperty* zprop = static_cast<const Nif::NiZBufferProperty*>(property);
                osg::StateSet* stateset = node->getOrCreateStateSet();
                // Depth test flag
                stateset->setMode(GL_DEPTH_TEST, zprop->flags&1 ? osg::StateAttribute::ON
                                                                : osg::StateAttribute::OFF);
                osg::ref_ptr<osg::Depth> depth = new osg::Depth;
                // Depth write flag
                depth->setWriteMask((zprop->flags>>1)&1);
                // Morrowind ignores depth test function
                depth = shareAttribute(depth);
                stateset->setAttributeAndModes(depth, osg::StateAttribute::ON);
                break;
            }
            // OSG groups the material properties that NIFs have separate, so we have to parse them all again when one changed
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
                handleTextureProperty(texprop, node->getName(), stateset, composite, imageManager, boundTextures, animflags);
                break;
            }
            case Nif::RC_BSShaderPPLightingProperty:
            {
                auto texprop = static_cast<const Nif::BSShaderPPLightingProperty*>(property);
                bool shaderRequired = true;
                node->setUserValue("shaderPrefix", getNVShaderPrefix(texprop->type));
                node->setUserValue("shaderRequired", shaderRequired);
                osg::StateSet* stateset = node->getOrCreateStateSet();
                if (!texprop->textureSet.empty())
                {
                    auto textureSet = texprop->textureSet.getPtr();
                    handleTextureSet(textureSet, texprop->clamp, node->getName(), stateset, imageManager, boundTextures);
                }
                handleTextureControllers(texprop, composite, imageManager, stateset, animflags);
                break;
            }
            case Nif::RC_BSShaderNoLightingProperty:
            {
                auto texprop = static_cast<const Nif::BSShaderNoLightingProperty*>(property);
                bool shaderRequired = true;
                node->setUserValue("shaderPrefix", getNVShaderPrefix(texprop->type));
                node->setUserValue("shaderRequired", shaderRequired);
                osg::StateSet* stateset = node->getOrCreateStateSet();
                if (!texprop->filename.empty())
                {
                    if (!boundTextures.empty())
                    {
                        for (unsigned int i = 0; i < boundTextures.size(); ++i)
                            stateset->setTextureMode(i, GL_TEXTURE_2D, osg::StateAttribute::OFF);
                        boundTextures.clear();
                    }
                    std::string filename = Misc::ResourceHelpers::correctTexturePath(texprop->filename, imageManager->getVFS());
                    osg::ref_ptr<osg::Image> image = imageManager->getImage(filename);
                    osg::ref_ptr<osg::Texture2D> texture2d = new osg::Texture2D(image);
                    texture2d->setName("diffuseMap");
                    if (image)
                        texture2d->setTextureSize(image->s(), image->t());
                    bool wrapT = texprop->clamp & 0x1;
                    bool wrapS = (texprop->clamp >> 1) & 0x1;
                    texture2d->setWrap(osg::Texture::WRAP_S, wrapS ? osg::Texture::REPEAT : osg::Texture::CLAMP_TO_EDGE);
                    texture2d->setWrap(osg::Texture::WRAP_T, wrapT ? osg::Texture::REPEAT : osg::Texture::CLAMP_TO_EDGE);
                    const unsigned int texUnit = 0;
                    const unsigned int uvSet = 0;
                    stateset->setTextureAttributeAndModes(texUnit, texture2d, osg::StateAttribute::ON);
                    boundTextures.push_back(uvSet);
                }
                if (mBethVersion >= 27)
                {
                    stateset->addUniform(new osg::Uniform("useFalloff", true));
                    stateset->addUniform(new osg::Uniform("falloffParams", texprop->falloffParams));
                }
                else
                {
                    stateset->addUniform(new osg::Uniform("useFalloff", false));
                }
                handleTextureControllers(texprop, composite, imageManager, stateset, animflags);
                break;
            }
            // unused by mw
            case Nif::RC_NiShadeProperty:
            case Nif::RC_NiDitherProperty:
            case Nif::RC_NiFogProperty:
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
            bool operator() (const osg::ref_ptr<osg::StateAttribute>& left, const osg::ref_ptr<osg::StateAttribute>& right) const
            {
                return left->compare(*right) < 0;
            }
        };

        // global sharing of State Attributes will reduce the number of GL calls as the osg::State will check by pointer to see if state is the same
        template <class Attribute>
        Attribute* shareAttribute(const osg::ref_ptr<Attribute>& attr)
        {
            typedef std::set<osg::ref_ptr<Attribute>, CompareStateAttribute> Cache;
            static Cache sCache;
            static std::mutex sMutex;
            std::lock_guard<std::mutex> lock(sMutex);
            typename Cache::iterator found = sCache.find(attr);
            if (found == sCache.end())
                found = sCache.insert(attr).first;
            return *found;
        }

        void applyDrawableProperties(osg::Node* node, const std::vector<const Nif::Property*>& properties, SceneUtil::CompositeStateSetUpdater* composite,
                                             bool hasVertexColors, int animflags)
        {
            osg::StateSet* stateset = node->getOrCreateStateSet();

            // Specular lighting is enabled by default, but there's a quirk...
            bool specEnabled = true;
            osg::ref_ptr<osg::Material> mat (new osg::Material);
            mat->setColorMode(hasVertexColors ? osg::Material::AMBIENT_AND_DIFFUSE : osg::Material::OFF);

            // NIF material defaults don't match OpenGL defaults
            mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,1));
            mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,1));

            bool hasMatCtrl = false;

            int lightmode = 1;
            float emissiveMult = 1.f;

            for (const Nif::Property* property : properties)
            {
                switch (property->recType)
                {
                case Nif::RC_NiSpecularProperty:
                {
                    // Specular property can turn specular lighting off.
                    auto specprop = static_cast<const Nif::NiSpecularProperty*>(property);
                    specEnabled = specprop->flags & 1;
                    break;
                }
                case Nif::RC_NiMaterialProperty:
                {
                    const Nif::NiMaterialProperty* matprop = static_cast<const Nif::NiMaterialProperty*>(property);

                    mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(matprop->data.diffuse, matprop->data.alpha));
                    mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(matprop->data.ambient, 1.f));
                    mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(matprop->data.emissive, 1.f));
                    emissiveMult = matprop->data.emissiveMult;

                    mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(matprop->data.specular, 1.f));
                    mat->setShininess(osg::Material::FRONT_AND_BACK, matprop->data.glossiness);

                    if (!matprop->controller.empty())
                    {
                        hasMatCtrl = true;
                        handleMaterialControllers(matprop, composite, animflags, mat);
                    }

                    break;
                }
                case Nif::RC_NiVertexColorProperty:
                {
                    const Nif::NiVertexColorProperty* vertprop = static_cast<const Nif::NiVertexColorProperty*>(property);
                    lightmode = vertprop->data.lightmode;

                    switch (vertprop->data.vertmode)
                    {
                        case 0:
                            mat->setColorMode(osg::Material::OFF);
                            break;
                        case 1:
                            mat->setColorMode(osg::Material::EMISSION);
                            break;
                        case 2:
                            if (lightmode != 0)
                                mat->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
                            else
                                mat->setColorMode(osg::Material::OFF);
                            break;
                    }
                    break;
                }
                case Nif::RC_NiAlphaProperty:
                {
                    const Nif::NiAlphaProperty* alphaprop = static_cast<const Nif::NiAlphaProperty*>(property);
                    if (alphaprop->flags&1)
                    {
                        osg::ref_ptr<osg::BlendFunc> blendFunc (new osg::BlendFunc(getBlendMode((alphaprop->flags>>1)&0xf),
                                                                                   getBlendMode((alphaprop->flags>>5)&0xf)));
                        // on AMD hardware, alpha still seems to be stored with an RGBA framebuffer with OpenGL.
                        // This might be mandated by the OpenGL 2.1 specification section 2.14.9, or might be a bug.
                        // Either way, D3D8.1 doesn't do that, so adapt the destination factor.
                        if (blendFunc->getDestination() == GL_DST_ALPHA)
                            blendFunc->setDestination(GL_ONE);
                        blendFunc = shareAttribute(blendFunc);
                        stateset->setAttributeAndModes(blendFunc, osg::StateAttribute::ON);

                        bool noSort = (alphaprop->flags>>13)&1;
                        if (!noSort)
                            stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
                        else
                            stateset->setRenderBinToInherit();
                    }
                    else
                    {
                        stateset->removeAttribute(osg::StateAttribute::BLENDFUNC);
                        stateset->removeMode(GL_BLEND);
                        stateset->setRenderBinToInherit();
                    }

                    if((alphaprop->flags>>9)&1)
                    {
                        osg::ref_ptr<osg::AlphaFunc> alphaFunc (new osg::AlphaFunc(getTestMode((alphaprop->flags>>10)&0x7), alphaprop->data.threshold/255.f));
                        alphaFunc = shareAttribute(alphaFunc);
                        stateset->setAttributeAndModes(alphaFunc, osg::StateAttribute::ON);
                    }
                    else
                    {
                        stateset->removeAttribute(osg::StateAttribute::ALPHAFUNC);
                        stateset->removeMode(GL_ALPHA_TEST);
                    }
                    break;
                }
                }
            }

            // While NetImmerse and Gamebryo support specular lighting, Morrowind has its support disabled.
            if (mVersion <= Nif::NIFFile::NIFVersion::VER_MW || !specEnabled)
                mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f,0.f,0.f,0.f));

            if (lightmode == 0)
            {
                osg::Vec4f diffuse = mat->getDiffuse(osg::Material::FRONT_AND_BACK);
                diffuse = osg::Vec4f(0,0,0,diffuse.a());
                mat->setDiffuse(osg::Material::FRONT_AND_BACK, diffuse);
                mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f());
            }

            // If we're told to use vertex colors but there are none to use, use a default color instead.
            if (!hasVertexColors)
            {
                switch (mat->getColorMode())
                {
                case osg::Material::AMBIENT:
                    mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,1));
                    break;
                case osg::Material::AMBIENT_AND_DIFFUSE:
                    mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,1));
                    mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,1));
                    break;
                case osg::Material::EMISSION:
                    mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,1));
                    break;
                default:
                    break;
                }
                mat->setColorMode(osg::Material::OFF);
            }

            if (!hasMatCtrl && mat->getColorMode() == osg::Material::OFF
                    && mat->getEmission(osg::Material::FRONT_AND_BACK) == osg::Vec4f(0,0,0,1)
                    && mat->getDiffuse(osg::Material::FRONT_AND_BACK) == osg::Vec4f(1,1,1,1)
                    && mat->getAmbient(osg::Material::FRONT_AND_BACK) == osg::Vec4f(1,1,1,1)
                    && mat->getShininess(osg::Material::FRONT_AND_BACK) == 0
                    && mat->getSpecular(osg::Material::FRONT_AND_BACK) == osg::Vec4f(0.f, 0.f, 0.f, 0.f))
            {
                // default state, skip
                return;
            }

            mat = shareAttribute(mat);

            stateset->setAttributeAndModes(mat, osg::StateAttribute::ON);
            stateset->addUniform(new osg::Uniform("emissiveMult", emissiveMult));
        }

    };

    osg::ref_ptr<osg::Node> Loader::load(Nif::NIFFilePtr file, Resource::ImageManager* imageManager)
    {
        LoaderImpl impl(file->getFilename(), file->getVersion(), file->getUserVersion(), file->getBethVersion());
        return impl.load(file, imageManager);
    }

    void Loader::loadKf(Nif::NIFFilePtr kf, SceneUtil::KeyframeHolder& target)
    {
        LoaderImpl impl(kf->getFilename(), kf->getVersion(), kf->getUserVersion(), kf->getBethVersion());
        impl.loadKf(kf, target);
    }

}
