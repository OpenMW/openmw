#ifndef _GAME_RENDER_ANIMATION_H
#define _GAME_RENDER_ANIMATION_H

#include <vector>

#include <components/nifogre/ogre_nif_loader.hpp>
#include <openengine/ogre/renderer.hpp>
#include "../mwworld/actiontalk.hpp"
#include <components/nif/node.hpp>
#include <openengine/bullet/physic.hpp>




namespace MWRender {

struct PosAndRot {
    Ogre::Quaternion vecRot;
    Ogre::Vector3 vecPos;
};

class Animation {
protected:
    Ogre::SceneNode* mInsert;
    OEngine::Render::OgreRenderer &mRend;
    static std::map<std::string, int> sUniqueIDs;

    float mTime;
    float mStartTime;
    float mStopTime;
    int mAnimate;
    //Represents a rotation index for each bone
    std::vector<int>mRindexI;
    //Represents a translation index for each bone
    std::vector<int>mTindexI;

    //Only shapes with morphing data will use a shape number
    int mShapeNumber;
    std::vector<std::vector<int> > mShapeIndexI;

    std::vector<Nif::NiKeyframeData>* mTransformations;
    std::map<std::string,float>* mTextmappings;
    NifOgre::EntityList mEntityList;
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
