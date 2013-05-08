#ifndef COMPONENTS_NIFOGRE_SKELETON_HPP
#define COMPONENTS_NIFOGRE_SKELETON_HPP

#include <iostream>
#include <string>
#include <cassert>

#include <OgreResource.h>

#include "ogrenifloader.hpp"

namespace Nif
{
    class NiTextKeyExtraData;
    class Node;
    class NiKeyframeController;
}

namespace NifOgre
{

/** Manual resource loader for NIF skeletons. This is the main class
    responsible for translating the internal NIF skeleton structure into
    something Ogre can use (includes animations and node TextKeyData).
 */
class NIFSkeletonLoader : public Ogre::ManualResourceLoader
{
    static void warn(const std::string &msg)
    {
        std::cerr << "NIFSkeletonLoader: Warn: " << msg << std::endl;
    }

    static void fail(const std::string &msg)
    {
        std::cerr << "NIFSkeletonLoader: Fail: "<< msg << std::endl;
        abort();
    }

    void buildBones(Ogre::Skeleton *skel, const Nif::Node *node, Ogre::Bone *parent=NULL);

    static bool needSkeleton(const Nif::Node *node);

    // Lookup to retrieve an Ogre bone handle for a given Nif record index
    std::map<int,int> mNifToOgreHandleMap;

    typedef std::map<std::string,NIFSkeletonLoader> LoaderMap;
    static LoaderMap sLoaders;

public:
    void loadResource(Ogre::Resource *resource);

    static Ogre::SkeletonPtr createSkeleton(const std::string &name, const std::string &group, const Nif::Node *node);

    // Looks up an Ogre Bone handle ID from a NIF's record index. Should only
    // be used when the bone name is insufficient as this is a relatively slow
    // lookup
    static int lookupOgreBoneHandle(const std::string &nifname, int idx);
};

}

#endif
