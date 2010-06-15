// Place a mesh in the 3D scene graph, at the given
// location/scale. Returns a node pointer to the inserted object.
NodePtr placeObject(MeshIndex mesh, Placement *pos, float scale,
                    bool collide)
{
  // Get a scene node for this object. mesh.getNode() will either load
  // it from file or BSA archive, or give us a handle if it is already
  // loaded.

  // This must be called BEFORE UniqueName below, because it might
  // possibly use UniqueName itself and overwrite the data
  // there. (That was a fun bug to track down...) Calling getNode()
  // will load the mesh if it is not already loaded.
  NodePtr node = mesh.getNode();

  // First, convert the Morrowind rotation to a quaternion
  float[4] quat;
  ogre_mwToQuaternion(pos.rotation.ptr, quat.ptr);

  // Insert a mesh copy into Ogre.
  char[] name = UniqueName(mesh.getName);
  node = ogre_insertNode(node, name.ptr, pos.position.ptr,
                         quat.ptr, scale);

  // Insert a collision shape too, if the mesh has one.
  if(collide && mesh.shape !is null)
    bullet_insertStatic(mesh.shape, pos.position.ptr,
                        quat.ptr, scale);

  return node;
}

void setAmbient(Color amb, Color sun, Color fog, float density)
{
  ogre_setAmbient(amb.red/255.0, amb.green/255.0, amb.blue/255.0,
		 sun.red/255.0, sun.green/255.0, sun.blue/255.0);

  // Calculate fog distance
  // TODO: Mesh with absolute view distance later
  float fhigh = 4500 + 9000*(1-density);
  float flow = 200 + 2000*(1-density);

  ogre_setFog(fog.red/255.0, fog.green/255.0, fog.blue/255.0, 200, fhigh);
}

// Gives the placement of an item in the scene (position and
// orientation). It must have this exact structure since we also use
// it when reading ES files.
align(1) struct Placement
{
  float[3] position;
  float[3] rotation;
}
static assert(Placement.sizeof == 4*6);
