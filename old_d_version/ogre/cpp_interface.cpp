// Copy a scene node and all its children
void cloneNode(SceneNode *from, SceneNode *to, char* name)
{
  to->setPosition(from->getPosition());
  to->setOrientation(from->getOrientation());
  to->setScale(from->getScale());

  SceneNode::ObjectIterator it = from->getAttachedObjectIterator();
  while(it.hasMoreElements())
    {
      // We can't handle non-entities.
      Entity *e = dynamic_cast<Entity*> (it.getNext());
      if(e)
        {
          e = e->clone(String(name) + ":" + e->getName());
          to->attachObject(e);
        }
    }

  // Recursively clone all child nodes
  SceneNode::ChildNodeIterator it2 = from->getChildIterator();
  while(it2.hasMoreElements())
    {
      cloneNode((SceneNode*)it2.getNext(), to->createChildSceneNode(), name);
    }
}

// Supposed to insert a copy of the node, for now it just inserts the
// actual node.
extern "C" SceneNode *ogre_insertNode(SceneNode *base, char* name,
                                      float *pos, float *quat,
                                      float scale)
{
  //std::cout << "ogre_insertNode(" << name << ")\n";
  SceneNode *node = mwRoot->createChildSceneNode(name);

  // Make a copy of the node
  cloneNode(base, node, name);

  // Apply transformations
  node->setPosition(pos[0], pos[1], pos[2]);
  node->setOrientation(quat[0], quat[1], quat[2], quat[3]);

  node->setScale(scale, scale, scale);

  return node;
}

// Get the world transformation of a node (the total transformation of
// this node and all parent nodes). Return it as a translation
// (3-vector) and a rotation / scaling part (3x3 matrix)
extern "C" void ogre_getWorldTransform(SceneNode *node,
                                       float *trans, // Storage for translation
                                       float *matrix)// For 3x3 matrix
{
  // Get the world transformation first
  Matrix4 trafo;
  node->getWorldTransforms(&trafo);

  // Extract the translation part and pass it to the caller
  Vector3 tr = trafo.getTrans();
  trans[0] = tr[0];
  trans[1] = tr[1];
  trans[2] = tr[2];

  // Next extract the matrix
  Matrix3 mat;
  trafo.extract3x3Matrix(mat);
  matrix[0] = mat[0][0];
  matrix[1] = mat[0][1];
  matrix[2] = mat[0][2];
  matrix[3] = mat[1][0];
  matrix[4] = mat[1][1];
  matrix[5] = mat[1][2];
  matrix[6] = mat[2][0];
  matrix[7] = mat[2][1];
  matrix[8] = mat[2][2];
}

// Create the water plane. It doesn't really resemble "water" yet
// though.
extern "C" void ogre_createWater(float level)
{
    // Create a plane aligned with the xy-plane.
    MeshManager::getSingleton().createPlane("water",
           ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
           Plane(Vector3::UNIT_Z, level),
	   150000,150000
	   );
    Entity *ent = mSceneMgr->createEntity( "WaterEntity", "water" );
    mwRoot->createChildSceneNode()->attachObject(ent);
    ent->setCastShadows(false);
}

extern "C" SceneNode *ogre_getDetachedNode()
{
  SceneNode *node = mwRoot->createChildSceneNode();
  mwRoot->removeChild(node);
  return node;
}

extern "C" SceneNode* ogre_createNode(
		char *name,
		float *trafo,
		SceneNode *parent,
		int32_t noRot)
{
  //std::cout << "ogre_createNode(" << name << ")";
  SceneNode *node = parent->createChildSceneNode(name);
  //std::cout << " ... done\n";

  // First is the translation vector

  // TODO should be "if(!noRot)" only for exterior cells!? Yay for
  // consistency. Apparently, the displacement of the base node in NIF
  // files must be ignored for meshes in interior cells, but not for
  // exterior cells. Or at least that's my hypothesis, and it seems
  // work. There might be some other NIF trickery going on though, you
  // never know when you're reverse engineering someone else's file
  // format. We will handle this later.
  if(!noRot)
    node->setPosition(trafo[0], trafo[1], trafo[2]);

  // Then a 3x3 rotation matrix.
  if(!noRot)
    node->setOrientation(Quaternion(Matrix3(trafo[3], trafo[4], trafo[5],
					    trafo[6], trafo[7], trafo[8],
					    trafo[9], trafo[10], trafo[11]
					    )));

  // Scale is at the end
  node->setScale(trafo[12],trafo[12],trafo[12]);

  return node;
}
