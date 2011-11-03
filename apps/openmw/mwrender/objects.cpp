#include "objects.hpp"
#include <OgreSceneNode.h>

using namespace MWRender;

void Objects::insertBegin (const MWWorld::Ptr& ptr, bool enabled, bool static_){
	Ogre::SceneNode* root = rend.getScene()->getRootSceneNode();
	Ogre::SceneNode* cellnode;
	if(cellSceneNodes.find(ptr.getCell()) == cellSceneNodes.end())
	{
		//Create the scenenode and put it in the map
		cellnode = root->createChildSceneNode();
		cellSceneNodes[ptr.getCell()] = cellnode;
		//assert(!cellnode->getChildIterator()->begin());  //Is this right?
	}
	else
	{
		cellnode = (cellSceneNodes.find(ptr.getCell()))->second;
	}
	Ogre::SceneNode* insert = cellnode->createChildSceneNode();
	
    ptr.getRefData().setBaseNode(insert);
	isStatic = static_;

    
}
void Objects::insertMesh (const MWWorld::Ptr& ptr, const std::string& mesh){

}
void Objects::insertLight (const MWWorld::Ptr& ptr, float r, float g, float b, float radius){

}

