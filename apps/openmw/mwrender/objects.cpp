#include "objects.hpp"
#include <OgreSceneNode.h>
#include <components/nifogre/ogre_nif_loader.hpp>

using namespace Ogre;
using namespace MWRender;


bool Objects::lightConst = false;
float Objects::lightConstValue = 0.0f;

bool Objects::lightLinear = true;
int Objects::lightLinearMethod = 1;
float Objects::lightLinearValue = 3;
float Objects::lightLinearRadiusMult = 1;

bool Objects::lightQuadratic = false;
int Objects::lightQuadraticMethod = 2;
float Objects::lightQuadraticValue = 16;
float Objects::lightQuadraticRadiusMult = 1;

bool Objects::lightOutQuadInLin = false;

int Objects::uniqueID = 0;

void Objects::setMwRoot(Ogre::SceneNode* root){
    mMwRoot = root;
}
void Objects::insertBegin (const MWWorld::Ptr& ptr, bool enabled, bool static_){
    Ogre::SceneNode* root = mMwRoot;
    Ogre::SceneNode* cellnode;
    if(mCellSceneNodes.find(ptr.getCell()) == mCellSceneNodes.end())
    {
        //Create the scenenode and put it in the map
        cellnode = root->createChildSceneNode();
        mCellSceneNodes[ptr.getCell()] = cellnode;
    }
    else
    {
        cellnode = mCellSceneNodes[ptr.getCell()];
    }

    Ogre::SceneNode* insert = cellnode->createChildSceneNode();
    const float *f = ptr.getRefData().getPosition().pos;
    insert->setPosition(f[0], f[1], f[2]);
    insert->setScale(ptr.getCellRef().scale, ptr.getCellRef().scale, ptr.getCellRef().scale);

    // Convert MW rotation to a quaternion:
    f = ptr.getCellRef().pos.rot;

    // Rotate around X axis
    Quaternion xr(Radian(-f[0]), Vector3::UNIT_X);

    // Rotate around Y axis
    Quaternion yr(Radian(-f[1]), Vector3::UNIT_Y);

    // Rotate around Z axis
    Quaternion zr(Radian(-f[2]), Vector3::UNIT_Z);

   // Rotates first around z, then y, then x
    insert->setOrientation(xr*yr*zr);
    if (!enabled)
         insert->setVisible (false);
    ptr.getRefData().setBaseNode(insert);
    isStatic = static_;


}
void Objects::insertMesh (const MWWorld::Ptr& ptr, const std::string& mesh){
    Ogre::SceneNode* insert = ptr.getRefData().getBaseNode();
    assert(insert);

    NifOgre::NIFLoader::load(mesh);
    Entity *ent = mRend.getScene()->createEntity(mesh);

    if(!isStatic)
    {
        insert->attachObject(ent);
    }
    else
    {
        Ogre::StaticGeometry* sg = 0;
        if(mSG.find(ptr.getCell()) == mSG.end())
        {
            uniqueID = uniqueID +1;
            sg = mRend.getScene()->createStaticGeometry( "sg" + Ogre::StringConverter::toString(uniqueID));
            //Create the scenenode and put it in the map
            mSG[ptr.getCell()] = sg;
        }
        else
        {
            sg = mSG[ptr.getCell()];
        }

        sg->addEntity(ent,insert->_getDerivedPosition(),insert->_getDerivedOrientation(),insert->_getDerivedScale());
        sg->setRegionDimensions(Ogre::Vector3(100000,10000,100000));


        mRend.getScene()->destroyEntity(ent);
    }


}
void Objects::insertLight (const MWWorld::Ptr& ptr, float r, float g, float b, float radius){
    Ogre::SceneNode* insert = mRend.getScene()->getSceneNode(ptr.getRefData().getHandle());
    assert(insert);
    Ogre::Light *light = mRend.getScene()->createLight();
    light->setDiffuseColour (r, g, b);

    float cval=0.0f, lval=0.0f, qval=0.0f;

    if(lightConst)
         cval = lightConstValue;
  if(!lightOutQuadInLin)
  {
    if(lightLinear)
      radius *= lightLinearRadiusMult;
    if(lightQuadratic)
      radius *= lightQuadraticRadiusMult;

    if(lightLinear)
      lval = lightLinearValue / pow(radius, lightLinearMethod);
    if(lightQuadratic)
      qval = lightQuadraticValue / pow(radius, lightQuadraticMethod);
  }
  else
  {
    // FIXME:
    // Do quadratic or linear, depending if we're in an exterior or interior
    // cell, respectively. Ignore lightLinear and lightQuadratic.
  }

  light->setAttenuation(10*radius, cval, lval, qval);

  insert->attachObject(light);
}

bool Objects::deleteObject (const MWWorld::Ptr& ptr)
{
    if (Ogre::SceneNode *base = ptr.getRefData().getBaseNode())
    {
        Ogre::SceneNode *parent = base->getParentSceneNode();

        for (std::map<MWWorld::Ptr::CellStore *, Ogre::SceneNode *>::const_iterator iter (
            mCellSceneNodes.begin()); iter!=mCellSceneNodes.end(); ++iter)
            if (iter->second==parent)
            {
                base->removeAndDestroyAllChildren();
                mRend.getScene()->destroySceneNode (base);
                ptr.getRefData().setBaseNode (0);
                return true;
            }

        return false;
    }

    return true;
}

void Objects::removeCell(MWWorld::Ptr::CellStore* store){
    if(mCellSceneNodes.find(store) != mCellSceneNodes.end())
    {
        Ogre::SceneNode* base = mCellSceneNodes[store];
        base->removeAndDestroyAllChildren();
        mCellSceneNodes.erase(store);
        mRend.getScene()->destroySceneNode(base);
        base = 0;
    }


    if(mSG.find(store) != mSG.end())
    {
        Ogre::StaticGeometry* sg = mSG[store];
        mSG.erase(store);
        mRend.getScene()->destroyStaticGeometry (sg);
        sg = 0;
    }
}
void Objects::buildStaticGeometry(ESMS::CellStore<MWWorld::RefData>& cell){
    if(mSG.find(&cell) != mSG.end())
    {
        Ogre::StaticGeometry* sg = mSG[&cell];
        sg->build();
    }
}
