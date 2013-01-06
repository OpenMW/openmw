#include "skeleton.hpp"

#include <OgreSkeletonManager.h>
#include <OgreResource.h>
#include <OgreSkeleton.h>
#include <OgreBone.h>

#include <components/nif/node.hpp>
#include <components/misc/stringops.hpp>

namespace NifOgre
{
template <typename value_type>
static value_type min (value_type V0, value_type V1, value_type V2, value_type V3)
{
    return std::min (std::min (V0, V1), std::min (V2, V3));
}

void NIFSkeletonLoader::buildAnimation(Ogre::Skeleton *skel, const std::string &name, const std::vector<const Nif::NiKeyframeController*> &ctrls, const std::vector<std::string> &targets, float startTime, float stopTime)
{
    Ogre::Animation *anim = skel->createAnimation(name, stopTime);

    for(size_t i = 0;i < ctrls.size();i++)
    {
        const Nif::NiKeyframeController *kfc = ctrls[i];
        if(kfc->data.empty())
            continue;
        const Nif::NiKeyframeData *kf = kfc->data.getPtr();

        Ogre::Bone *bone = skel->getBone(targets[i]);
        // NOTE: For some reason, Ogre doesn't like the node track ID being different from
        // the bone ID
        Ogre::NodeAnimationTrack *nodetrack = anim->hasNodeTrack(bone->getHandle()) ?
                                              anim->getNodeTrack(bone->getHandle()) :
                                              anim->createNodeTrack(bone->getHandle(), bone);

        Nif::QuaternionCurve::interpolator rci (kf->mRotations);
        Nif::Vector3Curve::interpolator tci (kf->mTranslations);
        Nif::FloatCurve::interpolator sci (kf->mScales);

        float next_timestamp = startTime;

        for (;;)
        {
            static const Ogre::Vector3 one (1,1,1);

            Ogre::TransformKeyFrame *kframe;
            kframe = nodetrack->createNodeKeyFrame (next_timestamp);

            if (rci.hasData ()) kframe->setRotation  (rci.valueAt (next_timestamp));
            if (tci.hasData ()) kframe->setTranslate (tci.valueAt (next_timestamp));
            if (sci.hasData ()) kframe->setScale     (sci.valueAt (next_timestamp)*one);

            if (next_timestamp >= stopTime)
                break;

            next_timestamp = min (stopTime,
                                  rci.nextTime (),
                                  tci.nextTime (),
                                  sci.nextTime ());
        }
    }
    anim->optimise();
}


TextKeyMap NIFSkeletonLoader::extractTextKeys(const Nif::NiTextKeyExtraData *tk)
{
    TextKeyMap textkeys;
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
            std::string result = str.substr(pos, nextpos-pos);
            textkeys.insert(std::make_pair(tk->list[i].time, Misc::StringUtils::toLower(result)));

            pos = nextpos;
        }
    }
    return textkeys;
}

void NIFSkeletonLoader::buildBones(Ogre::Skeleton *skel, const Nif::Node *node, Ogre::Bone *&animroot, TextKeyMap &textkeys, std::vector<Nif::NiKeyframeController const*> &ctrls, Ogre::Bone *parent)
{
    Ogre::Bone *bone;
    if(!skel->hasBone(node->name))
        bone = skel->createBone(node->name);
    else
        bone = skel->createBone();
    if(parent) parent->addChild(bone);
    mNifToOgreHandleMap[node->recIndex] = bone->getHandle();

    bone->setOrientation(node->trafo.rotation);
    bone->setPosition(node->trafo.pos);
    bone->setScale(Ogre::Vector3(node->trafo.scale));
    bone->setBindingPose();

    if(!(node->recType == Nif::RC_NiNode || /* Nothing special; children traversed below */
         node->recType == Nif::RC_RootCollisionNode || /* handled in nifbullet (hopefully) */
         node->recType == Nif::RC_NiTriShape || /* Handled in the mesh loader */
         node->recType == Nif::RC_NiCamera ||
         node->recType == Nif::RC_NiAutoNormalParticles ||
         node->recType == Nif::RC_NiRotatingParticles
         ))
        warn("Unhandled "+node->recName+" "+node->name+" in "+skel->getName());

    Nif::ControllerPtr ctrl = node->controller;
    while(!ctrl.empty())
    {
        if(ctrl->recType == Nif::RC_NiKeyframeController)
            ctrls.push_back(static_cast<const Nif::NiKeyframeController*>(ctrl.getPtr()));
        else if(!(ctrl->recType == Nif::RC_NiParticleSystemController ||
                  ctrl->recType == Nif::RC_NiVisController ||
                  ctrl->recType == Nif::RC_NiUVController
                  ))
            warn("Unhandled "+ctrl->recName+" from node "+node->name+" in "+skel->getName());
        ctrl = ctrl->next;
    }

    Nif::ExtraPtr e = node->extra;
    while(!e.empty())
    {
        if(e->recType == Nif::RC_NiTextKeyExtraData && !animroot)
        {
            const Nif::NiTextKeyExtraData *tk = static_cast<const Nif::NiTextKeyExtraData*>(e.getPtr());
            textkeys = extractTextKeys(tk);
            animroot = bone;
        }
        e = e->extra;
    }

    const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(node);
    if(ninode)
    {
        const Nif::NodeList &children = ninode->children;
        for(size_t i = 0;i < children.length();i++)
        {
            if(!children[i].empty())
                buildBones(skel, children[i].getPtr(), animroot, textkeys, ctrls, bone);
        }
    }
}

void NIFSkeletonLoader::loadResource(Ogre::Resource *resource)
{
    Ogre::Skeleton *skel = dynamic_cast<Ogre::Skeleton*>(resource);
    OgreAssert(skel, "Attempting to load a skeleton into a non-skeleton resource!");

    Nif::NIFFile::ptr nif(Nif::NIFFile::create(skel->getName()));
    const Nif::Node *node = static_cast<const Nif::Node*>(nif->getRoot(0));

    std::vector<const Nif::NiKeyframeController*> ctrls;
    Ogre::Bone *animroot = NULL;
    TextKeyMap textkeys;
    try {
        buildBones(skel, node, animroot, textkeys, ctrls);
    }
    catch(std::exception &e) {
        std::cerr<< "Exception while loading "<<skel->getName() <<std::endl;
        std::cerr<< e.what() <<std::endl;
        return;
    }

    /* Animations without textkeys don't get Ogre::Animation objects. */
    if(!animroot)
        return;

    std::vector<std::string> targets;
    // TODO: If ctrls.size() == 0, check for a .kf file sharing the name of the .nif file
    if(ctrls.size() == 0) // No animations? Then we're done.
        return;

    float maxtime = 0.0f;
    for(size_t i = 0;i < ctrls.size();i++)
    {
        const Nif::NiKeyframeController *ctrl = ctrls[i];
        maxtime = std::max(maxtime, ctrl->timeStop);
        Nif::Named *target = dynamic_cast<Nif::Named*>(ctrl->target.getPtr());
        if(target != NULL)
            targets.push_back(target->name);
    }

    if(targets.size() != ctrls.size())
    {
        warn("Target size mismatch ("+Ogre::StringConverter::toString(targets.size())+" targets, "+
             Ogre::StringConverter::toString(ctrls.size())+" controllers)");
        return;
    }

    Ogre::UserObjectBindings &bindings = animroot->getUserObjectBindings();
    bindings.setUserAny(sTextKeyExtraDataID, Ogre::Any(true));

    std::string currentgroup;
    TextKeyMap::const_iterator keyiter = textkeys.begin();
    for(keyiter = textkeys.begin();keyiter != textkeys.end();keyiter++)
    {
        std::string::size_type sep = keyiter->second.find(':');
        if((sep == currentgroup.length() && keyiter->second.compare(0, sep, currentgroup) == 0) ||
           (sep == sizeof("soundgen")-1 && keyiter->second.compare(0, sep, "soundgen") == 0) ||
           (sep == sizeof("sound")-1 && keyiter->second.compare(0, sep, "sound") == 0))
            continue;
        currentgroup = keyiter->second.substr(0, sep);

        if(skel->hasAnimation(currentgroup))
            continue;

        TextKeyMap::const_iterator lastkeyiter = textkeys.end();
        while((--lastkeyiter)->first > keyiter->first)
        {
            if(lastkeyiter->second.find(':') == currentgroup.length() &&
               lastkeyiter->second.compare(0, currentgroup.length(), currentgroup) == 0)
                break;
        }

        buildAnimation(skel, currentgroup, ctrls, targets, keyiter->first, lastkeyiter->first);

        TextKeyMap::const_iterator insiter(keyiter);
        TextKeyMap groupkeys;
        do {
            sep = insiter->second.find(':');
            if(sep == currentgroup.length() && insiter->second.compare(0, sep, currentgroup) == 0)
                groupkeys.insert(std::make_pair(insiter->first, insiter->second.substr(sep+2)));
            else if((sep == sizeof("soundgen")-1 && insiter->second.compare(0, sep, "soundgen") == 0) ||
                    (sep == sizeof("sound")-1 && insiter->second.compare(0, sep, "sound") == 0))
                groupkeys.insert(std::make_pair(insiter->first, insiter->second));
        } while(insiter++ != lastkeyiter);

        bindings.setUserAny(std::string(sTextKeyExtraDataID)+"@"+currentgroup, Ogre::Any(groupkeys));
    }
}


Ogre::SkeletonPtr NIFSkeletonLoader::createSkeleton(const std::string &name, const std::string &group, const Nif::Node *node)
{
    /* We need to be a little aggressive here, since some NIFs have a crap-ton
     * of nodes and Ogre only supports 256 bones. We will skip a skeleton if:
     * There are no bones used for skinning, there are no controllers on non-
     * NiTriShape nodes, there are no nodes named "AttachLight", and the tree
     * consists of NiNode, NiTriShape, and RootCollisionNode types only.
     */
    if(!node->boneTrafo)
    {
        if(node->recType == Nif::RC_NiTriShape)
            return Ogre::SkeletonPtr();
        if(node->controller.empty() && node->name != "AttachLight")
        {
            if(node->recType == Nif::RC_NiNode || node->recType == Nif::RC_RootCollisionNode)
            {
                const Nif::NiNode *ninode = static_cast<const Nif::NiNode*>(node);
                const Nif::NodeList &children = ninode->children;
                for(size_t i = 0;i < children.length();i++)
                {
                    if(!children[i].empty())
                    {
                        Ogre::SkeletonPtr skel = createSkeleton(name, group, children[i].getPtr());
                        if(!skel.isNull())
                            return skel;
                    }
                }
                return Ogre::SkeletonPtr();
            }
        }
    }

    Ogre::SkeletonManager &skelMgr = Ogre::SkeletonManager::getSingleton();
    return skelMgr.create(name, group, true, &sLoaders[name]);
}

// Looks up an Ogre Bone handle ID from a NIF's record index. Should only be
// used when the bone name is insufficient as this is a relatively slow lookup
int NIFSkeletonLoader::lookupOgreBoneHandle(const std::string &nifname, int idx)
{
    LoaderMap::const_iterator loader = sLoaders.find(nifname);
    if(loader != sLoaders.end())
    {
        std::map<int,int>::const_iterator entry = loader->second.mNifToOgreHandleMap.find(idx);
        if(entry != loader->second.mNifToOgreHandleMap.end())
            return entry->second;
    }
    throw std::runtime_error("Invalid NIF record lookup ("+nifname+", index "+Ogre::StringConverter::toString(idx)+")");
}

NIFSkeletonLoader::LoaderMap NIFSkeletonLoader::sLoaders;

}
