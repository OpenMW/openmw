#ifndef _GAME_RENDER_ANIMATION_H
#define _GAME_RENDER_ANIMATION_H
#include <components/nif/data.hpp>
#include <openengine/ogre/renderer.hpp>
#include "../mwworld/refdata.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontalk.hpp"
#include <components/nif/node.hpp>
#include <map>
#include <openengine/bullet/physic.hpp>




namespace MWRender {

struct PosAndRot {
    Ogre::Quaternion vecRot;
    Ogre::Vector3 vecPos;
};

class Animation {
protected:
    Ogre::SceneNode* insert;
    OEngine::Render::OgreRenderer &mRend;
    static std::map<std::string, int> mUniqueIDs;

    float time;
    float startTime;
    float stopTime;
    int animate;
    //Represents a rotation index for each bone
    std::vector<int>rindexI;
    //Represents a translation index for each bone
    std::vector<int>tindexI;

    //Only shapes with morphing data will use a shape number
    int shapeNumber;
    std::vector<std::vector<int> > shapeIndexI;

    std::vector<Nif::NiKeyframeData>* transformations;
    std::map<std::string,float>* textmappings;
    Ogre::Entity* base;
    void handleAnimationTransforms();
    bool timeIndex( float time, const std::vector<float> & times, int & i, int & j, float & x );

public:
    Animation(OEngine::Render::OgreRenderer& _rend);
    virtual void runAnimation(float timepassed) = 0;
    void startScript(std::string groupname, int mode, int loops);
    void stopScript();


    virtual ~Animation();
};

}
#endif
