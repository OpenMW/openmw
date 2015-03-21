#include "nifloader.hpp"

#include <osg/Matrixf>
#include <osg/MatrixTransform>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Array>

// resource
#include <components/bsa/bsa_file.hpp>
#include <osgDB/Registry>
#include <osg/io_utils>
#include <components/misc/stringops.hpp>
#include <components/misc/resourcehelpers.hpp>

// skel
#include <osgAnimation/Skeleton>
#include <osgAnimation/Bone>
#include <osgAnimation/RigGeometry>
#include <osgAnimation/MorphGeometry>

// particle
#include <osgParticle/ParticleSystem>
#include <osgParticle/ParticleSystemUpdater>
#include <osgParticle/ConstantRateCounter>
#include <osgParticle/ModularEmitter>
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

#include "particle.hpp"

namespace
{
    osg::Matrixf toMatrix(const Nif::Transformation& nifTrafo)
    {
        osg::Matrixf transform;
        transform.setTrans(nifTrafo.pos);

        for (int i=0;i<3;++i)
            for (int j=0;j<3;++j)
                transform(j,i) = nifTrafo.rotation.mValues[i][j] * nifTrafo.scale; // NB column/row major difference

        return transform;
    }

    osg::Matrixf getWorldTransform(const Nif::Node* node)
    {
        if(node->parent != NULL)
            return toMatrix(node->trafo) * getWorldTransform(node->parent);
        return toMatrix(node->trafo);
    }

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
            std::cerr<< "Unexpected blend mode: "<< mode << std::endl;
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
            std::cerr << "Unexpected blend mode: " << mode << std::endl;
            return osg::AlphaFunc::LEQUAL;
        }
    }

    // Collect all properties affecting the given node that should be applied to an osg::Material.
    void collectMaterialProperties(const Nif::Node* nifNode, std::vector<const Nif::Property*>& out)
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
                    out.push_back(props[i].getPtr());
                    break;
                default:
                    break;
                }
            }
        }
        if (nifNode->parent)
            collectMaterialProperties(nifNode->parent, out);
    }

    // NodeCallback used to update the bone matrices in skeleton space as needed for skinning.
    // Must be set on a Bone.
    class UpdateBone : public osg::NodeCallback
    {
    public:
        UpdateBone() {}
        UpdateBone(const UpdateBone& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY)
            : osg::Object(copy, copyop), osg::NodeCallback(copy, copyop)
        {
        }

        META_Object(NifOsg, UpdateBone)

        // Callback method called by the NodeVisitor when visiting a node.
        void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            if (nv && nv->getVisitorType() == osg::NodeVisitor::UPDATE_VISITOR)
            {
                osgAnimation::Bone* b = dynamic_cast<osgAnimation::Bone*>(node);
                if (!b)
                {
                    OSG_WARN << "Warning: UpdateBone set on non-Bone object." << std::endl;
                    return;
                }

                osgAnimation::Bone* parent = b->getBoneParent();
                if (parent)
                    b->setMatrixInSkeletonSpace(b->getMatrixInBoneSpace() * parent->getMatrixInSkeletonSpace());
                else
                    b->setMatrixInSkeletonSpace(b->getMatrixInBoneSpace());
            }
            traverse(node,nv);
        }
    };

    // Custom node used to have a transform always oriented towards the camera. Can have translation and scale
    // set just like a regular MatrixTransform, but the rotation set will be overridden in order to face the camera.
    class BillboardNode : public osg::MatrixTransform
    {
    public:
        BillboardNode() : osg::MatrixTransform() {}
        BillboardNode(const BillboardNode& copy, const osg::CopyOp& copyop)
            : osg::MatrixTransform(copy, copyop) {}
        BillboardNode(const osg::Matrix& matrix)
            : osg::MatrixTransform(matrix) {}

        META_Node(NifOsg, BillboardNode)

        virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor*) const
        {
            if (_referenceFrame==RELATIVE_RF)
            {
                matrix.preMult(_matrix);
                osg::Vec3 scale = matrix.getScale();
                matrix.setRotate(osg::Quat());
                matrix(0,0) = scale.x();
                matrix(1,1) = scale.y();
                matrix(2,2) = scale.z();
            }
            else // absolute
            {
                matrix = _matrix;
            }
            return true;
        }
    };

    // NodeCallback used to set the inverse of the parent bone's matrix in skeleton space
    // on the MatrixTransform that the NodeCallback is attached to. This is used so we can
    // attach skinned meshes to their actual parent node, while still having the skinning
    // work in skeleton space as expected.
    // Must be set on a MatrixTransform.
    class InvertBoneMatrix : public osg::NodeCallback
    {
    public:
        InvertBoneMatrix() {}

        InvertBoneMatrix(const InvertBoneMatrix& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY)
            : osg::Object(copy, copyop), osg::NodeCallback(copy, copyop) {}

        META_Object(NifOsg, InvertBoneMatrix)

        void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            if (nv && nv->getVisitorType() == osg::NodeVisitor::UPDATE_VISITOR)
            {
                osg::NodePath path = nv->getNodePath();
                path.pop_back();

                osg::MatrixTransform* trans = dynamic_cast<osg::MatrixTransform*>(node);

                for (osg::NodePath::iterator it = path.begin(); it != path.end(); ++it)
                {
                    if (dynamic_cast<osgAnimation::Skeleton*>(*it))
                    {
                        path.erase(path.begin(), it+1);
                        // the bone's transform in skeleton space
                        osg::Matrix boneMat = osg::computeLocalToWorld( path );
                        trans->setMatrix(osg::Matrix::inverse(boneMat));
                        break;
                    }
                }
            }
            traverse(node,nv);
        }
    };

    osg::ref_ptr<osg::Geometry> handleMorphGeometry(const Nif::NiGeomMorpherController* morpher)
    {
        osg::ref_ptr<osgAnimation::MorphGeometry> morphGeom = new osgAnimation::MorphGeometry;
        morphGeom->setMethod(osgAnimation::MorphGeometry::RELATIVE);
        // No normals available in the MorphData
        morphGeom->setMorphNormals(false);

        const std::vector<Nif::NiMorphData::MorphData>& morphs = morpher->data.getPtr()->mMorphs;
        // Note we are not interested in morph 0, which just contains the original vertices
        for (unsigned int i = 1; i < morphs.size(); ++i)
        {
            osg::ref_ptr<osg::Geometry> morphTarget = new osg::Geometry;
            morphTarget->setVertexArray(new osg::Vec3Array(morphs[i].mVertices.size(), &morphs[i].mVertices[0]));
            morphGeom->addMorphTarget(morphTarget, 0.f);
        }
        return morphGeom;
    }
}

namespace NifOsg
{

    void Loader::load(Nif::NIFFilePtr nif, osg::Group *parentNode)
    {
        mNif = nif;

        if (nif->numRoots() < 1)
        {
            nif->warn("Found no root nodes");
            return;
        }

        const Nif::Record* r = nif->getRoot(0);
        assert(r != NULL);

        const Nif::Node* nifNode = dynamic_cast<const Nif::Node*>(r);
        if (nifNode == NULL)
        {
            nif->warn("First root was not a node, but a " + r->recName);
            return;
        }

        mRootNode = parentNode;
        handleNode(nifNode, parentNode, false, std::map<int, int>(), 0, 0);
    }

    void Loader::loadAsSkeleton(Nif::NIFFilePtr nif, osg::Group *parentNode)
    {
        mNif = nif;

        if (nif->numRoots() < 1)
        {
            nif->warn("Found no root nodes");
            return;
        }

        const Nif::Record* r = nif->getRoot(0);
        assert(r != NULL);

        const Nif::Node* nifNode = dynamic_cast<const Nif::Node*>(r);
        if (nifNode == NULL)
        {
            nif->warn("First root was not a node, but a " + r->recName);
            return;
        }

        mRootNode = parentNode;

        osgAnimation::Skeleton* skel = new osgAnimation::Skeleton;
        mSkeleton = skel;
        mRootNode->addChild(mSkeleton);

        handleNode(nifNode, mSkeleton, true, std::map<int, int>(), 0, 0);
    }

    void Loader::applyNodeProperties(const Nif::Node *nifNode, osg::Node *applyTo, std::map<int, int>& boundTextures, int animflags)
    {
        const Nif::PropertyList& props = nifNode->props;
        for (size_t i = 0; i <props.length();++i)
        {
            if (!props[i].empty())
                handleProperty(props[i].getPtr(), nifNode, applyTo, boundTextures, animflags);
        }
    }

    void Loader::setupController(const Nif::Controller* ctrl, Controller* toSetup, int animflags)
    {
        //bool autoPlay = animflags & Nif::NiNode::AnimFlag_AutoPlay;
        //if (autoPlay)
            toSetup->mSource = boost::shared_ptr<ControllerSource>(new FrameTimeSource);

        toSetup->mFunction = boost::shared_ptr<ControllerFunction>(new ControllerFunction(ctrl, 1 /*autoPlay*/));
    }

    void Loader::handleNode(const Nif::Node* nifNode, osg::Group* parentNode, bool createSkeleton,
                            std::map<int, int> boundTextures, int animflags, int particleflags, bool collisionNode)
    {
        osg::ref_ptr<osg::MatrixTransform> transformNode;
        if (nifNode->recType == Nif::RC_NiBillboardNode)
        {
            transformNode = new BillboardNode(toMatrix(nifNode->trafo));
        }
        else if (createSkeleton)
        {
            osgAnimation::Bone* bone = new osgAnimation::Bone;
            transformNode = bone;
            bone->setMatrix(toMatrix(nifNode->trafo));
            bone->setName(nifNode->name);
            bone->setInvBindMatrixInSkeletonSpace(osg::Matrixf::inverse(getWorldTransform(nifNode)));
        }
        else
        {
            transformNode = new osg::MatrixTransform(toMatrix(nifNode->trafo));
        }

        // UserData used for a variety of features:
        // - finding the correct emitter node for a particle system
        // - establishing connections to the animated collision shapes, which are handled in a separate loader
        // - finding a random child NiNode in NiBspArrayController
        transformNode->setUserData(new RecIndexHolder(nifNode->recIndex));

        if (nifNode->recType == Nif::RC_NiBSAnimationNode)
            animflags |= nifNode->flags;
        if (nifNode->recType == Nif::RC_NiBSParticleNode)
            particleflags |= nifNode->flags;

        // Hide collision shapes, but don't skip the subgraph
        // We still need to animate the hidden bones so the physics system can access them
        if (nifNode->recType == Nif::RC_RootCollisionNode)
        {
            collisionNode = true;
            // Leave mask for UpdateVisitor enabled
            transformNode->setNodeMask(0x1);
        }

        // We could probably skip hidden nodes entirely if they don't have a VisController that
        // might make them visible later
        if (nifNode->flags & Nif::NiNode::Flag_Hidden)
            transformNode->setNodeMask(0x1); // Leave mask for UpdateVisitor enabled

        // Insert bones at position 0 to prevent update order problems (see comment in osg Skeleton.cpp)
        parentNode->insertChild(0, transformNode);

        applyNodeProperties(nifNode, transformNode, boundTextures, animflags);

        if (nifNode->recType == Nif::RC_NiTriShape && !collisionNode)
        {
            const Nif::NiTriShape* triShape = static_cast<const Nif::NiTriShape*>(nifNode);
            if (!createSkeleton || triShape->skin.empty())
                handleTriShape(triShape, transformNode, boundTextures, animflags);
            else
                handleSkinnedTriShape(triShape, transformNode, boundTextures, animflags);

            if (!nifNode->controller.empty())
                handleMeshControllers(nifNode, transformNode, boundTextures, animflags);
        }

        if(nifNode->recType == Nif::RC_NiAutoNormalParticles || nifNode->recType == Nif::RC_NiRotatingParticles)
            handleParticleSystem(nifNode, transformNode, animflags, particleflags);

        if (!nifNode->controller.empty())
            handleNodeControllers(nifNode, transformNode, animflags);

        // Added last so the changes from KeyframeControllers are taken into account
        if (osgAnimation::Bone* bone = dynamic_cast<osgAnimation::Bone*>(transformNode.get()))
            bone->addUpdateCallback(new UpdateBone);

        const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(nifNode);
        if(ninode)
        {
            const Nif::NodeList &children = ninode->children;
            for(size_t i = 0;i < children.length();++i)
            {
                if(!children[i].empty())
                    handleNode(children[i].getPtr(), transformNode, createSkeleton, boundTextures, animflags, particleflags, collisionNode);
            }
        }
    }

    void Loader::handleMeshControllers(const Nif::Node *nifNode, osg::MatrixTransform *transformNode, const std::map<int, int> &boundTextures, int animflags)
    {
        for (Nif::ControllerPtr ctrl = nifNode->controller; !ctrl.empty(); ctrl = ctrl->next)
        {
            if (!(ctrl->flags & Nif::NiNode::ControllerFlag_Active))
                continue;
            if (ctrl->recType == Nif::RC_NiUVController)
            {
                const Nif::NiUVController *uvctrl = static_cast<const Nif::NiUVController*>(ctrl.getPtr());
                std::set<int> texUnits;
                for (std::map<int, int>::const_iterator it = boundTextures.begin(); it != boundTextures.end(); ++it)
                    texUnits.insert(it->first);

                osg::ref_ptr<UVController> ctrl = new UVController(uvctrl->data.getPtr(), texUnits);
                setupController(uvctrl, ctrl, animflags);
                transformNode->getOrCreateStateSet()->setDataVariance(osg::StateSet::DYNAMIC);

                transformNode->addUpdateCallback(ctrl);
            }
        }
    }

    void Loader::handleNodeControllers(const Nif::Node* nifNode, osg::MatrixTransform* transformNode, int animflags)
    {
        bool seenKeyframeCtrl = false;
        for (Nif::ControllerPtr ctrl = nifNode->controller; !ctrl.empty(); ctrl = ctrl->next)
        {
            if (!(ctrl->flags & Nif::NiNode::ControllerFlag_Active))
                continue;
            if (ctrl->recType == Nif::RC_NiKeyframeController)
            {
                const Nif::NiKeyframeController *key = static_cast<const Nif::NiKeyframeController*>(ctrl.getPtr());
                if(!key->data.empty())
                {
                    if (seenKeyframeCtrl)
                    {
                        std::cerr << "Warning: multiple KeyframeControllers on the same node" << std::endl;
                        continue;
                    }

                    // build the rotation part manually to avoid issues caused by scaling
                    osg::Matrixf mat;
                    for (int i=0;i<3;++i)
                        for (int j=0;j<3;++j)
                            mat(j,i) = nifNode->trafo.rotation.mValues[i][j];

                    osg::ref_ptr<KeyframeController> callback(new KeyframeController(mNif, key->data.getPtr(), mat.getRotate(), nifNode->trafo.scale));

                    setupController(key, callback, animflags);
                    transformNode->addUpdateCallback(callback);

                    seenKeyframeCtrl = true;
                }
            }
            else if (ctrl->recType == Nif::RC_NiVisController)
            {
                const Nif::NiVisController* visctrl = static_cast<const Nif::NiVisController*>(ctrl.getPtr());

                osg::ref_ptr<VisController> callback(new VisController(visctrl->data.getPtr()));
                setupController(visctrl, callback, animflags);
                transformNode->addUpdateCallback(callback);
            }
        }
    }

    void Loader::handleMaterialControllers(const Nif::Property *materialProperty, osg::Node* node, osg::StateSet *stateset, int animflags)
    {
        for (Nif::ControllerPtr ctrl = materialProperty->controller; !ctrl.empty(); ctrl = ctrl->next)
        {
            if (!(ctrl->flags & Nif::NiNode::ControllerFlag_Active))
                continue;
            if (ctrl->recType == Nif::RC_NiAlphaController)
            {
                const Nif::NiAlphaController* alphactrl = static_cast<const Nif::NiAlphaController*>(ctrl.getPtr());
                osg::ref_ptr<AlphaController> ctrl(new AlphaController(alphactrl->data.getPtr()));
                setupController(alphactrl, ctrl, animflags);
                stateset->setDataVariance(osg::StateSet::DYNAMIC);
                node->addUpdateCallback(ctrl);
            }
            else if (ctrl->recType == Nif::RC_NiMaterialColorController)
            {
                const Nif::NiMaterialColorController* matctrl = static_cast<const Nif::NiMaterialColorController*>(ctrl.getPtr());
                osg::ref_ptr<MaterialColorController> ctrl(new MaterialColorController(matctrl->data.getPtr()));
                setupController(matctrl, ctrl, animflags);
                stateset->setDataVariance(osg::StateSet::DYNAMIC);
                node->addUpdateCallback(ctrl);
            }
            else
                std::cerr << "Unexpected material controller " << ctrl->recType << std::endl;
        }
    }

    void Loader::handleTextureControllers(const Nif::Property *texProperty, osg::Node* node, osg::StateSet *stateset, int animflags)
    {
        for (Nif::ControllerPtr ctrl = texProperty->controller; !ctrl.empty(); ctrl = ctrl->next)
        {
            if (!(ctrl->flags & Nif::NiNode::ControllerFlag_Active))
                continue;
            if (ctrl->recType == Nif::RC_NiFlipController)
            {
                const Nif::NiFlipController* flipctrl = static_cast<const Nif::NiFlipController*>(ctrl.getPtr());
                std::vector<osg::ref_ptr<osg::Image> > textures;
                for (unsigned int i=0; i<flipctrl->mSources.length(); ++i)
                {
                    Nif::NiSourceTexturePtr st = flipctrl->mSources[i];
                    if (st.empty())
                        continue;

                    std::string filename = Misc::ResourceHelpers::correctTexturePath(st->filename, resourceManager);

                    // tx_creature_werewolf.dds isn't loading in the correct format without this option
                    osgDB::Options* opts = new osgDB::Options;
                    opts->setOptionString("dds_dxt1_detect_rgba");
                    osgDB::ReaderWriter* reader = osgDB::Registry::instance()->getReaderWriterForExtension("dds");
                    osgDB::ReaderWriter::ReadResult result = reader->readImage(*resourceManager->get(filename.c_str()), opts);
                    textures.push_back(osg::ref_ptr<osg::Image>(result.getImage()));
                }
                osg::ref_ptr<FlipController> callback(new FlipController(flipctrl, textures));
                setupController(ctrl.getPtr(), callback, animflags);
                stateset->setDataVariance(osg::StateSet::DYNAMIC);
                node->addUpdateCallback(callback);
            }
            else
                std::cerr << "Unexpected texture controller " << ctrl->recName << std::endl;
        }
    }

    void Loader::handleParticleSystem(const Nif::Node *nifNode, osg::Group *parentNode, int animflags, int particleflags)
    {
        osg::ref_ptr<ParticleSystem> partsys (new ParticleSystem);
        partsys->setSortMode(osgParticle::ParticleSystem::SORT_BACK_TO_FRONT);

        const Nif::NiAutoNormalParticlesData *particledata = NULL;
        if(nifNode->recType == Nif::RC_NiAutoNormalParticles)
            particledata = static_cast<const Nif::NiAutoNormalParticles*>(nifNode)->data.getPtr();
        else if(nifNode->recType == Nif::RC_NiRotatingParticles)
            particledata = static_cast<const Nif::NiRotatingParticles*>(nifNode)->data.getPtr();
        else
            return;

        const Nif::NiParticleSystemController* partctrl = NULL;
        for (Nif::ControllerPtr ctrl = nifNode->controller; !ctrl.empty(); ctrl = ctrl->next)
        {
            if (!(ctrl->flags & Nif::NiNode::ControllerFlag_Active))
                continue;
            if(ctrl->recType == Nif::RC_NiParticleSystemController || ctrl->recType == Nif::RC_NiBSPArrayController)
                partctrl = static_cast<Nif::NiParticleSystemController*>(ctrl.getPtr());
        }
        if (!partctrl)
        {
            std::cerr << "No particle controller found " << std::endl;
            return;
        }

        std::vector<int> targets;
        if (partctrl->recType == Nif::RC_NiBSPArrayController)
        {
            getAllNiNodes(partctrl->emitter.getPtr(), targets);
        }

        osgParticle::ParticleProcessor::ReferenceFrame rf = (particleflags & Nif::NiNode::ParticleFlag_LocalSpace)
                ? osgParticle::ParticleProcessor::RELATIVE_RF
                : osgParticle::ParticleProcessor::ABSOLUTE_RF;

        // TODO: also take into account the transform by placement in the scene
        osg::Matrix particletransform;
        if (rf == osgParticle::ParticleProcessor::ABSOLUTE_RF)
            particletransform = getWorldTransform(nifNode);

        int i=0;
        for (std::vector<Nif::NiParticleSystemController::Particle>::const_iterator it = partctrl->particles.begin();
             i<particledata->activeCount && it != partctrl->particles.end(); ++it, ++i)
        {
            const Nif::NiParticleSystemController::Particle& particle = *it;

            ParticleAgeSetter particletemplate(std::max(0.f, particle.lifetime));

            osgParticle::Particle* created = partsys->createParticle(&particletemplate);
            created->setLifeTime(std::max(0.f, particle.lifespan));
            osg::Vec4f adjustedVelocity = osg::Vec4f(particle.velocity, 0.f) * particletransform;
            created->setVelocity(osg::Vec3f(adjustedVelocity.x(), adjustedVelocity.y(), adjustedVelocity.z()));
            created->setPosition(particledata->vertices.at(particle.vertex) * particletransform);

            osg::Vec4f partcolor (1.f,1.f,1.f,1.f);
            if (particle.vertex < int(particledata->colors.size()))
                partcolor = particledata->colors.at(particle.vertex);

            float size = particledata->sizes.at(particle.vertex) * partctrl->size;

            created->setSizeRange(osgParticle::rangef(size, size));
        }

        partsys->setQuota(partctrl->numParticles);

        partsys->getDefaultParticleTemplate().setSizeRange(osgParticle::rangef(partctrl->size, partctrl->size));
        partsys->getDefaultParticleTemplate().setColorRange(osgParticle::rangev4(osg::Vec4f(1.f,1.f,1.f,1.f), osg::Vec4f(1.f,1.f,1.f,1.f)));
        partsys->getDefaultParticleTemplate().setAlphaRange(osgParticle::rangef(1.f, 1.f));

        // ---- emitter

        osg::ref_ptr<Emitter> emitter = new Emitter(targets);
        emitter->setParticleSystem(partsys);
        emitter->setReferenceFrame(osgParticle::ParticleProcessor::RELATIVE_RF);

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
        placer->setXRange(-partctrl->offsetRandom.x(), partctrl->offsetRandom.x());
        placer->setYRange(-partctrl->offsetRandom.y(), partctrl->offsetRandom.y());
        placer->setZRange(-partctrl->offsetRandom.z(), partctrl->offsetRandom.z());

        emitter->setPlacer(placer);

        // Note: we assume that the Emitter node is placed *before* the Particle node in the scene graph.
        // This seems to be true for all NIF files in the game that I've checked, suggesting that NIFs work similar to OSG with regards to update order.
        // If something ever violates this assumption, the worst that could happen is the culling being one frame late, which wouldn't be a disaster.

        FindRecIndexVisitor find (partctrl->emitter->recIndex);
        mRootNode->accept(find);
        if (!find.mFound)
        {
            std::cerr << "can't find emitter node, wrong node order?" << std::endl;
            return;
        }
        osg::Group* emitterNode = find.mFound;

        // Emitter attached to the emitter node. Note one side effect of the emitter using the CullVisitor is that hiding its node
        // actually causes the emitter to stop firing. Convenient, because MW behaves this way too!
        emitterNode->addChild(emitter);

        osg::ref_ptr<ParticleSystemController> callback(new ParticleSystemController(partctrl));
        setupController(partctrl, callback, animflags);
        emitter->setUpdateCallback(callback);

        // ----------- affector (must be after emitters in the scene graph)
        osgParticle::ModularProgram* program = new osgParticle::ModularProgram;
        program->setParticleSystem(partsys);
        program->setReferenceFrame(rf);
        emitterNode->addChild(program);
        for (Nif::ExtraPtr e = partctrl->extra; !e.empty(); e = e->extra)
        {
            if (e->recType == Nif::RC_NiParticleGrowFade)
            {
                const Nif::NiParticleGrowFade *gf = static_cast<const Nif::NiParticleGrowFade*>(e.getPtr());
                GrowFadeAffector* affector = new GrowFadeAffector(gf->growTime, gf->fadeTime);
                program->addOperator(affector);
            }
            else if (e->recType == Nif::RC_NiGravity)
            {
                const Nif::NiGravity* gr = static_cast<const Nif::NiGravity*>(e.getPtr());
                GravityAffector* affector = new GravityAffector(gr);
                program->addOperator(affector);
            }
            else if (e->recType == Nif::RC_NiParticleColorModifier)
            {
                const Nif::NiParticleColorModifier *cl = static_cast<const Nif::NiParticleColorModifier*>(e.getPtr());
                const Nif::NiColorData *clrdata = cl->data.getPtr();
                ParticleColorAffector* affector = new ParticleColorAffector(clrdata);
                program->addOperator(affector);
            }
            else if (e->recType == Nif::RC_NiParticleRotation)
            {
                // TODO: Implement?
            }
            else
                std::cerr << "Unhandled particle modifier " << e->recName << std::endl;
        }

        // -----------

        osg::ref_ptr<osg::Geode> geode (new osg::Geode);
        geode->addDrawable(partsys);

        std::vector<const Nif::Property*> materialProps;
        collectMaterialProperties(nifNode, materialProps);
        applyMaterialProperties(geode, materialProps, true, animflags);

        partsys->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        partsys->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);


        if (rf == osgParticle::ParticleProcessor::RELATIVE_RF)
            parentNode->addChild(geode);
        else
        {
            osg::MatrixTransform* trans = new osg::MatrixTransform;
            trans->setUpdateCallback(new InverseWorldMatrix);
            trans->addChild(geode);
            parentNode->addChild(trans);
        }

        // particle system updater (after the emitters and affectors in the scene graph)
        osgParticle::ParticleSystemUpdater* updater = new osgParticle::ParticleSystemUpdater;
        updater->addParticleSystem(partsys);
        parentNode->addChild(updater);
    }

    void Loader::triShapeToGeometry(const Nif::NiTriShape *triShape, osg::Geometry *geometry, osg::Geode* parentGeode, const std::map<int, int>& boundTextures, int animflags)
    {
        const Nif::NiTriShapeData* data = triShape->data.getPtr();

        const Nif::NiSkinInstance *skin = (triShape->skin.empty() ? NULL : triShape->skin.getPtr());
        if (skin)
        {
            // Convert vertices and normals to bone space from bind position. It would be
            // better to transform the bones into bind position, but there doesn't seem to
            // be a reliable way to do that.
            osg::ref_ptr<osg::Vec3Array> newVerts (new osg::Vec3Array(data->vertices.size()));
            osg::ref_ptr<osg::Vec3Array> newNormals (new osg::Vec3Array(data->normals.size()));

            const Nif::NiSkinData *skinData = skin->data.getPtr();
            const Nif::NodeList &bones = skin->bones;
            for(size_t b = 0;b < bones.length();b++)
            {
                osg::Matrixf mat = toMatrix(skinData->bones[b].trafo);

                mat = mat * getWorldTransform(bones[b].getPtr());

                const std::vector<Nif::NiSkinData::VertWeight> &weights = skinData->bones[b].weights;
                for(size_t i = 0;i < weights.size();i++)
                {
                    size_t index = weights[i].vertex;
                    float weight = weights[i].weight;

                    osg::Vec4f mult = (osg::Vec4f(data->vertices.at(index),1.f) * mat) * weight;
                    (*newVerts)[index] += osg::Vec3f(mult.x(),mult.y(),mult.z());
                    if(newNormals->size() > index)
                    {
                        osg::Vec4 normal(data->normals[index].x(), data->normals[index].y(), data->normals[index].z(), 0.f);
                        normal = (normal * mat) * weight;
                        (*newNormals)[index] += osg::Vec3f(normal.x(),normal.y(),normal.z());
                    }
                }
            }
            // Interpolating normalized normals doesn't necessarily give you a normalized result
            // Currently we're using GL_NORMALIZE, so this isn't needed
            //for (unsigned int i=0;i<newNormals->size();++i)
            //    (*newNormals)[i].normalize();

            geometry->setVertexArray(newVerts);
            if (!data->normals.empty())
                geometry->setNormalArray(newNormals, osg::Array::BIND_PER_VERTEX);
        }
        else
        {
            geometry->setVertexArray(new osg::Vec3Array(data->vertices.size(), &data->vertices[0]));
            if (!data->normals.empty())
                geometry->setNormalArray(new osg::Vec3Array(data->normals.size(), &data->normals[0]), osg::Array::BIND_PER_VERTEX);
        }

        for (std::map<int, int>::const_iterator it = boundTextures.begin(); it != boundTextures.end(); ++it)
        {
            int textureStage = it->first;
            int uvSet = it->second;
            if (uvSet >= (int)data->uvlist.size())
            {
                // Occurred in "ascendedsleeper.nif", but only for hidden Shadow nodes, apparently
                //std::cerr << "Warning: using an undefined UV set " << uvSet << " on TriShape " << triShape->name << std::endl;
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
        std::vector<const Nif::Property*> materialProps;
        collectMaterialProperties(triShape, materialProps);
        applyMaterialProperties(parentGeode, materialProps, !data->colors.empty(), animflags);
    }

    void Loader::handleTriShape(const Nif::NiTriShape* triShape, osg::Group* parentNode, const std::map<int, int>& boundTextures, int animflags)
    {
        osg::ref_ptr<osg::Geometry> geometry;
        if(!triShape->controller.empty())
        {
            Nif::ControllerPtr ctrl = triShape->controller;
            do {
                if(ctrl->recType == Nif::RC_NiGeomMorpherController && ctrl->flags & Nif::NiNode::ControllerFlag_Active)
                {
                    geometry = handleMorphGeometry(static_cast<const Nif::NiGeomMorpherController*>(ctrl.getPtr()));

                    osg::ref_ptr<GeomMorpherController> morphctrl = new GeomMorpherController(
                                static_cast<const Nif::NiGeomMorpherController*>(ctrl.getPtr())->data.getPtr());
                    setupController(ctrl.getPtr(), morphctrl, animflags);
                    geometry->setUpdateCallback(morphctrl);
                    break;
                }
            } while(!(ctrl=ctrl->next).empty());
        }

        if (!geometry.get())
            geometry = new osg::Geometry;

        osg::ref_ptr<osg::Geode> geode (new osg::Geode);
        geode->setName(triShape->name); // name will be used for part filtering
        triShapeToGeometry(triShape, geometry, geode, boundTextures, animflags);

        geode->addDrawable(geometry);

        parentNode->addChild(geode);
    }

    void Loader::handleSkinnedTriShape(const Nif::NiTriShape *triShape, osg::Group *parentNode, const std::map<int, int>& boundTextures, int animflags)
    {
        osg::ref_ptr<osg::Geode> geode (new osg::Geode);
        geode->setName(triShape->name); // name will be used for part filtering
        osg::ref_ptr<osg::Geometry> geometry (new osg::Geometry);
        triShapeToGeometry(triShape, geometry, geode, boundTextures, animflags);

        osg::ref_ptr<osgAnimation::RigGeometry> rig(new osgAnimation::RigGeometry);
        rig->setSourceGeometry(geometry);
        // Slightly expand the bounding box to account for movement of the bones
        // For more accuracy the skinning should be relative to the parent of the first skinned bone,
        // rather than the root bone.
        osg::BoundingBox box = geometry->getBound();
        box.expandBy(box._min-(box._max-box._min)/2);
        box.expandBy(box._max+(box._max-box._min)/2);
        rig->setInitialBound(box);

        const Nif::NiSkinInstance *skin = triShape->skin.getPtr();

        // Assign bone weights
        osg::ref_ptr<osgAnimation::VertexInfluenceMap> map (new osgAnimation::VertexInfluenceMap);

        const Nif::NiSkinData *data = skin->data.getPtr();
        const Nif::NodeList &bones = skin->bones;
        for(size_t i = 0;i < bones.length();i++)
        {
            std::string boneName = bones[i].getPtr()->name;

            osgAnimation::VertexInfluence influence;
            influence.setName(boneName);
            const std::vector<Nif::NiSkinData::VertWeight> &weights = data->bones[i].weights;
            influence.reserve(weights.size());
            for(size_t j = 0;j < weights.size();j++)
            {
                osgAnimation::VertexIndexWeight indexWeight = std::make_pair(weights[j].vertex, weights[j].weight);
                influence.push_back(indexWeight);
            }

            map->insert(std::make_pair(boneName, influence));
        }
        rig->setInfluenceMap(map);

        osg::ref_ptr<osg::MatrixTransform> trans(new osg::MatrixTransform);
        trans->setUpdateCallback(new InvertBoneMatrix());

        geode->addDrawable(rig);

        trans->addChild(geode);
        parentNode->addChild(trans);
    }

    void Loader::handleProperty(const Nif::Property *property, const Nif::Node* nifNode,
                        osg::Node *node, std::map<int, int>& boundTextures, int animflags)
    {
        osg::StateSet* stateset = node->getOrCreateStateSet();

        switch (property->recType)
        {
        case Nif::RC_NiStencilProperty:
        {
            const Nif::NiStencilProperty* stencilprop = static_cast<const Nif::NiStencilProperty*>(property);
            osg::FrontFace* frontFace = new osg::FrontFace;
            switch (stencilprop->data.drawMode)
            {
            case 1:
                frontFace->setMode(osg::FrontFace::CLOCKWISE);
                break;
            case 0:
            case 2:
            default:
                frontFace->setMode(osg::FrontFace::COUNTER_CLOCKWISE);
                break;
            }

            stateset->setAttribute(frontFace, osg::StateAttribute::ON);
            stateset->setMode(GL_CULL_FACE, stencilprop->data.drawMode == 3 ? osg::StateAttribute::OFF
                                                                            : osg::StateAttribute::ON);

            // Stencil settings not enabled yet, not sure if the original engine is actually using them,
            // since they might conflict with Morrowind's stencil shadows.
            /*
            osg::Stencil* stencil = new osg::Stencil;
            stencil->setFunction(func, stencilprop->data.stencilRef, stencilprop->data.stencilMask);

            stateset->setMode(GL_STENCIL_TEST, stencilprop->data.enabled != 0 ? osg::StateAttribute::ON
                                                                              : osg::StateAttribute::OFF);
            */
        }
        case Nif::RC_NiWireframeProperty:
        {
            const Nif::NiWireframeProperty* wireprop = static_cast<const Nif::NiWireframeProperty*>(property);
            osg::PolygonMode* mode = new osg::PolygonMode;
            mode->setMode(osg::PolygonMode::FRONT_AND_BACK, wireprop->flags == 0 ? osg::PolygonMode::FILL
                                                                                 : osg::PolygonMode::LINE);
            stateset->setAttributeAndModes(mode, osg::StateAttribute::ON);
            break;
        }
        case Nif::RC_NiZBufferProperty:
        {
            const Nif::NiZBufferProperty* zprop = static_cast<const Nif::NiZBufferProperty*>(property);
            // VER_MW doesn't support a DepthFunction according to NifSkope
            osg::Depth* depth = new osg::Depth;
            depth->setWriteMask((zprop->flags>>1)&1);
            stateset->setAttributeAndModes(depth, osg::StateAttribute::ON);
            break;
        }
        // OSG groups the material properties that NIFs have separate, so we have to parse them all again when one changed
        case Nif::RC_NiMaterialProperty:
        case Nif::RC_NiVertexColorProperty:
        case Nif::RC_NiSpecularProperty:
        {
            // Handled in handleTriShape so we know whether vertex colors are available
            break;
        }
        case Nif::RC_NiAlphaProperty:
        {
            const Nif::NiAlphaProperty* alphaprop = static_cast<const Nif::NiAlphaProperty*>(property);
            osg::BlendFunc* blendfunc = new osg::BlendFunc;
            if (alphaprop->flags&1)
            {
                blendfunc->setFunction(getBlendMode((alphaprop->flags>>1)&0xf),
                                       getBlendMode((alphaprop->flags>>5)&0xf));
                stateset->setAttributeAndModes(blendfunc, osg::StateAttribute::ON);

                bool noSort = (alphaprop->flags>>13)&1;
                if (!noSort)
                {
                    stateset->setNestRenderBins(false);
                    stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
                }
            }
            else
            {
                stateset->setAttributeAndModes(blendfunc, osg::StateAttribute::OFF);
                stateset->setNestRenderBins(false);
                stateset->setRenderingHint(osg::StateSet::OPAQUE_BIN);
            }

            osg::AlphaFunc* alphafunc = new osg::AlphaFunc;
            if((alphaprop->flags>>9)&1)
            {
                alphafunc->setFunction(getTestMode((alphaprop->flags>>10)&0x7), alphaprop->data.threshold/255.f);
                stateset->setAttributeAndModes(alphafunc, osg::StateAttribute::ON);
            }
            else
                stateset->setAttributeAndModes(alphafunc, osg::StateAttribute::OFF);
            break;
        }
        case Nif::RC_NiTexturingProperty:
        {
            const Nif::NiTexturingProperty* texprop = static_cast<const Nif::NiTexturingProperty*>(property);
            for (int i=0; i<Nif::NiTexturingProperty::NumTextures; ++i)
            {
                if (texprop->textures[i].inUse)
                {
                    if (i != Nif::NiTexturingProperty::BaseTexture
                            && i != Nif::NiTexturingProperty::GlowTexture
                            && i != Nif::NiTexturingProperty::DarkTexture
                            && i != Nif::NiTexturingProperty::DetailTexture)
                    {
                        std::cerr << "Warning: unhandled texture stage " << i << std::endl;
                        continue;
                    }

                    const Nif::NiTexturingProperty::Texture& tex = texprop->textures[i];
                    if(tex.texture.empty())
                    {
                        std::cerr << "Warning: texture layer " << i << " is in use but empty " << std::endl;
                        continue;
                    }
                    const Nif::NiSourceTexture *st = tex.texture.getPtr();
                    if (!st->external)
                    {
                        std::cerr << "Warning: unhandled internal texture " << std::endl;
                        continue;
                    }

                    std::string filename = Misc::ResourceHelpers::correctTexturePath(st->filename, resourceManager);

                    // tx_creature_werewolf.dds isn't loading in the correct format without this option
                    osgDB::Options* opts = new osgDB::Options;
                    opts->setOptionString("dds_dxt1_detect_rgba");
                    osgDB::ReaderWriter* reader = osgDB::Registry::instance()->getReaderWriterForExtension("dds");
                    osgDB::ReaderWriter::ReadResult result = reader->readImage(*resourceManager->get(filename.c_str()), opts);
                    osg::Image* image = result.getImage();
                    osg::Texture2D* texture2d = new osg::Texture2D;
                    texture2d->setUnRefImageDataAfterApply(true);
                    texture2d->setImage(image);

                    unsigned int clamp = static_cast<unsigned int>(tex.clamp);
                    int wrapT = (clamp) & 0x1;
                    int wrapS = (clamp >> 1) & 0x1;

                    texture2d->setWrap(osg::Texture::WRAP_S, wrapS ? osg::Texture::REPEAT : osg::Texture::CLAMP);
                    texture2d->setWrap(osg::Texture::WRAP_T, wrapT ? osg::Texture::REPEAT : osg::Texture::CLAMP);

                    stateset->setTextureAttributeAndModes(i, texture2d, osg::StateAttribute::ON);

                    if (i == Nif::NiTexturingProperty::GlowTexture)
                    {
                        osg::TexEnv* texEnv = new osg::TexEnv;
                        texEnv->setMode(osg::TexEnv::ADD);
                        stateset->setTextureAttributeAndModes(i, texEnv, osg::StateAttribute::ON);
                    }
                    else if (i == Nif::NiTexturingProperty::DarkTexture)
                    {
                        // untested
                        osg::TexEnv* texEnv = new osg::TexEnv;
                        texEnv->setMode(osg::TexEnv::MODULATE);
                        stateset->setTextureAttributeAndModes(i, texEnv, osg::StateAttribute::ON);
                    }
                    else if (i == Nif::NiTexturingProperty::DetailTexture)
                    {
                        // untested
                        osg::TexEnvCombine* texEnv = new osg::TexEnvCombine;
                        texEnv->setScale_RGB(2.f);
                        texEnv->setCombine_Alpha(GL_MODULATE);
                        texEnv->setOperand0_Alpha(GL_SRC_ALPHA);
                        texEnv->setOperand1_Alpha(GL_SRC_ALPHA);
                        texEnv->setSource0_Alpha(GL_PREVIOUS);
                        texEnv->setSource1_Alpha(GL_TEXTURE);
                        texEnv->setCombine_RGB(GL_MODULATE);
                        texEnv->setOperand0_RGB(GL_SRC_COLOR);
                        texEnv->setOperand1_RGB(GL_SRC_COLOR);
                        texEnv->setSource0_RGB(GL_PREVIOUS);
                        texEnv->setSource1_RGB(GL_TEXTURE);
                        stateset->setTextureAttributeAndModes(i, texEnv, osg::StateAttribute::ON);
                    }

                    boundTextures[i] = tex.uvSet;
                }
                else if (boundTextures.find(i) != boundTextures.end())
                {
                    stateset->setTextureAttributeAndModes(i, new osg::Texture2D, osg::StateAttribute::OFF);
                    boundTextures.erase(i);
                }
                handleTextureControllers(texprop, node, stateset, animflags);
            }
            break;
        }
        case Nif::RC_NiDitherProperty:
        {
            stateset->setMode(GL_DITHER, property->flags != 0 ? osg::StateAttribute::ON
                                                              : osg::StateAttribute::OFF);
            break;
        }
        default:
            std::cerr << "Unhandled " << property->recName << std::endl;
            break;
        }
    }

    void Loader::applyMaterialProperties(osg::Node* node, const std::vector<const Nif::Property*>& properties,
                                         bool hasVertexColors, int animflags)
    {
        osg::StateSet* stateset = node->getOrCreateStateSet();

        int specFlags = 0; // Specular is disabled by default, even if there's a specular color in the NiMaterialProperty
        osg::Material* mat = new osg::Material;
        mat->setColorMode(hasVertexColors ? osg::Material::AMBIENT_AND_DIFFUSE : osg::Material::OFF);
        // TODO: check if the OpenGL default material values are actually the default NIF material values, for when there's no NiMaterialProperty
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
                    handleMaterialControllers(matprop, node, stateset, animflags);

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
            }
            }
        }

        if (specFlags == 0)
            mat->setSpecular(osg::Material::FRONT_AND_BACK, osg::Vec4f(0.f,0.f,0.f,0.f));

        stateset->setAttributeAndModes(mat, osg::StateAttribute::ON);
    }
}
