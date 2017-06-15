#include "nifloader.hpp"

#include <osg/Matrixf>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osg/Array>
#include <osg/LOD>
#include <osg/TexGen>
#include <osg/ValueObject>

// resource
#include <components/misc/stringops.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/resource/imagemanager.hpp>

// skel
#include <osgAnimation/MorphGeometry>

// particle
#include <osgParticle/ParticleSystem>
#include <osgParticle/ParticleSystemUpdater>
#include <osgParticle/ConstantRateCounter>
#include <osgParticle/Shooter>
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

#include "particle.hpp"
#include "userdata.hpp"

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

    // Collect all properties affecting the given drawable that should be handled on drawable basis rather than on the node hierarchy above it.
    void collectDrawableProperties(const Nif::Node* nifNode, std::vector<const Nif::Property*>& out)
    {
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
        if (nifNode->parent)
            collectDrawableProperties(nifNode->parent, out);
    }

    class FrameSwitch : public osg::Group
    {
    public:
        FrameSwitch()
        {
        }

        FrameSwitch(const FrameSwitch& copy, const osg::CopyOp& copyop)
            : osg::Group(copy, copyop)
        {
        }

        META_Object(NifOsg, FrameSwitch)

        virtual void traverse(osg::NodeVisitor& nv)
        {
            if (nv.getTraversalMode() != osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN && nv.getVisitorType() != osg::NodeVisitor::UPDATE_VISITOR)
                osg::Group::traverse(nv);
            else
            {
                for (unsigned int i=0; i<getNumChildren(); ++i)
                {
                    if (i%2 == nv.getTraversalNumber()%2)
                        getChild(i)->accept(nv);
                }
            }
        }
    };

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

        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
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

    struct UpdateMorphGeometry : public osg::Drawable::CullCallback
    {
        UpdateMorphGeometry()
            : mLastFrameNumber(0)
        {
        }

        UpdateMorphGeometry(const UpdateMorphGeometry& copy, const osg::CopyOp& copyop)
            : osg::Drawable::CullCallback(copy, copyop)
            , mLastFrameNumber(0)
        {
        }

        META_Object(NifOsg, UpdateMorphGeometry)

        virtual bool cull(osg::NodeVisitor* nv, osg::Drawable * drw, osg::State *) const
        {
            osgAnimation::MorphGeometry* geom = static_cast<osgAnimation::MorphGeometry*>(drw);
            if (!geom)
                return false;

            if (mLastFrameNumber == nv->getTraversalNumber())
                return false;
            mLastFrameNumber = nv->getTraversalNumber();

            geom->transformSoftwareMethod();
            return false;
        }

    private:
        mutable unsigned int mLastFrameNumber;
    };

    // Callback to return a static bounding box for a MorphGeometry. The idea is to not recalculate the bounding box
    // every time the morph weights change. To do so we return a maximum containing box that is big enough for all possible combinations of morph targets.
    class StaticBoundingBoxCallback : public osg::Drawable::ComputeBoundingBoxCallback
    {
    public:
        StaticBoundingBoxCallback()
        {
        }

        StaticBoundingBoxCallback(const osg::BoundingBox& bounds)
            : mBoundingBox(bounds)
        {
        }

        StaticBoundingBoxCallback(const StaticBoundingBoxCallback& copy, const osg::CopyOp& copyop)
            : osg::Drawable::ComputeBoundingBoxCallback(copy, copyop)
            , mBoundingBox(copy.mBoundingBox)
        {
        }

        META_Object(NifOsg, StaticBoundingBoxCallback)

        virtual osg::BoundingBox computeBound(const osg::Drawable&) const
        {
            return mBoundingBox;
        }

    private:
        osg::BoundingBox mBoundingBox;
    };

    void extractTextKeys(const Nif::NiTextKeyExtraData *tk, NifOsg::TextKeyMap &textkeys)
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
                textkeys.insert(std::make_pair(tk->list[i].time, Misc::StringUtils::lowerCase(result)));

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

    class LoaderImpl
    {
    public:
        /// @param filename used for warning messages.
        LoaderImpl(const std::string& filename)
            : mFilename(filename), mFirstRootTextureIndex(-1), mFoundFirstRootTexturingProperty(false)
        {

        }
        std::string mFilename;
        size_t mFirstRootTextureIndex;
        bool mFoundFirstRootTexturingProperty;

        static void loadKf(Nif::NIFFilePtr nif, KeyframeHolder& target)
        {
            if(nif->numRoots() < 1)
            {
                nif->warn("Found no root nodes");
                return;
            }

            const Nif::Record *r = nif->getRoot(0);
            assert(r != NULL);

            if(r->recType != Nif::RC_NiSequenceStreamHelper)
            {
                nif->warn("First root was not a NiSequenceStreamHelper, but a "+
                          r->recName+".");
                return;
            }
            const Nif::NiSequenceStreamHelper *seq = static_cast<const Nif::NiSequenceStreamHelper*>(r);

            Nif::ExtraPtr extra = seq->extra;
            if(extra.empty() || extra->recType != Nif::RC_NiTextKeyExtraData)
            {
                nif->warn("First extra data was not a NiTextKeyExtraData, but a "+
                          (extra.empty() ? std::string("nil") : extra->recName)+".");
                return;
            }

            extractTextKeys(static_cast<const Nif::NiTextKeyExtraData*>(extra.getPtr()), target.mTextKeys);

            extra = extra->extra;
            Nif::ControllerPtr ctrl = seq->controller;
            for(;!extra.empty() && !ctrl.empty();(extra=extra->extra),(ctrl=ctrl->next))
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

                if(key->data.empty())
                    continue;

                osg::ref_ptr<NifOsg::KeyframeController> callback(new NifOsg::KeyframeController(key->data.getPtr()));
                callback->setFunction(std::shared_ptr<NifOsg::ControllerFunction>(new NifOsg::ControllerFunction(key)));

                if (target.mKeyframeControllers.find(strdata->string) != target.mKeyframeControllers.end())
                    std::cerr << "Warning: controller " << strdata->string << " present more than once in " << nif->getFilename() << ", ignoring later version" << std::endl;
                else
                    target.mKeyframeControllers[strdata->string] = callback;
            }
        }

        osg::ref_ptr<osg::Node> load(Nif::NIFFilePtr nif, Resource::ImageManager* imageManager)
        {
            if (nif->numRoots() < 1)
                nif->fail("Found no root nodes");

            const Nif::Record* r = nif->getRoot(0);

            const Nif::Node* nifNode = dynamic_cast<const Nif::Node*>(r);
            if (nifNode == NULL)
                nif->fail("First root was not a node, but a " + r->recName);

            osg::ref_ptr<TextKeyMapHolder> textkeys (new TextKeyMapHolder);

            osg::ref_ptr<osg::Node> created = handleNode(nifNode, NULL, imageManager, std::vector<int>(), 0, false, false, &textkeys->mTextKeys);

            if (nif->getUseSkinning())
            {
                osg::ref_ptr<SceneUtil::Skeleton> skel = new SceneUtil::Skeleton;

                osg::Group* root = created->asGroup();
                if (root && root->getDataVariance() == osg::Object::STATIC)
                {
                    skel->setStateSet(root->getStateSet());
                    skel->setName(root->getName());
                    for (unsigned int i=0; i<root->getNumChildren(); ++i)
                        skel->addChild(root->getChild(i));
                    root->removeChildren(0, root->getNumChildren());
                    created = skel;
                }
                else
                    skel->addChild(created);
                created = skel;
            }

            if (!textkeys->mTextKeys.empty())
                created->getOrCreateUserDataContainer()->addUserObject(textkeys);

            return created;
        }

        void applyNodeProperties(const Nif::Node *nifNode, osg::Node *applyTo, SceneUtil::CompositeStateSetUpdater* composite, Resource::ImageManager* imageManager, std::vector<int>& boundTextures, int animflags)
        {
            const Nif::PropertyList& props = nifNode->props;
            for (size_t i = 0; i <props.length(); ++i)
            {
                if (!props[i].empty())
                {
                    // Get the lowest numbered recIndex of the NiTexturingProperty root node.
                    // This is what is overridden when a spell effect "particle texture" is used.
                    if (nifNode->parent == NULL && !mFoundFirstRootTexturingProperty && props[i].getPtr()->recType == Nif::RC_NiTexturingProperty)
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

        osg::ref_ptr<osg::Image> handleSourceTexture(const Nif::NiSourceTexture* st, Resource::ImageManager* imageManager)
        {
            if (!st)
                return NULL;

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
                std::cerr << "Unhandled effect " << nifNode->recName << " in " << mFilename << std::endl;
                return;
            }

            const Nif::NiTextureEffect* textureEffect = static_cast<const Nif::NiTextureEffect*>(nifNode);
            if (textureEffect->textureType != Nif::NiTextureEffect::Environment_Map)
            {
                std::cerr << "Unhandled NiTextureEffect type " << textureEffect->textureType << " in " << mFilename << std::endl;
                return;
            }

            if (textureEffect->texture.empty())
            {
                std::cerr << "NiTextureEffect missing source texture in " << mFilename << std::endl;
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
                std::cerr << "Unhandled NiTextureEffect coordGenType " << textureEffect->coordGenType << " in " << mFilename << std::endl;
                return;
            }

            osg::ref_ptr<osg::Texture2D> texture2d (new osg::Texture2D(handleSourceTexture(textureEffect->texture.getPtr(), imageManager)));
            texture2d->setName("envMap");
            unsigned int clamp = static_cast<unsigned int>(textureEffect->clamp);
            int wrapT = (clamp) & 0x1;
            int wrapS = (clamp >> 1) & 0x1;
            texture2d->setWrap(osg::Texture::WRAP_S, wrapS ? osg::Texture::REPEAT : osg::Texture::CLAMP);
            texture2d->setWrap(osg::Texture::WRAP_T, wrapT ? osg::Texture::REPEAT : osg::Texture::CLAMP);

            osg::ref_ptr<osg::TexEnvCombine> texEnv = new osg::TexEnvCombine;
            texEnv->setCombine_Alpha(osg::TexEnvCombine::REPLACE);
            texEnv->setSource0_Alpha(osg::TexEnvCombine::PREVIOUS);
            texEnv->setCombine_RGB(osg::TexEnvCombine::ADD);
            texEnv->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
            texEnv->setSource1_RGB(osg::TexEnvCombine::TEXTURE);

            int texUnit = 3; // FIXME

            osg::StateSet* stateset = node->getOrCreateStateSet();
            stateset->setTextureAttributeAndModes(texUnit, texture2d, osg::StateAttribute::ON);
            stateset->setTextureAttributeAndModes(texUnit, texGen, osg::StateAttribute::ON);
            stateset->setTextureAttributeAndModes(texUnit, texEnv, osg::StateAttribute::ON);

            stateset->addUniform(new osg::Uniform("envMapColor", osg::Vec4f(1,1,1,1)));
        }

        // Get a default dataVariance for this node to be used as a hint by optimization (post)routines
        osg::ref_ptr<osg::Group> createNode(const Nif::Node* nifNode)
        {
            osg::ref_ptr<osg::Group> node;
            osg::Object::DataVariance dataVariance = osg::Object::UNSPECIFIED;

            switch (nifNode->recType)
            {
            case Nif::RC_NiTriShape:
            case Nif::RC_NiAutoNormalParticles:
            case Nif::RC_NiRotatingParticles:
                // Leaf nodes in the NIF hierarchy, so won't be able to dynamically attach children.
                // No support for keyframe controllers (just crashes in the original engine).
                if (nifNode->trafo.isIdentity())
                    node = new osg::Group;
                dataVariance = osg::Object::STATIC;
                break;
            case Nif::RC_NiBillboardNode:
                dataVariance = osg::Object::DYNAMIC;
                break;
            default:
                // The Root node can be created as a Group if no transformation is required.
                // This takes advantage of the fact root nodes can't have additional controllers
                // loaded from an external .kf file (original engine just throws "can't find node" errors if you try).
                if (!nifNode->parent && nifNode->controller.empty())
                {
                    node = new osg::Group;
                    dataVariance = osg::Object::STATIC;
                }
                else
                {
                    dataVariance = (nifNode->controller.empty() ? osg::Object::STATIC : osg::Object::DYNAMIC);
                }

                if (nifNode->isBone)
                    dataVariance = osg::Object::DYNAMIC;

                break;
            }
            if (!node)
                node = new osg::MatrixTransform(nifNode->trafo.toMatrix());

            node->setDataVariance(dataVariance);

            return node;
        }

        osg::ref_ptr<osg::Node> handleNode(const Nif::Node* nifNode, osg::Group* parentNode, Resource::ImageManager* imageManager,
                                std::vector<int> boundTextures, int animflags, bool skipMeshes, bool isAnimated, TextKeyMap* textKeys, osg::Node* rootNode=NULL)
        {
            if (rootNode != NULL && Misc::StringUtils::ciEqual(nifNode->name, "Bounding Box"))
                return NULL;

            osg::ref_ptr<osg::Group> node = createNode(nifNode);

            if (nifNode->recType == Nif::RC_NiBillboardNode)
            {
                node->addCullCallback(new BillboardCallback);
            }

            if (!nifNode->controller.empty() && nifNode->controller->recType == Nif::RC_NiKeyframeController)
                isAnimated = true;

            node->setName(nifNode->name);

            if (parentNode)
                parentNode->addChild(node);

            if (!rootNode)
                rootNode = node;

            // UserData used for a variety of features:
            // - finding the correct emitter node for a particle system
            // - establishing connections to the animated collision shapes, which are handled in a separate loader
            // - finding a random child NiNode in NiBspArrayController
            // - storing the previous 3x3 rotation and scale values for when a KeyframeController wants to
            //   change only certain elements of the 4x4 transform
            node->getOrCreateUserDataContainer()->addUserObject(
                new NodeUserData(nifNode->recIndex, nifNode->trafo.scale, nifNode->trafo.rotation));

            for (Nif::ExtraPtr e = nifNode->extra; !e.empty(); e = e->extra)
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
                        skipMeshes = true;
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
                // Leave mask for UpdateVisitor enabled
                node->setNodeMask(0x1);
            }

            if (skipMeshes && isAnimated) // make sure the empty node is not optimized away so the physicssystem can find it.
            {
                node->setDataVariance(osg::Object::DYNAMIC);
            }

            // We can skip creating meshes for hidden nodes if they don't have a VisController that
            // might make them visible later
            if (nifNode->flags & Nif::NiNode::Flag_Hidden)
            {
                bool hasVisController = false;
                for (Nif::ControllerPtr ctrl = nifNode->controller; !ctrl.empty(); ctrl = ctrl->next)
                    hasVisController = (ctrl->recType == Nif::RC_NiVisController);

                if (!hasVisController)
                    skipMeshes = true; // skip child meshes, but still create the child node hierarchy for animating collision shapes

                // now hide this node, but leave the mask for UpdateVisitor enabled so that KeyframeController works
                node->setNodeMask(0x1);
            }

            osg::ref_ptr<SceneUtil::CompositeStateSetUpdater> composite = new SceneUtil::CompositeStateSetUpdater;

            applyNodeProperties(nifNode, node, composite, imageManager, boundTextures, animflags);

            if (nifNode->recType == Nif::RC_NiTriShape && !skipMeshes)
            {
                const Nif::NiTriShape* triShape = static_cast<const Nif::NiTriShape*>(nifNode);
                if (triShape->skin.empty())
                    handleTriShape(triShape, node, composite, boundTextures, animflags);
                else
                    handleSkinnedTriShape(triShape, node, composite, boundTextures, animflags);

                if (!nifNode->controller.empty())
                    handleMeshControllers(nifNode, node, composite, boundTextures, animflags);
            }

            if(nifNode->recType == Nif::RC_NiAutoNormalParticles || nifNode->recType == Nif::RC_NiRotatingParticles)
                handleParticleSystem(nifNode, node, composite, animflags, rootNode);

            if (composite->getNumControllers() > 0)
                node->addUpdateCallback(composite);


            // Note: NiTriShapes are not allowed to have KeyframeControllers (the vanilla engine just crashes when there is one).
            // We can take advantage of this constraint for optimizations later.
            if (!nifNode->controller.empty() && node->getDataVariance() == osg::Object::DYNAMIC)
                handleNodeControllers(nifNode, static_cast<osg::MatrixTransform*>(node.get()), animflags);


            if (nifNode->recType == Nif::RC_NiLODNode)
            {
                const Nif::NiLODNode* niLodNode = static_cast<const Nif::NiLODNode*>(nifNode);
                osg::ref_ptr<osg::LOD> lod = handleLodNode(niLodNode);
                node->addChild(lod); // unsure if LOD should be above or below this node's transform
                node = lod;
            }

            const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(nifNode);
            if(ninode)
            {
                const Nif::NodeList &effects = ninode->effects;
                for (size_t i = 0; i < effects.length(); ++i)
                {
                    if (!effects[i].empty())
                        handleEffect(effects[i].getPtr(), node, imageManager);
                }

                const Nif::NodeList &children = ninode->children;
                for(size_t i = 0;i < children.length();++i)
                {
                    if(!children[i].empty())
                        handleNode(children[i].getPtr(), node, imageManager, boundTextures, animflags, skipMeshes, isAnimated, textKeys, rootNode);
                }
            }

            return node;
        }

        void handleMeshControllers(const Nif::Node *nifNode, osg::Node* node, SceneUtil::CompositeStateSetUpdater* composite, const std::vector<int> &boundTextures, int animflags)
        {
            for (Nif::ControllerPtr ctrl = nifNode->controller; !ctrl.empty(); ctrl = ctrl->next)
            {
                if (!(ctrl->flags & Nif::NiNode::ControllerFlag_Active))
                    continue;
                if (ctrl->recType == Nif::RC_NiUVController)
                {
                    const Nif::NiUVController *niuvctrl = static_cast<const Nif::NiUVController*>(ctrl.getPtr());
                    std::set<int> texUnits;
                    for (unsigned int i=0; i<boundTextures.size(); ++i)
                        texUnits.insert(i);

                    osg::ref_ptr<UVController> uvctrl = new UVController(niuvctrl->data.getPtr(), texUnits);
                    setupController(niuvctrl, uvctrl, animflags);
                    composite->addController(uvctrl);
                }
                else if (ctrl->recType == Nif::RC_NiVisController)
                {
                    handleVisController(static_cast<const Nif::NiVisController*>(ctrl.getPtr()), node, animflags);
                }
                else if(ctrl->recType == Nif::RC_NiGeomMorpherController)
                {} // handled in handleTriShape
                else
                    std::cerr << "Unhandled controller " << ctrl->recName << " on node " << nifNode->recIndex << " in " << mFilename << std::endl;
            }
        }

        void handleNodeControllers(const Nif::Node* nifNode, osg::MatrixTransform* transformNode, int animflags)
        {
            for (Nif::ControllerPtr ctrl = nifNode->controller; !ctrl.empty(); ctrl = ctrl->next)
            {
                if (!(ctrl->flags & Nif::NiNode::ControllerFlag_Active))
                    continue;
                if (ctrl->recType == Nif::RC_NiKeyframeController)
                {
                    const Nif::NiKeyframeController *key = static_cast<const Nif::NiKeyframeController*>(ctrl.getPtr());
                    if(!key->data.empty())
                    {
                        osg::ref_ptr<KeyframeController> callback(new KeyframeController(key->data.getPtr()));

                        setupController(key, callback, animflags);
                        transformNode->addUpdateCallback(callback);
                    }
                }
                else if (ctrl->recType == Nif::RC_NiVisController)
                {
                    handleVisController(static_cast<const Nif::NiVisController*>(ctrl.getPtr()), transformNode, animflags);
                }
                else
                    std::cerr << "Unhandled controller " << ctrl->recName << " on node " << nifNode->recIndex << " in " << mFilename << std::endl;
            }
        }

        void handleVisController(const Nif::NiVisController* visctrl, osg::Node* node, int animflags)
        {
            osg::ref_ptr<VisController> callback(new VisController(visctrl->data.getPtr()));
            setupController(visctrl, callback, animflags);
            node->addUpdateCallback(callback);
        }

        void handleMaterialControllers(const Nif::Property *materialProperty, SceneUtil::CompositeStateSetUpdater* composite, int animflags)
        {
            for (Nif::ControllerPtr ctrl = materialProperty->controller; !ctrl.empty(); ctrl = ctrl->next)
            {
                if (!(ctrl->flags & Nif::NiNode::ControllerFlag_Active))
                    continue;
                if (ctrl->recType == Nif::RC_NiAlphaController)
                {
                    const Nif::NiAlphaController* alphactrl = static_cast<const Nif::NiAlphaController*>(ctrl.getPtr());
                    osg::ref_ptr<AlphaController> osgctrl(new AlphaController(alphactrl->data.getPtr()));
                    setupController(alphactrl, osgctrl, animflags);
                    composite->addController(osgctrl);
                }
                else if (ctrl->recType == Nif::RC_NiMaterialColorController)
                {
                    const Nif::NiMaterialColorController* matctrl = static_cast<const Nif::NiMaterialColorController*>(ctrl.getPtr());
                    osg::ref_ptr<MaterialColorController> osgctrl(new MaterialColorController(matctrl->data.getPtr()));
                    setupController(matctrl, osgctrl, animflags);
                    composite->addController(osgctrl);
                }
                else
                    std::cerr << "Unexpected material controller " << ctrl->recType << " in " << mFilename << std::endl;
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
                    for (unsigned int i=0; i<flipctrl->mSources.length(); ++i)
                    {
                        Nif::NiSourceTexturePtr st = flipctrl->mSources[i];
                        if (st.empty())
                            continue;

                        // inherit wrap settings from the target slot
                        osg::Texture2D* inherit = dynamic_cast<osg::Texture2D*>(stateset->getTextureAttribute(flipctrl->mTexSlot, osg::StateAttribute::TEXTURE));
                        osg::Texture2D::WrapMode wrapS = osg::Texture2D::CLAMP;
                        osg::Texture2D::WrapMode wrapT = osg::Texture2D::CLAMP;
                        if (inherit)
                        {
                            wrapS = inherit->getWrap(osg::Texture2D::WRAP_S);
                            wrapT = inherit->getWrap(osg::Texture2D::WRAP_T);
                        }

                        osg::ref_ptr<osg::Texture2D> texture (new osg::Texture2D(handleSourceTexture(st.getPtr(), imageManager)));
                        texture->setWrap(osg::Texture::WRAP_S, wrapS);
                        texture->setWrap(osg::Texture::WRAP_T, wrapT);
                        textures.push_back(texture);
                    }
                    osg::ref_ptr<FlipController> callback(new FlipController(flipctrl, textures));
                    setupController(ctrl.getPtr(), callback, animflags);
                    composite->addController(callback);
                }
                else
                    std::cerr << "Unexpected texture controller " << ctrl->recName << " in " << mFilename << std::endl;
            }
        }

        void handleParticlePrograms(Nif::ExtraPtr affectors, Nif::ExtraPtr colliders, osg::Group *attachTo, osgParticle::ParticleSystem* partsys, osgParticle::ParticleProcessor::ReferenceFrame rf)
        {
            osgParticle::ModularProgram* program = new osgParticle::ModularProgram;
            attachTo->addChild(program);
            program->setParticleSystem(partsys);
            program->setReferenceFrame(rf);
            for (; !affectors.empty(); affectors = affectors->extra)
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
                    const Nif::NiColorData *clrdata = cl->data.getPtr();
                    program->addOperator(new ParticleColorAffector(clrdata));
                }
                else if (affectors->recType == Nif::RC_NiParticleRotation)
                {
                    // unused
                }
                else
                    std::cerr << "Unhandled particle modifier " << affectors->recName << " in " << mFilename << std::endl;
            }
            for (; !colliders.empty(); colliders = colliders->extra)
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
                    std::cerr << "Unhandled particle collider " << colliders->recName << " in " << mFilename << std::endl;
            }
        }

        // Load the initial state of the particle system, i.e. the initial particles and their positions, velocity and colors.
        void handleParticleInitialState(const Nif::Node* nifNode, osgParticle::ParticleSystem* partsys, const Nif::NiParticleSystemController* partctrl)
        {
            const Nif::NiAutoNormalParticlesData *particledata = NULL;
            if(nifNode->recType == Nif::RC_NiAutoNormalParticles)
                particledata = static_cast<const Nif::NiAutoNormalParticles*>(nifNode)->data.getPtr();
            else if(nifNode->recType == Nif::RC_NiRotatingParticles)
                particledata = static_cast<const Nif::NiRotatingParticles*>(nifNode)->data.getPtr();
            else
                return;

            osg::BoundingBox box;

            int i=0;
            for (std::vector<Nif::NiParticleSystemController::Particle>::const_iterator it = partctrl->particles.begin();
                 i<particledata->activeCount && it != partctrl->particles.end(); ++it, ++i)
            {
                const Nif::NiParticleSystemController::Particle& particle = *it;

                ParticleAgeSetter particletemplate(std::max(0.f, particle.lifetime));

                osgParticle::Particle* created = partsys->createParticle(&particletemplate);
                created->setLifeTime(std::max(0.f, particle.lifespan));

                // Note this position and velocity is not correct for a particle system with absolute reference frame,
                // which can not be done in this loader since we are not attached to the scene yet. Will be fixed up post-load in the SceneManager.
                created->setVelocity(particle.velocity);
                const osg::Vec3f& position = particledata->vertices.at(particle.vertex);
                created->setPosition(position);

                osg::Vec4f partcolor (1.f,1.f,1.f,1.f);
                if (particle.vertex < int(particledata->colors.size()))
                    partcolor = particledata->colors.at(particle.vertex);

                float size = particledata->sizes.at(particle.vertex) * partctrl->size;

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

        void handleParticleSystem(const Nif::Node *nifNode, osg::Group *parentNode, SceneUtil::CompositeStateSetUpdater* composite, int animflags, osg::Node* rootNode)
        {
            osg::ref_ptr<ParticleSystem> partsys (new ParticleSystem);
            partsys->setSortMode(osgParticle::ParticleSystem::SORT_BACK_TO_FRONT);

            const Nif::NiParticleSystemController* partctrl = NULL;
            for (Nif::ControllerPtr ctrl = nifNode->controller; !ctrl.empty(); ctrl = ctrl->next)
            {
                if (!(ctrl->flags & Nif::NiNode::ControllerFlag_Active))
                    continue;
                if(ctrl->recType == Nif::RC_NiParticleSystemController || ctrl->recType == Nif::RC_NiBSPArrayController)
                    partctrl = static_cast<Nif::NiParticleSystemController*>(ctrl.getPtr());
                else
                    std::cerr << "Unhandled controller " << ctrl->recName << " on node " << nifNode->recIndex << " in " << mFilename << std::endl;
            }
            if (!partctrl)
            {
                std::cerr << "No particle controller found in " << mFilename << std::endl;
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

            partsys->setQuota(partctrl->numParticles);

            partsys->getDefaultParticleTemplate().setSizeRange(osgParticle::rangef(partctrl->size, partctrl->size));
            partsys->getDefaultParticleTemplate().setColorRange(osgParticle::rangev4(osg::Vec4f(1.f,1.f,1.f,1.f), osg::Vec4f(1.f,1.f,1.f,1.f)));
            partsys->getDefaultParticleTemplate().setAlphaRange(osgParticle::rangef(1.f, 1.f));

            partsys->setFreezeOnCull(true);

            if (!partctrl->emitter.empty())
            {
                osg::ref_ptr<Emitter> emitter = handleParticleEmitter(partctrl);
                emitter->setParticleSystem(partsys);
                emitter->setReferenceFrame(osgParticle::ParticleProcessor::RELATIVE_RF);

                // Note: we assume that the Emitter node is placed *before* the Particle node in the scene graph.
                // This seems to be true for all NIF files in the game that I've checked, suggesting that NIFs work similar to OSG with regards to update order.
                // If something ever violates this assumption, the worst that could happen is the culling being one frame late, which wouldn't be a disaster.

                FindGroupByRecIndex find (partctrl->emitter->recIndex);
                rootNode->accept(find);
                if (!find.mFound)
                {
                    std::cerr << "can't find emitter node, wrong node order? in " << mFilename << std::endl;
                    return;
                }
                osg::Group* emitterNode = find.mFound;

                // Emitter attached to the emitter node. Note one side effect of the emitter using the CullVisitor is that hiding its node
                // actually causes the emitter to stop firing. Convenient, because MW behaves this way too!
                emitterNode->addChild(emitter);

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

            // affectors must be attached *after* the emitter in the scene graph for correct update order
            // attach to same node as the ParticleSystem, we need osgParticle Operators to get the correct
            // localToWorldMatrix for transforming to particle space
            handleParticlePrograms(partctrl->affectors, partctrl->colliders, parentNode, partsys.get(), rf);

            std::vector<const Nif::Property*> drawableProps;
            collectDrawableProperties(nifNode, drawableProps);
            applyDrawableProperties(parentNode, drawableProps, composite, true, animflags, true);

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
        }

        void triShapeToGeometry(const Nif::NiTriShape *triShape, osg::Geometry *geometry, osg::Node* parentNode, SceneUtil::CompositeStateSetUpdater* composite, const std::vector<int>& boundTextures, int animflags)
        {
            const Nif::NiTriShapeData* data = triShape->data.getPtr();

            {
                geometry->setVertexArray(new osg::Vec3Array(data->vertices.size(), &data->vertices[0]));
                if (!data->normals.empty())
                    geometry->setNormalArray(new osg::Vec3Array(data->normals.size(), &data->normals[0]), osg::Array::BIND_PER_VERTEX);
            }

            int textureStage = 0;
            for (std::vector<int>::const_iterator it = boundTextures.begin(); it != boundTextures.end(); ++it,++textureStage)
            {
                int uvSet = *it;
                if (uvSet >= (int)data->uvlist.size())
                {
                    std::cerr << "Warning: out of bounds UV set " << uvSet << " on TriShape \"" << triShape->name << "\" in " << mFilename << std::endl;
                    if (!data->uvlist.empty())
                        geometry->setTexCoordArray(textureStage, new osg::Vec2Array(data->uvlist[0].size(), &data->uvlist[0][0]), osg::Array::BIND_PER_VERTEX);
                    continue;
                }

                geometry->setTexCoordArray(textureStage, new osg::Vec2Array(data->uvlist[uvSet].size(), &data->uvlist[uvSet][0]), osg::Array::BIND_PER_VERTEX);
            }

            if (!data->colors.empty())
                geometry->setColorArray(new osg::Vec4Array(data->colors.size(), &data->colors[0]), osg::Array::BIND_PER_VERTEX);

            geometry->addPrimitiveSet(new osg::DrawElementsUShort(osg::PrimitiveSet::TRIANGLES,
                                                                  data->triangles.size(),
                                                                  (unsigned short*)&data->triangles[0]));

            // osg::Material properties are handled here for two reasons:
            // - if there are no vertex colors, we need to disable colorMode.
            // - there are 3 "overlapping" nif properties that all affect the osg::Material, handling them
            //   above the actual renderable would be tedious.
            std::vector<const Nif::Property*> drawableProps;
            collectDrawableProperties(triShape, drawableProps);
            applyDrawableProperties(parentNode, drawableProps, composite, !data->colors.empty(), animflags, false);
        }

        void handleTriShape(const Nif::NiTriShape* triShape, osg::Group* parentNode, SceneUtil::CompositeStateSetUpdater* composite, const std::vector<int>& boundTextures, int animflags)
        {
            osg::ref_ptr<osg::Geometry> geometry;
            for (Nif::ControllerPtr ctrl = triShape->controller; !ctrl.empty(); ctrl = ctrl->next)
            {
                if (!(ctrl->flags & Nif::NiNode::ControllerFlag_Active))
                    continue;
                if(ctrl->recType == Nif::RC_NiGeomMorpherController)
                {
                    geometry = handleMorphGeometry(static_cast<const Nif::NiGeomMorpherController*>(ctrl.getPtr()), triShape, parentNode, composite, boundTextures, animflags);

                    osg::ref_ptr<GeomMorpherController> morphctrl = new GeomMorpherController(
                                static_cast<const Nif::NiGeomMorpherController*>(ctrl.getPtr())->data.getPtr());
                    setupController(ctrl.getPtr(), morphctrl, animflags);
                    geometry->setUpdateCallback(morphctrl);
                    break;
                }
            }

            if (!geometry.get())
            {
                geometry = new osg::Geometry;
                triShapeToGeometry(triShape, geometry, parentNode, composite, boundTextures, animflags);
            }

            if (geometry->getDataVariance() == osg::Object::DYNAMIC)
            {
                // Add a copy, we will alternate between the two copies every other frame using the FrameSwitch
                // This is so we can set the DataVariance as STATIC, giving a huge performance boost
                geometry->setDataVariance(osg::Object::STATIC);
                osg::ref_ptr<FrameSwitch> frameswitch = new FrameSwitch;

                osg::ref_ptr<osg::Geometry> geom2 = osg::clone(geometry.get(), osg::CopyOp::DEEP_COPY_NODES|osg::CopyOp::DEEP_COPY_DRAWABLES);
                frameswitch->addChild(geometry);
                frameswitch->addChild(geom2);

                parentNode->addChild(frameswitch);
            }
            else
                parentNode->addChild(geometry);
        }

        osg::ref_ptr<osg::Geometry> handleMorphGeometry(const Nif::NiGeomMorpherController* morpher, const Nif::NiTriShape *triShape, osg::Node* parentNode, SceneUtil::CompositeStateSetUpdater* composite, const std::vector<int>& boundTextures, int animflags)
        {
            osg::ref_ptr<osgAnimation::MorphGeometry> morphGeom = new osgAnimation::MorphGeometry;
            morphGeom->setMethod(osgAnimation::MorphGeometry::RELATIVE);
            // No normals available in the MorphData
            morphGeom->setMorphNormals(false);

            morphGeom->setUpdateCallback(NULL);
            morphGeom->setCullCallback(new UpdateMorphGeometry);
            morphGeom->setUseVertexBufferObjects(true);

            triShapeToGeometry(triShape, morphGeom, parentNode, composite, boundTextures, animflags);

            morphGeom->getOrCreateVertexBufferObject()->setUsage(GL_DYNAMIC_DRAW_ARB);

            const std::vector<Nif::NiMorphData::MorphData>& morphs = morpher->data.getPtr()->mMorphs;
            if (morphs.empty())
                return morphGeom;
            // Note we are not interested in morph 0, which just contains the original vertices
            for (unsigned int i = 1; i < morphs.size(); ++i)
            {
                osg::ref_ptr<osg::Geometry> morphTarget = new osg::Geometry;
                morphTarget->setVertexArray(new osg::Vec3Array(morphs[i].mVertices.size(), &morphs[i].mVertices[0]));
                morphGeom->addMorphTarget(morphTarget, 0.f);
            }

            // build the bounding box containing all possible morph combinations

            std::vector<osg::BoundingBox> vertBounds(morphs[0].mVertices.size());

            // Since we don't know what combinations of morphs are being applied we need to keep track of a bounding box for each vertex.
            // The minimum/maximum of the box is the minimum/maximum offset the vertex can have from its starting position.

            // Start with zero offsets which will happen when no morphs are applied.
            for (unsigned int i=0; i<vertBounds.size(); ++i)
                vertBounds[i].set(osg::Vec3f(0,0,0), osg::Vec3f(0,0,0));

            for (unsigned int i = 1; i < morphs.size(); ++i)
            {
                for (unsigned int j=0; j<morphs[i].mVertices.size() && vertBounds.size(); ++j)
                {
                    osg::BoundingBox& bounds = vertBounds[j];
                    bounds.expandBy(bounds._max + morphs[i].mVertices[j]);
                    bounds.expandBy(bounds._min + morphs[i].mVertices[j]);
                }
            }

            osg::BoundingBox box;
            for (unsigned int i=0; i<vertBounds.size(); ++i)
            {
                vertBounds[i]._max += morphs[0].mVertices[i];
                vertBounds[i]._min += morphs[0].mVertices[i];
                box.expandBy(vertBounds[i]);
            }

            // For the initial bounding box (used for object placement) use the default pose, fire off a bounding compute to set this initial box
            morphGeom->getBound();

            // Now set up the callback so that we get properly enlarged bounds if/when the mesh starts animating
            morphGeom->setComputeBoundingBoxCallback(new StaticBoundingBoxCallback(box));

            return morphGeom;
        }

        void handleSkinnedTriShape(const Nif::NiTriShape *triShape, osg::Group *parentNode, SceneUtil::CompositeStateSetUpdater* composite,
                                          const std::vector<int>& boundTextures, int animflags)
        {
            osg::ref_ptr<osg::Geometry> geometry (new osg::Geometry);
            triShapeToGeometry(triShape, geometry, parentNode, composite, boundTextures, animflags);

            osg::ref_ptr<SceneUtil::RigGeometry> rig(new SceneUtil::RigGeometry);
            rig->setSourceGeometry(geometry);

            const Nif::NiSkinInstance *skin = triShape->skin.getPtr();

            // Assign bone weights
            osg::ref_ptr<SceneUtil::RigGeometry::InfluenceMap> map (new SceneUtil::RigGeometry::InfluenceMap);

            const Nif::NiSkinData *data = skin->data.getPtr();
            const Nif::NodeList &bones = skin->bones;
            for(size_t i = 0;i < bones.length();i++)
            {
                std::string boneName = bones[i].getPtr()->name;

                SceneUtil::RigGeometry::BoneInfluence influence;
                const std::vector<Nif::NiSkinData::VertWeight> &weights = data->bones[i].weights;
                //influence.mWeights.reserve(weights.size());
                for(size_t j = 0;j < weights.size();j++)
                {
                    std::pair<unsigned short, float> indexWeight = std::make_pair(weights[j].vertex, weights[j].weight);
                    influence.mWeights.insert(indexWeight);
                }
                influence.mInvBindMatrix = data->bones[i].trafo.toMatrix();
                influence.mBoundSphere = osg::BoundingSpheref(data->bones[i].boundSphereCenter, data->bones[i].boundSphereRadius);

                map->mMap.insert(std::make_pair(boneName, influence));
            }
            rig->setInfluenceMap(map);

            // Add a copy, we will alternate between the two copies every other frame using the FrameSwitch
            // This is so we can set the DataVariance as STATIC, giving a huge performance boost
            rig->setDataVariance(osg::Object::STATIC);

            osg::ref_ptr<FrameSwitch> frameswitch = new FrameSwitch;

            SceneUtil::RigGeometry* rig2 = osg::clone(rig.get(), osg::CopyOp::DEEP_COPY_NODES|osg::CopyOp::DEEP_COPY_DRAWABLES);
            frameswitch->addChild(rig);
            frameswitch->addChild(rig2);

            parentNode->addChild(frameswitch);
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
                std::cerr<< "Unexpected blend mode: "<< mode << " in " << mFilename << std::endl;
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
                std::cerr << "Unexpected blend mode: " << mode << " in " << mFilename << std::endl;
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
                std::cerr << "Unexpected stencil function: " << func << " in " << mFilename << std::endl;
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
                std::cerr << "Unexpected stencil operation: " << op << " in " << mFilename << std::endl;
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
                pixelformat = GL_RGB;
                break;
            case Nif::NiPixelData::NIPXFMT_RGBA8:
                pixelformat = GL_RGBA;
                break;
            default:
                std::cerr << "Unhandled internal pixel format " << pixelData->fmt << " in " << mFilename << std::endl;
                return NULL;
            }

            if (pixelData->mipmaps.empty())
                return NULL;

            unsigned int width = 0;
            unsigned int height = 0;

            std::vector<unsigned int> mipmapVector;
            for (unsigned int i=0; i<pixelData->mipmaps.size()-3; ++i)
            {
                const Nif::NiPixelData::Mipmap& mip = pixelData->mipmaps[i];

                size_t mipSize = mip.height * mip.width * pixelData->bpp / 8;
                if (mipSize + mip.dataOffset > pixelData->data.size())
                {
                    std::cerr << "Warning: Internal texture's mipmap data out of bounds, ignoring texture" << std::endl;
                    return NULL;
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
                std::cerr << "Warning: Internal Texture Width and height must be non zero, ignoring texture" << std::endl;
                return NULL;
            }

            unsigned char* data = new unsigned char[pixelData->data.size()];
            memcpy(data, &pixelData->data[0], pixelData->data.size());

            image->setImage(width, height, 1, pixelformat, pixelformat, GL_UNSIGNED_BYTE, data, osg::Image::USE_NEW_DELETE);
            image->setMipmapLevels(mipmapVector);
            image->flipVertical();

            return image;
        }

        void handleTextureProperty(const Nif::NiTexturingProperty* texprop, osg::StateSet* stateset, SceneUtil::CompositeStateSetUpdater* composite, Resource::ImageManager* imageManager, std::vector<int>& boundTextures, int animflags)
        {
            if (!boundTextures.empty())
            {
                // overriding a parent NiTexturingProperty, so remove what was previously bound
                for (unsigned int i=0; i<boundTextures.size(); ++i)
                    stateset->setTextureMode(i, GL_TEXTURE_2D, osg::StateAttribute::OFF);
                boundTextures.clear();
            }

            for (int i=0; i<Nif::NiTexturingProperty::NumTextures; ++i)
            {
                if (texprop->textures[i].inUse)
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
                            //std::cerr << "NiTexturingProperty::GlossTexture in " << mFilename << " not currently used." << std::endl;
                            continue;
                        }
                        default:
                        {
                            std::cerr << "Warning: unhandled texture stage " << i << " in " << mFilename << std::endl;
                            continue;
                        }
                    }

                    const Nif::NiTexturingProperty::Texture& tex = texprop->textures[i];
                    if(tex.texture.empty() && texprop->controller.empty())
                    {
                        std::cerr << "Warning: texture layer " << i << " is in use but empty in " << mFilename << std::endl;
                        continue;
                    }

                    // create a new texture, will later attempt to share using the SharedStateManager
                    osg::ref_ptr<osg::Texture2D> texture2d;
                    if (!tex.texture.empty())
                    {
                        const Nif::NiSourceTexture *st = tex.texture.getPtr();
                        osg::ref_ptr<osg::Image> image = handleSourceTexture(st, imageManager);
                        texture2d = new osg::Texture2D(image);
                    }
                    else
                        texture2d = new osg::Texture2D;

                    unsigned int clamp = static_cast<unsigned int>(tex.clamp);
                    int wrapT = (clamp) & 0x1;
                    int wrapS = (clamp >> 1) & 0x1;

                    texture2d->setWrap(osg::Texture::WRAP_S, wrapS ? osg::Texture::REPEAT : osg::Texture::CLAMP);
                    texture2d->setWrap(osg::Texture::WRAP_T, wrapT ? osg::Texture::REPEAT : osg::Texture::CLAMP);

                    int texUnit = boundTextures.size();

                    stateset->setTextureAttributeAndModes(texUnit, texture2d, osg::StateAttribute::ON);

                    if (i == Nif::NiTexturingProperty::GlowTexture)
                    {
                        osg::TexEnvCombine* texEnv = new osg::TexEnvCombine;
                        texEnv->setCombine_Alpha(osg::TexEnvCombine::REPLACE);
                        texEnv->setSource0_Alpha(osg::TexEnvCombine::PREVIOUS);
                        texEnv->setCombine_RGB(osg::TexEnvCombine::ADD);
                        texEnv->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
                        texEnv->setSource1_RGB(osg::TexEnvCombine::TEXTURE);

                        stateset->setTextureAttributeAndModes(texUnit, texEnv, osg::StateAttribute::ON);
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
                        texture2d->setName("normalMap");
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

                    boundTextures.push_back(tex.uvSet);
                }
            }
            handleTextureControllers(texprop, composite, imageManager, stateset, animflags);
        }

        void handleProperty(const Nif::Property *property,
                            osg::Node *node, SceneUtil::CompositeStateSetUpdater* composite, Resource::ImageManager* imageManager, std::vector<int>& boundTextures, int animflags)
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
                // VER_MW doesn't support a DepthFunction according to NifSkope
                osg::ref_ptr<osg::Depth> depth = new osg::Depth;
                depth->setWriteMask((zprop->flags>>1)&1);
                depth = shareAttribute(depth);
                node->getOrCreateStateSet()->setAttributeAndModes(depth, osg::StateAttribute::ON);
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
                handleTextureProperty(texprop, stateset, composite, imageManager, boundTextures, animflags);
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
                std::cerr << "Unhandled " << property->recName << " in " << mFilename << std::endl;
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
            static OpenThreads::Mutex sMutex;
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(sMutex);
            typename Cache::iterator found = sCache.find(attr);
            if (found == sCache.end())
                found = sCache.insert(attr).first;
            return *found;
        }

        void applyDrawableProperties(osg::Node* node, const std::vector<const Nif::Property*>& properties, SceneUtil::CompositeStateSetUpdater* composite,
                                             bool hasVertexColors, int animflags, bool particleMaterial)
        {
            osg::StateSet* stateset = node->getOrCreateStateSet();

            int specFlags = 0; // Specular is disabled by default, even if there's a specular color in the NiMaterialProperty
            osg::ref_ptr<osg::Material> mat (new osg::Material);
            mat->setColorMode(hasVertexColors ? osg::Material::AMBIENT_AND_DIFFUSE : osg::Material::OFF);

            // NIF material defaults don't match OpenGL defaults
            mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,1));
            mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(1,1,1,1));

            bool hasMatCtrl = false;

            for (std::vector<const Nif::Property*>::const_reverse_iterator it = properties.rbegin(); it != properties.rend(); ++it)
            {
                const Nif::Property* property = *it;
                switch (property->recType)
                {
                case Nif::RC_NiSpecularProperty:
                {
                    specFlags = property->flags;
                    break;
                }
                case Nif::RC_NiMaterialProperty:
                {
                    const Nif::NiMaterialProperty* matprop = static_cast<const Nif::NiMaterialProperty*>(property);

                    mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(matprop->data.diffuse, matprop->data.alpha));
                    mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4f(matprop->data.ambient, 1.f));
                    mat->setEmission(osg::Material::FRONT_AND_BACK, osg::Vec4f(matprop->data.emissive, 1.f));

                    mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(matprop->data.specular, 1.f));
                    mat->setShininess(osg::Material::FRONT_AND_BACK, matprop->data.glossiness);

                    if (!matprop->controller.empty())
                    {
                        hasMatCtrl = true;
                        handleMaterialControllers(matprop, composite, animflags);
                    }

                    break;
                }
                case Nif::RC_NiVertexColorProperty:
                {
                    const Nif::NiVertexColorProperty* vertprop = static_cast<const Nif::NiVertexColorProperty*>(property);
                    if (!hasVertexColors)
                        break;
                    switch (vertprop->flags)
                    {
                    case 0:
                        mat->setColorMode(osg::Material::OFF);
                        break;
                    case 1:
                        mat->setColorMode(osg::Material::EMISSION);
                        break;
                    case 2:
                        mat->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
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

            if (specFlags == 0)
                mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f,0.f,0.f,0.f));

            // Particles don't have normals, so can't be diffuse lit.
            if (particleMaterial)
            {
                // NB ignoring diffuse.a()
                mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4f(0,0,0,1));
                mat->setColorMode(osg::Material::AMBIENT);
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
        }

    };

    osg::ref_ptr<osg::Node> Loader::load(Nif::NIFFilePtr file, Resource::ImageManager* imageManager)
    {
        LoaderImpl impl(file->getFilename());
        return impl.load(file, imageManager);
    }

    void Loader::loadKf(Nif::NIFFilePtr kf, KeyframeHolder& target)
    {
        LoaderImpl impl(kf->getFilename());
        impl.loadKf(kf, target);
    }

}
