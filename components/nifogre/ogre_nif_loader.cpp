/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (ogre_nif_loader.cpp) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */

//loadResource->handleNode->handleNiTriShape->createSubMesh

#include "ogre_nif_loader.hpp"

#include <algorithm>

#include <OgreMaterialManager.h>
#include <OgreMeshManager.h>
#include <OgreHardwareBufferManager.h>
#include <OgreSkeletonManager.h>
#include <OgreTechnique.h>
#include <OgreSubMesh.h>
#include <OgreRoot.h>
#include <OgreEntity.h>
#include <OgreTagPoint.h>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/functional/hash.hpp>

#include <extern/shiny/Main/Factory.hpp>

#include <components/nif/node.hpp>
#include <components/misc/stringops.hpp>
#include <components/settings/settings.hpp>
#include <components/nifoverrides/nifoverrides.hpp>

typedef unsigned char ubyte;

namespace std
{

// These operators allow extra data types to be stored in an Ogre::Any
// object, which can then be stored in user object bindings on the nodes

// TODO: Do something useful
ostream& operator<<(ostream &o, const NifOgre::TextKeyMap&)
{ return o; }

}

namespace NifOgre
{
// Helper class that computes the bounding box and of a mesh
class BoundsFinder
{
    struct MaxMinFinder
    {
        float max, min;

        MaxMinFinder()
        {
            min = std::numeric_limits<float>::infinity();
            max = -min;
        }

        void add(float f)
        {
            if (f > max) max = f;
            if (f < min) min = f;
        }

        // Return Max(max**2, min**2)
        float getMaxSquared()
        {
            float m1 = max*max;
            float m2 = min*min;
            if (m1 >= m2) return m1;
            return m2;
        }
    };

    MaxMinFinder X, Y, Z;

public:
    // Add 'verts' vertices to the calculation. The 'data' pointer is
    // expected to point to 3*verts floats representing x,y,z for each
    // point.
    void add(float *data, int verts)
    {
        for (int i=0;i<verts;i++)
        {
            X.add(*(data++));
            Y.add(*(data++));
            Z.add(*(data++));
        }
    }

    // True if this structure has valid values
    bool isValid()
    {
        return
            minX() <= maxX() &&
            minY() <= maxY() &&
            minZ() <= maxZ();
    }

    // Compute radius
    float getRadius()
    {
        assert(isValid());

        // The radius is computed from the origin, not from the geometric
        // center of the mesh.
        return sqrt(X.getMaxSquared() + Y.getMaxSquared() + Z.getMaxSquared());
    }

    float minX() {
        return X.min;
    }
    float maxX() {
        return X.max;
    }
    float minY() {
        return Y.min;
    }
    float maxY() {
        return Y.max;
    }
    float minZ() {
        return Z.min;
    }
    float maxZ() {
        return Z.max;
    }
};


class NIFSkeletonLoader : public Ogre::ManualResourceLoader {

static void warn(const std::string &msg)
{
    std::cerr << "NIFSkeletonLoader: Warn: " << msg << std::endl;
}

static void fail(const std::string &msg)
{
    std::cerr << "NIFSkeletonLoader: Fail: "<< msg << std::endl;
    abort();
}


static TextKeyMap extractTextKeys(const Nif::NiTextKeyExtraData *tk)
{
    TextKeyMap textkeys;
    for(size_t i = 0;i < tk->list.size();i++)
    {
        const std::string &str = tk->list[i].text;
        std::string::size_type pos = 0;
        while(pos < str.length())
        {
            while(pos < str.length() && ::isspace(str[pos]))
                pos++;
            if(pos >= str.length())
                break;

            std::string::size_type nextpos = std::min(str.find('\r', pos), str.find('\n', pos));
            textkeys.insert(std::make_pair(tk->list[i].time, str.substr(pos, nextpos-pos)));

            pos = nextpos;
        }
    }
    return textkeys;
}


void buildBones(Ogre::Skeleton *skel, const Nif::Node *node, std::vector<Nif::NiKeyframeController const*> &ctrls, Ogre::Bone *parent=NULL)
{
    Ogre::Bone *bone;
    if(!skel->hasBone(node->name))
        bone = skel->createBone(node->name);
    else
        bone = skel->createBone();
    if(parent) parent->addChild(bone);

    bone->setOrientation(node->trafo.rotation);
    bone->setPosition(node->trafo.pos);
    bone->setScale(Ogre::Vector3(node->trafo.scale));
    bone->setBindingPose();
    bone->setInitialState();

    Nif::ControllerPtr ctrl = node->controller;
    while(!ctrl.empty())
    {
        if(ctrl->recType == Nif::RC_NiKeyframeController)
            ctrls.push_back(static_cast<Nif::NiKeyframeController*>(ctrl.getPtr()));
        ctrl = ctrl->next;
    }

    Nif::ExtraPtr e = node->extra;
    while(!e.empty())
    {
        if(e->recType == Nif::RC_NiTextKeyExtraData)
        {
            const Nif::NiTextKeyExtraData *tk = static_cast<const Nif::NiTextKeyExtraData*>(e.getPtr());
            bone->getUserObjectBindings().setUserAny("TextKeyExtraData", Ogre::Any(extractTextKeys(tk)));
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
                buildBones(skel, children[i].getPtr(), ctrls, bone);
        }
    }
}


/* Comparitor to help sort Key<> vectors */
template<class T>
struct KeyTimeSort
{
    bool operator()(const Nif::KeyT<T> &lhs, const Nif::KeyT<T> &rhs) const
    { return lhs.mTime < rhs.mTime; }
};


typedef std::map<std::string,NIFSkeletonLoader> LoaderMap;
static LoaderMap sLoaders;

public:
void loadResource(Ogre::Resource *resource)
{
    Ogre::Skeleton *skel = dynamic_cast<Ogre::Skeleton*>(resource);
    OgreAssert(skel, "Attempting to load a skeleton into a non-skeleton resource!");

    Nif::NIFFile::ptr pnif(Nif::NIFFile::create (skel->getName()));
    Nif::NIFFile & nif = *pnif.get ();
    const Nif::Node *node = dynamic_cast<const Nif::Node*>(nif.getRecord(0));

    std::vector<Nif::NiKeyframeController const*> ctrls;
    buildBones(skel, node, ctrls);

    std::vector<std::string> targets;
    // TODO: If ctrls.size() == 0, check for a .kf file sharing the name of the .nif file
    if(ctrls.size() == 0) // No animations? Then we're done.
        return;

    float maxtime = 0.0f;
    for(size_t i = 0;i < ctrls.size();i++)
    {
        Nif::NiKeyframeController const *ctrl = ctrls[i];
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

    Ogre::Animation *anim = skel->createAnimation(skel->getName(), maxtime);
    /* HACK: Pre-create the node tracks by matching the track IDs with the
     * bone IDs. Otherwise, Ogre animates the wrong bones. */
    size_t bonecount = skel->getNumBones();
    for(size_t i = 0;i < bonecount;i++)
        anim->createNodeTrack(i, skel->getBone(i));

    for(size_t i = 0;i < ctrls.size();i++)
    {
        Nif::NiKeyframeController const *kfc = ctrls[i];
        Nif::NiKeyframeData const *kf = kfc->data.getPtr();

        /* Get the keyframes and make sure they're sorted first to last */
        Nif::QuaternionKeyList quatkeys = kf->mRotations;
        Nif::Vector3KeyList trankeys = kf->mTranslations;
        Nif::FloatKeyList scalekeys = kf->mScales;
        std::sort(quatkeys.mKeys.begin(), quatkeys.mKeys.end(), KeyTimeSort<Ogre::Quaternion>());
        std::sort(trankeys.mKeys.begin(), trankeys.mKeys.end(), KeyTimeSort<Ogre::Vector3>());
        std::sort(scalekeys.mKeys.begin(), scalekeys.mKeys.end(), KeyTimeSort<float>());

        Nif::QuaternionKeyList::VecType::const_iterator quatiter = quatkeys.mKeys.begin();
        Nif::Vector3KeyList::VecType::const_iterator traniter = trankeys.mKeys.begin();
        Nif::FloatKeyList::VecType::const_iterator scaleiter = scalekeys.mKeys.begin();

        Ogre::Bone *bone = skel->getBone(targets[i]);
        const Ogre::Quaternion startquat = bone->getInitialOrientation();
        const Ogre::Vector3 starttrans = bone->getInitialPosition();
        const Ogre::Vector3 startscale = bone->getInitialScale();
        Ogre::NodeAnimationTrack *nodetrack = anim->getNodeTrack(bone->getHandle());

        Ogre::Quaternion lastquat, curquat;
        Ogre::Vector3 lasttrans(0.0f), curtrans(0.0f);
        Ogre::Vector3 lastscale(1.0f), curscale(1.0f);
        if(quatiter != quatkeys.mKeys.end())
            lastquat = curquat = startquat.Inverse() * quatiter->mValue;
        if(traniter != trankeys.mKeys.end())
            lasttrans = curtrans = traniter->mValue - starttrans;
        if(scaleiter != scalekeys.mKeys.end())
            lastscale = curscale = Ogre::Vector3(scaleiter->mValue) / startscale;
        bool didlast = false;

        while(!didlast)
        {
            float curtime = kfc->timeStop;

            //Get latest time
            if(quatiter != quatkeys.mKeys.end())
                curtime = std::min(curtime, quatiter->mTime);
            if(traniter != trankeys.mKeys.end())
                curtime = std::min(curtime, traniter->mTime);
            if(scaleiter != scalekeys.mKeys.end())
                curtime = std::min(curtime, scaleiter->mTime);

            curtime = std::max(curtime, kfc->timeStart);
            if(curtime >= kfc->timeStop)
            {
                didlast = true;
                curtime = kfc->timeStop;
            }

            // Get the latest quaternions, translations, and scales for the
            // current time
            while(quatiter != quatkeys.mKeys.end() && curtime >= quatiter->mTime)
            {
                lastquat = curquat;
                quatiter++;
                if(quatiter != quatkeys.mKeys.end())
                    curquat = startquat.Inverse() * quatiter->mValue ;
            }
            while(traniter != trankeys.mKeys.end() && curtime >= traniter->mTime)
            {
                lasttrans = curtrans;
                traniter++;
                if(traniter != trankeys.mKeys.end())
                    curtrans = traniter->mValue - starttrans;
            }
            while(scaleiter != scalekeys.mKeys.end() && curtime >= scaleiter->mTime)
            {
                lastscale = curscale;
                scaleiter++;
                if(scaleiter != scalekeys.mKeys.end())
                    curscale = Ogre::Vector3(scaleiter->mValue) / startscale;
            }

            Ogre::TransformKeyFrame *kframe;
            kframe = nodetrack->createNodeKeyFrame(curtime);
            if(quatiter == quatkeys.mKeys.end() || quatiter == quatkeys.mKeys.begin())
                kframe->setRotation(curquat);
            else
            {
                Nif::QuaternionKeyList::VecType::const_iterator last = quatiter-1;
                float diff = (curtime-last->mTime) / (quatiter->mTime-last->mTime);
                kframe->setRotation(Ogre::Quaternion::nlerp(diff, lastquat, curquat));
            }
            if(traniter == trankeys.mKeys.end() || traniter == trankeys.mKeys.begin())
                kframe->setTranslate(curtrans);
            else
            {
                Nif::Vector3KeyList::VecType::const_iterator last = traniter-1;
                float diff = (curtime-last->mTime) / (traniter->mTime-last->mTime);
                kframe->setTranslate(lasttrans + ((curtrans-lasttrans)*diff));
            }
            if(scaleiter == scalekeys.mKeys.end() || scaleiter == scalekeys.mKeys.begin())
                kframe->setScale(curscale);
            else
            {
                Nif::FloatKeyList::VecType::const_iterator last = scaleiter-1;
                float diff = (curtime-last->mTime) / (scaleiter->mTime-last->mTime);
                kframe->setScale(lastscale + ((curscale-lastscale)*diff));
            }
        }
    }
    anim->optimise();
}

bool createSkeleton(const std::string &name, const std::string &group, const Nif::Node *node)
{
    if(node->boneTrafo != NULL)
    {
        Ogre::SkeletonManager &skelMgr = Ogre::SkeletonManager::getSingleton();
        Ogre::SkeletonPtr skel = skelMgr.getByName(name);
        if(skel.isNull())
        {
            NIFSkeletonLoader *loader = &sLoaders[name];
            skel = skelMgr.create(name, group, true, loader);
        }
        return true;
    }

    const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(node);
    if(ninode)
    {
        const Nif::NodeList &children = ninode->children;
        for(size_t i = 0;i < children.length();i++)
        {
            if(!children[i].empty())
            {
                if(createSkeleton(name, group, children[i].getPtr()))
                    return true;
            }
        }
    }
    return false;
}

};
NIFSkeletonLoader::LoaderMap NIFSkeletonLoader::sLoaders;


// Conversion of blend / test mode from NIF -> OGRE.
// Not in use yet, so let's comment it out.
/*
static SceneBlendFactor getBlendFactor(int mode)
{
  switch(mode)
    {
    case 0: return SBF_ONE;
    case 1: return SBF_ZERO;
    case 2: return SBF_SOURCE_COLOUR;
    case 3: return SBF_ONE_MINUS_SOURCE_COLOUR;
    case 4: return SBF_DEST_COLOUR;
    case 5: return SBF_ONE_MINUS_DEST_COLOUR;
    case 6: return SBF_SOURCE_ALPHA;
    case 7: return SBF_ONE_MINUS_SOURCE_ALPHA;
    case 8: return SBF_DEST_ALPHA;
    case 9: return SBF_ONE_MINUS_DEST_ALPHA;
      // [Comment from Chris Robinson:] Can't handle this mode? :/
      // case 10: return SBF_SOURCE_ALPHA_SATURATE;
    default:
      return SBF_SOURCE_ALPHA;
    }
}


// This is also unused
static CompareFunction getTestMode(int mode)
{
  switch(mode)
    {
    case 0: return CMPF_ALWAYS_PASS;
    case 1: return CMPF_LESS;
    case 2: return CMPF_EQUAL;
    case 3: return CMPF_LESS_EQUAL;
    case 4: return CMPF_GREATER;
    case 5: return CMPF_NOT_EQUAL;
    case 6: return CMPF_GREATER_EQUAL;
    case 7: return CMPF_ALWAYS_FAIL;
    default:
      return CMPF_ALWAYS_PASS;
    }
}
*/


class NIFMaterialLoader {

static std::map<size_t,std::string> MaterialMap;

static void warn(const std::string &msg)
{
    std::cerr << "NIFMeshLoader: Warn: " << msg << std::endl;
}

static void fail(const std::string &msg)
{
    std::cerr << "NIFMeshLoader: Fail: "<< msg << std::endl;
    abort();
}


public:
static Ogre::String getMaterial(const Nif::NiTriShape *shape, const Ogre::String &name, const Ogre::String &group)
{
    Ogre::MaterialManager &matMgr = Ogre::MaterialManager::getSingleton();
    Ogre::MaterialPtr material = matMgr.getByName(name);
    if(!material.isNull())
        return name;

    Ogre::Vector3 ambient(1.0f);
    Ogre::Vector3 diffuse(1.0f);
    Ogre::Vector3 specular(0.0f);
    Ogre::Vector3 emissive(0.0f);
    float glossiness = 0.0f;
    float alpha = 1.0f;
    int alphaFlags = -1;
//    ubyte alphaTest = 0;
    Ogre::String texName;

    bool vertexColour = (shape->data->colors.size() != 0);

    // These are set below if present
    const Nif::NiTexturingProperty *t = NULL;
    const Nif::NiMaterialProperty *m = NULL;
    const Nif::NiAlphaProperty *a = NULL;

    // Scan the property list for material information
    const Nif::PropertyList &list = shape->props;
    for (size_t i = 0;i < list.length();i++)
    {
        // Entries may be empty
        if (list[i].empty()) continue;

        const Nif::Property *pr = list[i].getPtr();
        if (pr->recType == Nif::RC_NiTexturingProperty)
            t = static_cast<const Nif::NiTexturingProperty*>(pr);
        else if (pr->recType == Nif::RC_NiMaterialProperty)
            m = static_cast<const Nif::NiMaterialProperty*>(pr);
        else if (pr->recType == Nif::RC_NiAlphaProperty)
            a = static_cast<const Nif::NiAlphaProperty*>(pr);
        else
            warn("Skipped property type: "+pr->recName);
    }

    // Texture
    if (t && t->textures[0].inUse)
    {
        Nif::NiSourceTexture *st = t->textures[0].texture.getPtr();
        if (st->external)
        {
            /* Bethesda at some at some point converted all their BSA
             * textures from tga to dds for increased load speed, but all
             * texture file name references were kept as .tga.
             */
            static const char path[] = "textures\\";

            texName = path + st->filename;
            Ogre::String::size_type pos = texName.rfind('.');
            if(pos != Ogre::String::npos && texName.compare(pos, texName.length() - pos, ".dds") != 0)
            {
                // since we know all (GOTY edition or less) textures end
                // in .dds, we change the extension
                texName.replace(pos, texName.length(), ".dds");

                // if it turns out that the above wasn't true in all cases (not for vanilla, but maybe mods)
                // verify, and revert if false (this call succeeds quickly, but fails slowly)
                if(!Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(texName))
                    texName = path + st->filename;
            }
        }
        else warn("Found internal texture, ignoring.");
    }

    // Alpha modifiers
    if (a)
    {
        alphaFlags = a->flags;
//        alphaTest = a->data.threshold;
    }

    // Material
    if(m)
    {
        ambient = m->data.ambient;
        diffuse = m->data.diffuse;
        specular = m->data.specular;
        emissive = m->data.emissive;
        glossiness = m->data.glossiness;
        alpha = m->data.alpha;
    }

    Ogre::String matname = name;
    if (m || !texName.empty())
    {
        // Generate a hash out of all properties that can affect the material.
        size_t h = 0;
        boost::hash_combine(h, ambient.x);
        boost::hash_combine(h, ambient.y);
        boost::hash_combine(h, ambient.z);
        boost::hash_combine(h, diffuse.x);
        boost::hash_combine(h, diffuse.y);
        boost::hash_combine(h, diffuse.z);
        boost::hash_combine(h, specular.x);
        boost::hash_combine(h, specular.y);
        boost::hash_combine(h, specular.z);
        boost::hash_combine(h, emissive.x);
        boost::hash_combine(h, emissive.y);
        boost::hash_combine(h, emissive.z);
        boost::hash_combine(h, texName);
        boost::hash_combine(h, vertexColour);
        boost::hash_combine(h, alphaFlags);

        std::map<size_t,std::string>::iterator itr = MaterialMap.find(h);
        if (itr != MaterialMap.end())
        {
            // a suitable material exists already - use it
            return itr->second;
        }
        // not found, create a new one
        MaterialMap.insert(std::make_pair(h, matname));
    }

    // No existing material like this. Create a new one.
    sh::MaterialInstance* instance = sh::Factory::getInstance ().createMaterialInstance (matname, "openmw_objects_base");
    instance->setProperty ("ambient", sh::makeProperty<sh::Vector3> (
        new sh::Vector3(ambient.x, ambient.y, ambient.z)));

    instance->setProperty ("diffuse", sh::makeProperty<sh::Vector4> (
        new sh::Vector4(diffuse.x, diffuse.y, diffuse.z, alpha)));

    instance->setProperty ("specular", sh::makeProperty<sh::Vector4> (
        new sh::Vector4(specular.x, specular.y, specular.z, glossiness)));

    instance->setProperty ("emissive", sh::makeProperty<sh::Vector3> (
        new sh::Vector3(emissive.x, emissive.y, emissive.z)));

    instance->setProperty ("diffuseMap", sh::makeProperty(texName));

    if (vertexColour)
        instance->setProperty ("has_vertex_colour", sh::makeProperty<sh::BooleanValue>(new sh::BooleanValue(true)));

    // Add transparency if NiAlphaProperty was present
    if (alphaFlags != -1)
    {
        // The 237 alpha flags are by far the most common. Check
        // NiAlphaProperty in nif/property.h if you need to decode
        // other values. 237 basically means normal transparencly.
        if (alphaFlags == 237)
        {
            NifOverrides::TransparencyResult result = NifOverrides::Overrides::getTransparencyOverride(texName);
            if (result.first)
            {
                instance->setProperty("alpha_rejection",
                    sh::makeProperty<sh::StringValue>(new sh::StringValue("greater_equal " + boost::lexical_cast<std::string>(result.second))));
            }
            else
            {
                // Enable transparency
                instance->setProperty("scene_blend", sh::makeProperty<sh::StringValue>(new sh::StringValue("alpha_blend")));
                instance->setProperty("depth_write", sh::makeProperty<sh::StringValue>(new sh::StringValue("off")));
            }
        }
        else
            warn("Unhandled alpha setting for texture " + texName);
    }
    else
        instance->getMaterial ()->setShadowCasterMaterial ("openmw_shadowcaster_noalpha");

    // As of yet UNTESTED code from Chris:
    /*pass->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
    pass->setDepthFunction(Ogre::CMPF_LESS_EQUAL);
    pass->setDepthCheckEnabled(true);

    // Add transparency if NiAlphaProperty was present
    if (alphaFlags != -1)
    {
        std::cout << "Alpha flags set!" << endl;
        if ((alphaFlags&1))
        {
            pass->setDepthWriteEnabled(false);
            pass->setSceneBlending(getBlendFactor((alphaFlags>>1)&0xf),
                                   getBlendFactor((alphaFlags>>5)&0xf));
        }
        else
            pass->setDepthWriteEnabled(true);

        if ((alphaFlags>>9)&1)
            pass->setAlphaRejectSettings(getTestMode((alphaFlags>>10)&0x7),
                                         alphaTest);

        pass->setTransparentSortingEnabled(!((alphaFlags>>13)&1));
    }
*/

    return matname;
}

};
std::map<size_t,std::string> NIFMaterialLoader::MaterialMap;


class NIFMeshLoader : Ogre::ManualResourceLoader
{
    std::string mName;
    std::string mGroup;
    std::string mShapeName;
    std::string mMaterialName;
    std::string mSkelName;

    void warn(const std::string &msg)
    {
        std::cerr << "NIFMeshLoader: Warn: " << msg << std::endl;
    }

    void fail(const std::string &msg)
    {
        std::cerr << "NIFMeshLoader: Fail: "<< msg << std::endl;
        abort();
    }


    // Convert NiTriShape to Ogre::SubMesh
    void handleNiTriShape(Ogre::Mesh *mesh, Nif::NiTriShape const *shape)
    {
        Ogre::SkeletonPtr skel;
        const Nif::NiTriShapeData *data = shape->data.getPtr();
        const Nif::NiSkinInstance *skin = (shape->skin.empty() ? NULL : shape->skin.getPtr());
        std::vector<Ogre::Vector3> srcVerts = data->vertices;
        std::vector<Ogre::Vector3> srcNorms = data->normals;
        if(skin != NULL)
        {
            // Only set a skeleton when skinning. Unskinned meshes with a skeleton will be
            // explicitly attached later.
            mesh->setSkeletonName(mSkelName);

            // Get the skeleton resource, so vertices can be transformed into the bones' initial state.
            Ogre::SkeletonManager *skelMgr = Ogre::SkeletonManager::getSingletonPtr();
            skel = skelMgr->getByName(mSkelName);
            skel->touch();

            // Convert vertices and normals to bone space from bind position. It would be
            // better to transform the bones into bind position, but there doesn't seem to
            // be a reliable way to do that.
            std::vector<Ogre::Vector3> newVerts(srcVerts.size(), Ogre::Vector3(0.0f));
            std::vector<Ogre::Vector3> newNorms(srcNorms.size(), Ogre::Vector3(0.0f));

            const Nif::NiSkinData *data = skin->data.getPtr();
            const Nif::NodeList &bones = skin->bones;
            for(size_t b = 0;b < bones.length();b++)
            {
                Ogre::Bone *bone = skel->getBone(bones[b]->name);
                Ogre::Matrix4 mat, mat2;
                mat.makeTransform(data->bones[b].trafo.trans, Ogre::Vector3(data->bones[b].trafo.scale),
                                  Ogre::Quaternion(data->bones[b].trafo.rotation));
                mat2.makeTransform(bone->_getDerivedPosition(), bone->_getDerivedScale(),
                                   bone->_getDerivedOrientation());
                mat = mat2 * mat;

                const std::vector<Nif::NiSkinData::VertWeight> &weights = data->bones[b].weights;
                for(size_t i = 0;i < weights.size();i++)
                {
                    size_t index = weights[i].vertex;
                    float weight = weights[i].weight;

                    newVerts.at(index) += (mat*srcVerts[index]) * weight;
                    if(newNorms.size() > index)
                    {
                        Ogre::Vector4 vec4(srcNorms[index][0], srcNorms[index][1], srcNorms[index][2], 0.0f);
                        vec4 = mat*vec4 * weight;
                        newNorms[index] += Ogre::Vector3(&vec4[0]);
                    }
                }
            }

            srcVerts = newVerts;
            srcNorms = newNorms;
        }
        else if(mSkelName.length() == 0)
        {
            // No skinning and no skeleton, so just transform the vertices and
            // normals into position.
            Ogre::Matrix4 mat4 = shape->getWorldTransform();
            for(size_t i = 0;i < srcVerts.size();i++)
            {
                Ogre::Vector4 vec4(srcVerts[i].x, srcVerts[i].y, srcVerts[i].z, 1.0f);
                vec4 = mat4*vec4;
                srcVerts[i] = Ogre::Vector3(&vec4[0]);
            }
            for(size_t i = 0;i < srcNorms.size();i++)
            {
                Ogre::Vector4 vec4(srcNorms[i].x, srcNorms[i].y, srcNorms[i].z, 0.0f);
                vec4 = mat4*vec4;
                srcNorms[i] = Ogre::Vector3(&vec4[0]);
            }
        }

        // Set the bounding box first
        BoundsFinder bounds;
        bounds.add(&srcVerts[0][0], srcVerts.size());
        // No idea why this offset is needed. It works fine without it if the
        // vertices weren't transformed first, but otherwise it fails later on
        // when the object is being inserted into the scene.
        mesh->_setBounds(Ogre::AxisAlignedBox(bounds.minX()-0.5f, bounds.minY()-0.5f, bounds.minZ()-0.5f,
                                              bounds.maxX()+0.5f, bounds.maxY()+0.5f, bounds.maxZ()+0.5f));
        mesh->_setBoundingSphereRadius(bounds.getRadius());

        // This function is just one long stream of Ogre-barf, but it works
        // great.
        Ogre::HardwareBufferManager *hwBufMgr = Ogre::HardwareBufferManager::getSingletonPtr();
        Ogre::HardwareVertexBufferSharedPtr vbuf;
        Ogre::HardwareIndexBufferSharedPtr ibuf;
        Ogre::VertexBufferBinding *bind;
        Ogre::VertexDeclaration *decl;
        int nextBuf = 0;

        Ogre::SubMesh *sub = mesh->createSubMesh(shape->name);

        // Add vertices
        sub->useSharedVertices = false;
        sub->vertexData = new Ogre::VertexData();
        sub->vertexData->vertexStart = 0;
        sub->vertexData->vertexCount = srcVerts.size();

        decl = sub->vertexData->vertexDeclaration;
        bind = sub->vertexData->vertexBufferBinding;
        if(srcVerts.size())
        {
            vbuf = hwBufMgr->createVertexBuffer(Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3),
                                                srcVerts.size(), Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY,
                                                true);
            vbuf->writeData(0, vbuf->getSizeInBytes(), &srcVerts[0][0], true);

            decl->addElement(nextBuf, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
            bind->setBinding(nextBuf++, vbuf);
        }

        // Vertex normals
        if(srcNorms.size())
        {
            vbuf = hwBufMgr->createVertexBuffer(Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3),
                                                srcNorms.size(), Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY,
                                                true);
            vbuf->writeData(0, vbuf->getSizeInBytes(), &srcNorms[0][0], true);

            decl->addElement(nextBuf, 0, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
            bind->setBinding(nextBuf++, vbuf);
        }

        // Vertex colors
        const std::vector<Ogre::Vector4> &colors = data->colors;
        if(colors.size())
        {
            Ogre::RenderSystem* rs = Ogre::Root::getSingleton().getRenderSystem();
            std::vector<Ogre::RGBA> colorsRGB(colors.size());
            for(size_t i = 0;i < colorsRGB.size();i++)
            {
                Ogre::ColourValue clr(colors[i][0], colors[i][1], colors[i][2], colors[i][3]);
                rs->convertColourValue(clr, &colorsRGB[i]);
            }
            vbuf = hwBufMgr->createVertexBuffer(Ogre::VertexElement::getTypeSize(Ogre::VET_COLOUR),
                                                colorsRGB.size(), Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY,
                                                true);
            vbuf->writeData(0, vbuf->getSizeInBytes(), &colorsRGB[0], true);
            decl->addElement(nextBuf, 0, Ogre::VET_COLOUR, Ogre::VES_DIFFUSE);
            bind->setBinding(nextBuf++, vbuf);
        }

        // Texture UV coordinates
        size_t numUVs = data->uvlist.size();
        if(numUVs)
        {
            size_t elemSize = Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT2);
            vbuf = hwBufMgr->createVertexBuffer(elemSize, srcVerts.size()*numUVs,
                                                Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY, true);
            for(size_t i = 0;i < numUVs;i++)
            {
                const std::vector<Ogre::Vector2> &uvlist = data->uvlist[i];
                vbuf->writeData(i*srcVerts.size()*elemSize, elemSize*srcVerts.size(), &uvlist[0], true);
                decl->addElement(nextBuf, i*srcVerts.size()*elemSize, Ogre::VET_FLOAT2,
                                 Ogre::VES_TEXTURE_COORDINATES, i);
            }
            bind->setBinding(nextBuf++, vbuf);
        }

        // Triangle faces
        const std::vector<short> &srcIdx = data->triangles;
        if(srcIdx.size())
        {
            ibuf = hwBufMgr->createIndexBuffer(Ogre::HardwareIndexBuffer::IT_16BIT, srcIdx.size(),
                                               Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
            ibuf->writeData(0, ibuf->getSizeInBytes(), &srcIdx[0], true);
            sub->indexData->indexBuffer = ibuf;
            sub->indexData->indexCount = srcIdx.size();
            sub->indexData->indexStart = 0;
        }

        // Assign bone weights for this TriShape
        if(skin != NULL)
        {
            const Nif::NiSkinData *data = skin->data.getPtr();
            const Nif::NodeList &bones = skin->bones;
            for(size_t i = 0;i < bones.length();i++)
            {
                Ogre::VertexBoneAssignment boneInf;
                boneInf.boneIndex = skel->getBone(bones[i]->name)->getHandle();

                const std::vector<Nif::NiSkinData::VertWeight> &weights = data->bones[i].weights;
                for(size_t j = 0;j < weights.size();j++)
                {
                    boneInf.vertexIndex = weights[j].vertex;
                    boneInf.weight = weights[j].weight;
                    sub->addBoneAssignment(boneInf);
                }
            }
        }

        if(mMaterialName.length() > 0)
            sub->setMaterialName(mMaterialName);
    }

    bool findTriShape(Ogre::Mesh *mesh, Nif::Node const *node)
    {
        if(node->recType == Nif::RC_NiTriShape && mShapeName == node->name)
        {
            handleNiTriShape(mesh, dynamic_cast<Nif::NiTriShape const *>(node));
            return true;
        }

        Nif::NiNode const *ninode = dynamic_cast<Nif::NiNode const *>(node);
        if(ninode)
        {
            Nif::NodeList const &children = ninode->children;
            for(size_t i = 0;i < children.length();i++)
            {
                if(!children[i].empty())
                {
                    if(findTriShape(mesh, children[i].getPtr()))
                        return true;
                }
            }
        }
        return false;
    }


    typedef std::map<std::string,NIFMeshLoader> LoaderMap;
    static LoaderMap sLoaders;

public:
    NIFMeshLoader()
    { }
    NIFMeshLoader(const std::string &name, const std::string &group, const std::string skelName)
      : mName(name), mGroup(group), mSkelName(skelName)
    { }

    virtual void loadResource(Ogre::Resource *resource)
    {
        Ogre::Mesh *mesh = dynamic_cast<Ogre::Mesh*>(resource);
        assert(mesh && "Attempting to load a mesh into a non-mesh resource!");

        if(!mShapeName.length())
        {
            if(mSkelName.length() > 0)
                mesh->setSkeletonName(mSkelName);
            return;
        }

        Nif::NIFFile::ptr nif = Nif::NIFFile::create (mName);
        Nif::Node const *node = dynamic_cast<Nif::Node const *>(nif->getRecord(0));
        findTriShape(mesh, node);
    }

    void createMeshes(const Nif::Node *node, MeshPairList &meshes, int flags=0)
    {
        flags |= node->flags;

        // Marker objects: just skip the entire node
        /// \todo don't do this in the editor
        if (node->name.find("marker") != std::string::npos)
            return;

        Nif::ExtraPtr e = node->extra;
        while(!e.empty())
        {
            Nif::NiStringExtraData *sd;
            Nif::NiTextKeyExtraData *td;
            if((sd=dynamic_cast<Nif::NiStringExtraData*>(e.getPtr())) != NULL)
            {
                // String markers may contain important information
                // affecting the entire subtree of this obj
                if(sd->string == "MRK")
                {
                    // Marker objects. These are only visible in the
                    // editor.
                    flags |= 0x01;
                }
            }
            else if((td=dynamic_cast<Nif::NiTextKeyExtraData*>(e.getPtr())) != NULL)
            {
                // TODO: Read and store text keys somewhere
            }
            else
                warn("Unhandled extra data type "+e->recName);
            e = e->extra;
        }

        if(node->recType == Nif::RC_NiTriShape)
        {
            const Nif::NiTriShape *shape = dynamic_cast<const Nif::NiTriShape*>(node);

            Ogre::MeshManager &meshMgr = Ogre::MeshManager::getSingleton();
            std::string fullname = mName+"@shape="+shape->name;
            if(mSkelName.length() > 0 && mName != mSkelName)
                fullname += "@skel="+mSkelName;

            Misc::StringUtils::toLower(fullname);
            Ogre::MeshPtr mesh = meshMgr.getByName(fullname);
            if(mesh.isNull())
            {
                NIFMeshLoader *loader = &sLoaders[fullname];
                *loader = *this;
                if(!(flags&0x01)) // Not hidden
                {
                    loader->mShapeName = shape->name;
                    loader->mMaterialName = NIFMaterialLoader::getMaterial(shape, fullname, mGroup);
                }

                mesh = meshMgr.createManual(fullname, mGroup, loader);
                mesh->setAutoBuildEdgeLists(false);
            }

            meshes.push_back(std::make_pair(mesh->getName(), shape->name));
        }
        else if(node->recType != Nif::RC_NiNode && node->recType != Nif::RC_RootCollisionNode &&
                node->recType != Nif::RC_NiRotatingParticles)
            warn("Unhandled mesh node type: "+node->recName);

        const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(node);
        if(ninode)
        {
            const Nif::NodeList &children = ninode->children;
            for(size_t i = 0;i < children.length();i++)
            {
                if(!children[i].empty())
                    createMeshes(children[i].getPtr(), meshes, flags);
            }
        }
    }
};
NIFMeshLoader::LoaderMap NIFMeshLoader::sLoaders;


typedef std::map<std::string,MeshPairList> MeshPairMap;
static MeshPairMap sMeshPairMap;

MeshPairList NIFLoader::load(std::string name, std::string skelName, const std::string &group)
{
    Misc::StringUtils::toLower(name);
     Misc::StringUtils::toLower(skelName);

    MeshPairMap::const_iterator meshiter = sMeshPairMap.find(name+"@skel="+skelName);
    if(meshiter != sMeshPairMap.end())
        return meshiter->second;

    MeshPairList &meshes = sMeshPairMap[name+"@skel="+skelName];
    Nif::NIFFile::ptr pnif = Nif::NIFFile::create (name);
    Nif::NIFFile &nif = *pnif.get ();
    if (nif.numRecords() < 1)
    {
        nif.warn("Found no records in NIF.");
        return meshes;
    }

    // The first record is assumed to be the root node
    Nif::Record const *r = nif.getRecord(0);
    assert(r != NULL);

    Nif::Node const *node = dynamic_cast<Nif::Node const *>(r);
    if(node == NULL)
    {
        nif.warn("First record in file was not a node, but a "+
                 r->recName+". Skipping file.");
        return meshes;
    }

    NIFSkeletonLoader skelldr;
    bool hasSkel = skelldr.createSkeleton(name, group, node);

    NIFMeshLoader meshldr(name, group, (hasSkel ? skelName : std::string()));
    meshldr.createMeshes(node, meshes);

    return meshes;
}

EntityList NIFLoader::createEntities(Ogre::SceneNode *parent, TextKeyMap *textkeys, const std::string &name, const std::string &group)
{
    EntityList entitylist;

    MeshPairList meshes = load(name, name, group);
    if(meshes.size() == 0)
        return entitylist;

    Ogre::SceneManager *sceneMgr = parent->getCreator();
    for(size_t i = 0;i < meshes.size();i++)
    {
        entitylist.mEntities.push_back(sceneMgr->createEntity(meshes[i].first));
        Ogre::Entity *entity = entitylist.mEntities.back();
        if(!entitylist.mSkelBase && entity->hasSkeleton())
            entitylist.mSkelBase = entity;
    }

    if(entitylist.mSkelBase && textkeys)
    {
        // Would be nice if Ogre::SkeletonInstance allowed access to the 'master' Ogre::SkeletonPtr.
        Ogre::SkeletonManager &skelMgr = Ogre::SkeletonManager::getSingleton();
        Ogre::SkeletonPtr skel = skelMgr.getByName(entitylist.mSkelBase->getSkeleton()->getName());
        Ogre::Skeleton::BoneIterator iter = skel->getBoneIterator();
        while(iter.hasMoreElements())
        {
            Ogre::Bone *bone = iter.getNext();
            const Ogre::Any &data = bone->getUserObjectBindings().getUserAny("TextKeyExtraData");
            if(!data.isEmpty())
            {
                *textkeys = Ogre::any_cast<TextKeyMap>(data);
                break;
            }
        }
    }

    if(entitylist.mSkelBase)
    {
        parent->attachObject(entitylist.mSkelBase);
        for(size_t i = 0;i < entitylist.mEntities.size();i++)
        {
            Ogre::Entity *entity = entitylist.mEntities[i];
            if(entity != entitylist.mSkelBase && entity->hasSkeleton())
            {
                entity->shareSkeletonInstanceWith(entitylist.mSkelBase);
                parent->attachObject(entity);
            }
            else if(entity != entitylist.mSkelBase)
                entitylist.mSkelBase->attachObjectToBone(meshes[i].second, entity);
        }
    }
    else
    {
        for(size_t i = 0;i < entitylist.mEntities.size();i++)
            parent->attachObject(entitylist.mEntities[i]);
    }

    return entitylist;
}

EntityList NIFLoader::createEntities(Ogre::Entity *parent, const std::string &bonename,
                                     Ogre::SceneNode *parentNode,
                                     const std::string &name,
                                     const std::string &group)
{
    EntityList entitylist;

    MeshPairList meshes = load(name, parent->getMesh()->getSkeletonName(), group);
    if(meshes.size() == 0)
        return entitylist;

    Ogre::SceneManager *sceneMgr = parentNode->getCreator();
    std::string filter = "tri "+bonename;
    std::transform(filter.begin()+4, filter.end(), filter.begin()+4, ::tolower);
    for(size_t i = 0;i < meshes.size();i++)
    {
        Ogre::Entity *ent = sceneMgr->createEntity(meshes[i].first);
        if(ent->hasSkeleton())
        {
             Misc::StringUtils::toLower(meshes[i].second);

            if(meshes[i].second.length() < filter.length() ||
               meshes[i].second.compare(0, filter.length(), filter) != 0)
            {
                sceneMgr->destroyEntity(ent);
                continue;
            }
            if(!entitylist.mSkelBase)
                entitylist.mSkelBase = ent;
        }
        entitylist.mEntities.push_back(ent);
    }

    Ogre::Vector3 scale(1.0f);
    if(bonename.find("Left") != std::string::npos)
        scale.x *= -1.0f;

    if(entitylist.mSkelBase)
    {
        entitylist.mSkelBase->shareSkeletonInstanceWith(parent);
        parentNode->attachObject(entitylist.mSkelBase);
        for(size_t i = 0;i < entitylist.mEntities.size();i++)
        {
            Ogre::Entity *entity = entitylist.mEntities[i];
            if(entity != entitylist.mSkelBase && entity->hasSkeleton())
            {
                entity->shareSkeletonInstanceWith(parent);
                parentNode->attachObject(entity);
            }
            else if(entity != entitylist.mSkelBase)
            {
                Ogre::TagPoint *tag = parent->attachObjectToBone(bonename, entity);
                tag->setScale(scale);
            }
        }
    }
    else
    {
        for(size_t i = 0;i < entitylist.mEntities.size();i++)
        {
            Ogre::TagPoint *tag = parent->attachObjectToBone(bonename, entitylist.mEntities[i]);
            tag->setScale(scale);
        }
    }

    return entitylist;
}


/* More code currently not in use, from the old D source. This was
   used in the first attempt at loading NIF meshes, where each submesh
   in the file was given a separate bone in a skeleton. Unfortunately
   the OGRE skeletons can't hold more than 256 bones, and some NIFs go
   way beyond that. The code might be of use if we implement animated
   submeshes like this (the part of the NIF that is animated is
   usually much less than the entire file, but the method might still
   not be water tight.)

// Insert a raw RGBA image into the texture system.
extern "C" void ogre_insertTexture(char* name, uint32_t width, uint32_t height, void *data)
{
  TexturePtr texture = TextureManager::getSingleton().createManual(
      name,         // name
      "General",    // group
      TEX_TYPE_2D,      // type
      width, height,    // width & height
      0,                // number of mipmaps
      PF_BYTE_RGBA,     // pixel format
      TU_DEFAULT);      // usage; should be TU_DYNAMIC_WRITE_ONLY_DISCARDABLE for
                        // textures updated very often (e.g. each frame)

  // Get the pixel buffer
  HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();

  // Lock the pixel buffer and get a pixel box
  pixelBuffer->lock(HardwareBuffer::HBL_NORMAL); // for best performance use HBL_DISCARD!
  const PixelBox& pixelBox = pixelBuffer->getCurrentLock();

  void *dest = pixelBox.data;

  // Copy the data
  memcpy(dest, data, width*height*4);

  // Unlock the pixel buffer
  pixelBuffer->unlock();
}


*/

} // nsmaepace NifOgre
