#include "actors.hpp"
#include <OgreSceneNode.h>
#include <components/nifogre/ogre_nif_loader.hpp>



using namespace Ogre;
using namespace MWRender;
using namespace NifOgre;

void Actors::setMwRoot(Ogre::SceneNode* root){
    mMwRoot = root;
}
void Actors::insertNPC(const MWWorld::Ptr& ptr){
        insertBegin(ptr, true, true);
          NpcAnimation* anim = new MWRender::NpcAnimation(ptr, mEnvironment, mRend);
    mAllActors.push_back(anim);
		
		
}
void Actors::insertBegin (const MWWorld::Ptr& ptr, bool enabled, bool static_){
    Ogre::SceneNode* cellnode;
    if(mCellSceneNodes.find(ptr.getCell()) == mCellSceneNodes.end())
    {
        //Create the scenenode and put it in the map
        cellnode = mMwRoot->createChildSceneNode();
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


}
void Actors::insertCreature (const MWWorld::Ptr& ptr){
    insertBegin(ptr, true, true);
   CreatureAnimation* anim = new MWRender::CreatureAnimation(ptr, mEnvironment, mRend);
    mAllActors.push_back(anim);
   //mAllActors.push_back(&anim);
}

bool Actors::deleteObject (const MWWorld::Ptr& ptr)
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

void Actors::removeCell(MWWorld::Ptr::CellStore* store){
    if(mCellSceneNodes.find(store) != mCellSceneNodes.end())
    {
        Ogre::SceneNode* base = mCellSceneNodes[store];
        base->removeAndDestroyAllChildren();
        mCellSceneNodes.erase(store);
        mRend.getScene()->destroySceneNode(base);
        base = 0;
    }
    
}