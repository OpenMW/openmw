/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.snaptoad.com/

  This file (cpp_interface.cpp) is part of the OpenMW package.

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

//-----------------------------------------------------------------------
//               E X P O R T E D    F U N C T I O N S
//-----------------------------------------------------------------------

extern "C" void cpp_cleanup()
{
  // Kill the input systems. This will reset input options such as key
  // repetition.
  mInputManager->destroyInputObject(mKeyboard);
  mInputManager->destroyInputObject(mMouse);
  OIS::InputManager::destroyInputSystem(mInputManager);

  // Kill OGRE
  if (mRoot)
    {
      delete mRoot;
      mRoot = 0;
    }
}

extern "C" int32_t cpp_configure(
                   int32_t showConfig, // Do we show the config dialogue?
                   char *plugincfg     // Name of 'plugin.cfg' file
                   )
{
  mRoot = new Root(plugincfg);

  // Add the BSA archive manager before reading the config file.
  ArchiveManager::getSingleton().addArchiveFactory( &mBSAFactory );

  /* The only entry we use from resources.cfg is the "BSA=internal"
     entry, which we can put in manually.

  // Load resource paths from config file
  ConfigFile cf;
  cf.load("resources.cfg");

  // Go through all sections & settings in the file
  ConfigFile::SectionIterator seci = cf.getSectionIterator();

  String secName, typeName, archName;
  while (seci.hasMoreElements())
    {
      secName = seci.peekNextKey();
      ConfigFile::SettingsMultiMap *settings = seci.getNext();
      ConfigFile::SettingsMultiMap::iterator i;
      for (i = settings->begin(); i != settings->end(); ++i)
	{
	  typeName = i->first;
	  archName = i->second;
	  ResourceGroupManager::getSingleton().addResourceLocation(
			     archName, typeName, secName);
	}
    }
  */
  ResourceGroupManager::getSingleton().
    addResourceLocation("internal", "BSA", "General");

  // Show the configuration dialog and initialise the system, if the
  // showConfig parameter is specified. The settings are stored in
  // ogre.cfg. If showConfig is false, the settings are assumed to
  // already exist in ogre.cfg.
  int result;
  if(showConfig)
    result = mRoot->showConfigDialog();
  else
    result = mRoot->restoreConfig();

  return !result;
}

// Initialize window. This will create and show the actual window.
extern "C" void cpp_initWindow()
{
  std::cout << "cpp_initWindow()\n";

  // Initialize OGRE.
  mWindow = mRoot->initialise(true);

  // Set up the input system

  using namespace OIS;

  size_t windowHnd;
  mWindow->getCustomAttribute("WINDOW", &windowHnd);

  std::ostringstream windowHndStr;
  ParamList pl;	

  windowHndStr << windowHnd;
  pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));

  // Non-exclusive mouse and keyboard input
  /*
#if defined OIS_WIN32_PLATFORM
    pl.insert(std::make_pair(std::string("w32_mouse"),
                             std::string("DISCL_FOREGROUND" )));
    pl.insert(std::make_pair(std::string("w32_mouse"),
                             std::string("DISCL_NONEXCLUSIVE")));
    pl.insert(std::make_pair(std::string("w32_keyboard"),
                             std::string("DISCL_FOREGROUND")));
    pl.insert(std::make_pair(std::string("w32_keyboard"),
                             std::string("DISCL_NONEXCLUSIVE")));
#elif defined OIS_LINUX_PLATFORM
    pl.insert(std::make_pair(std::string("x11_mouse_grab"),
                             std::string("true")));
    pl.insert(std::make_pair(std::string("x11_mouse_hide"),
                             std::string("true")));
    pl.insert(std::make_pair(std::string("x11_keyboard_grab"),
                             std::string("true")));
    pl.insert(std::make_pair(std::string("XAutoRepeatOn"),
                             std::string("false")));
#endif
  */

  mInputManager = InputManager::createInputSystem( pl );

  const bool bufferedKeys = true;
  const bool bufferedMouse = true;

  // Create all devices
  mKeyboard = static_cast<Keyboard*>(mInputManager->createInputObject
                                     ( OISKeyboard, bufferedKeys ));
  mMouse = static_cast<Mouse*>(mInputManager->createInputObject
                               ( OISMouse, bufferedMouse ));

  unsigned int width, height, depth;
  int left, top;
  mWindow->getMetrics(width, height, depth, left, top);

  // Set mouse region
  const MouseState &ms = mMouse->getMouseState();
  ms.width = width;
  ms.height = height;

  // Register the input listener
  mKeyboard -> setEventCallback( &mInput );
  mMouse    -> setEventCallback( &mInput );

  std::cout << "cpp_initWindow finished\n";
}

// Make a scene, set the given ambient light
extern "C" void cpp_makeScene()
{
  // Get the SceneManager, in this case a generic one
  mSceneMgr = mRoot->createSceneManager(ST_GENERIC);

  // Create the camera
  mCamera = mSceneMgr->createCamera("PlayerCam");

  mCamera->setNearClipDistance(5);

  // Create one viewport, entire window
  vp = mWindow->addViewport(mCamera);
  // Give the backround a healthy shade of green
  vp->setBackgroundColour(ColourValue(0,0.1,0));

  // Alter the camera aspect ratio to match the viewport
  mCamera->setAspectRatio(Real(vp->getActualWidth()) / Real(vp->getActualHeight()));

  // Set default mipmap level (NB some APIs ignore this)
  TextureManager::getSingleton().setDefaultNumMipmaps(5);

  // Load resources
  ResourceGroupManager::getSingleton().initialiseAllResourceGroups();

  // Add the frame listener
  mRoot->addFrameListener(&mFrameListener);

  // Turn the entire scene (represented by the 'root' node) -90
  // degrees around the x axis. This makes Z go upwards, and Y go into
  // the screen. This is the orientation that Morrowind uses, and it
  // automagically makes everything work as it should.
  SceneNode *rt = mSceneMgr->getRootSceneNode();
  root = rt->createChildSceneNode();
  root->pitch(Degree(-90));
}

// Create a sky dome. Currently disabled since we aren't including the
// Ogre example data (which has the sky material.)
extern "C" void cpp_makeSky()
{
  //mSceneMgr->setSkyDome( true, "Examples/CloudySky", 5, 8 );
}

extern "C" Light* cpp_attachLight(char *name, SceneNode* base,
				  float r, float g, float b,
				  float radius)
{
  Light *l = mSceneMgr->createLight(name);

  l->setDiffuseColour(r,g,b);

  // This seems to look reasonably ok.
  l->setAttenuation(3*radius, 0, 0, 12.0/(radius*radius));

  // base might be null, sometimes lights don't have meshes
  if(base) base->attachObject(l);

  return l;
}

// Toggle between fullscreen and windowed mode.
extern "C" void cpp_toggleFullscreen()
{
  std::cout << "Not implemented yet\n";
}

extern "C" void cpp_setAmbient(float r, float g, float b, // Ambient light
			       float rs, float gs, float bs) // "Sunlight"
{
  ColourValue c = ColourValue(r, g, b);
  mSceneMgr->setAmbientLight(c);

  // Create a "sun" that shines light downwards. It doesn't look
  // completely right, but leave it for now.
  Light *l = mSceneMgr->createLight("Sun");
  l->setDiffuseColour(rs, gs, bs);
  l->setType(Light::LT_DIRECTIONAL);
  l->setDirection(0,-1,0);
}

extern "C" void cpp_setFog(float rf, float gf, float bf, // Fog color
			float flow, float fhigh) // Fog distance
{
  ColourValue fogColor( rf, gf, bf );
  mSceneMgr->setFog( FOG_LINEAR, fogColor, 0.0, flow, fhigh );

  // Don't render what you can't see anyway
  mCamera->setFarClipDistance(fhigh + 10);

  // Leave this out for now
  //vp->setBackgroundColour(fogColor);
}

extern "C" void cpp_startRendering()
{
  mRoot->startRendering();
}

// Copy a scene node and all its children
void cloneNode(SceneNode *from, SceneNode *to, char* name)
{
  to->setPosition(from->getPosition());
  to->setOrientation(from->getOrientation());
  to->setScale(from->getScale());

  SceneNode::ObjectIterator it = from->getAttachedObjectIterator();
  while(it.hasMoreElements())
    {
      // We can't handle non-entities. To be honest I have no idea
      // what dynamic_cast does or if it's correct here. I used to be
      // a C++ person but after discovering D I dropped C++ like it
      // was red hot iron and never looked back.
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
extern "C" SceneNode *cpp_insertNode(SceneNode *base, char* name,
				     float *pos, float scale)
{
  //std::cout << "cpp_insertNode(" << name << ")\n";
  SceneNode *node = root->createChildSceneNode(name);

  // Make a copy of the node
  cloneNode(base, node, name);

  // pos points to a Placement struct, which has the format
  // float x, y, z; // position
  // float r1, r2, r3; // rotation

  node->setPosition(pos[0], pos[1], pos[2]);

  // Rotate around X axis
  Quaternion xr(Radian(-pos[3]), Vector3::UNIT_X);

  // Rotate around Y axis
  Quaternion yr(Radian(-pos[4]), Vector3::UNIT_Y);

  // Rotate around Z axis
  Quaternion zr(Radian(-pos[5]), Vector3::UNIT_Z);

  // Rotates first around z, then y, then x
  node->setOrientation(xr*yr*zr);

  node->setScale(scale, scale, scale);

  return node;
}

// Create the water plane. It doesn't really resemble "water" yet
// though.
extern "C" void cpp_createWater(float level)
{
    // Create a plane aligned with the xy-plane.
    MeshManager::getSingleton().createPlane("water",
           ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
           Plane(Vector3::UNIT_Z, level),
	   150000,150000
	   //,20,20,true,1,5,5,Vector3::UNIT_Z
	   );
    Entity *ent = mSceneMgr->createEntity( "WaterEntity", "water" );
    root->createChildSceneNode()->attachObject(ent);
    ent->setCastShadows(false);
}

// Manual loader for meshes. Reloading individual meshes is too
// difficult, and not worth the trouble. Later I should make one
// loader for each NIF file, and whenever it is invoked it should
// somehow reload the entire file. How this is to be done when some of
// the meshes might be loaded and in use already, I have no
// idea. Let's just ignore it for now.

class MeshLoader : public ManualResourceLoader
{
public:

  void loadResource(Resource *resource)
  {
  }
} dummyLoader;

  // TODO/FIXME/DEBUG (MURDER/DEATH/KILL)
String LASTNAME;

// Load the contents of a mesh
extern "C" void cpp_createMesh(
		char* name,		// Name of the mesh
		int32_t numVerts,	// Number of vertices
		float* vertices,	// Vertex list
		float* normals,		// Normal list
		float* colors,		// Vertex colors
		float* uvs,		// Texture coordinates
		int32_t numFaces,	// Number of faces*3
		uint16_t* faces,	// Faces
		float radius,		// Bounding sphere
		char* material,		// Material
		// Bounding box
		float minX,float minY,float minZ,
		float maxX,float maxY,float maxZ,
		SceneNode *owner
		)
{
  //std::cerr << "Creating mesh " << name << "\n";

  MeshPtr msh = MeshManager::getSingleton().createManual(name, "Meshes",
							 &dummyLoader);

  Entity *e = mSceneMgr->createEntity(name, name);

  owner->attachObject(e);
  //msh->setSkeletonName(name);

  // Create vertex data structure
  msh->sharedVertexData = new VertexData();
  msh->sharedVertexData->vertexCount = numVerts;

  /// Create declaration (memory format) of vertex data
  VertexDeclaration* decl = msh->sharedVertexData->vertexDeclaration;

  int nextBuf = 0;
  // 1st buffer
  decl->addElement(nextBuf, 0, VET_FLOAT3, VES_POSITION);

  /// Allocate vertex buffer of the requested number of vertices (vertexCount) 
  /// and bytes per vertex (offset)
  HardwareVertexBufferSharedPtr vbuf = 
    HardwareBufferManager::getSingleton().createVertexBuffer(
	VertexElement::getTypeSize(VET_FLOAT3),
        numVerts, HardwareBuffer::HBU_STATIC_WRITE_ONLY);

  /// Upload the vertex data to the card
  vbuf->writeData(0, vbuf->getSizeInBytes(), vertices, true);

  /// Set vertex buffer binding so buffer 0 is bound to our vertex buffer
  VertexBufferBinding* bind = msh->sharedVertexData->vertexBufferBinding; 
  bind->setBinding(nextBuf++, vbuf);

  // The lists are read in the same order that they appear in NIF
  // files, and likely in memory. Sequential reads might possibly
  // avert an occational cache miss.

  // normals
  if(normals)
    {
      //std::cerr << "+ Adding normals\n";
      decl->addElement(nextBuf, 0, VET_FLOAT3, VES_NORMAL);
      vbuf = HardwareBufferManager::getSingleton().createVertexBuffer(
          VertexElement::getTypeSize(VET_FLOAT3),
          numVerts, HardwareBuffer::HBU_STATIC_WRITE_ONLY);

      vbuf->writeData(0, vbuf->getSizeInBytes(), normals, true);

      bind->setBinding(nextBuf++, vbuf);       
    }

  // vertex colors
  if(colors)
    {
      //std::cerr << "+ Adding vertex colors\n";
      // Use render system to convert colour value since colour packing varies
      RenderSystem* rs = Root::getSingleton().getRenderSystem();
      RGBA colorsRGB[numVerts];
      RGBA *pColour = colorsRGB;
      for(int i=0; i<numVerts; i++)
	{
	  rs->convertColourValue(ColourValue(colors[0],colors[1],colors[2], colors[3]),
				 pColour++);
	  colors += 4;
	}

      decl->addElement(nextBuf, 0, VET_COLOUR, VES_DIFFUSE);
      /// Allocate vertex buffer of the requested number of vertices (vertexCount) 
      /// and bytes per vertex (offset)
      vbuf = HardwareBufferManager::getSingleton().createVertexBuffer(
          VertexElement::getTypeSize(VET_COLOUR),
	  numVerts, HardwareBuffer::HBU_STATIC_WRITE_ONLY);
      /// Upload the vertex data to the card
      vbuf->writeData(0, vbuf->getSizeInBytes(), colorsRGB, true);

      /// Set vertex buffer binding so buffer 1 is bound to our colour buffer
      bind->setBinding(nextBuf++, vbuf);
    }

  if(uvs)
    {
      //std::cerr << "+ Adding texture coordinates\n";
      decl->addElement(nextBuf, 0, VET_FLOAT2, VES_TEXTURE_COORDINATES);
      vbuf = HardwareBufferManager::getSingleton().createVertexBuffer(
          VertexElement::getTypeSize(VET_FLOAT2),
          numVerts, HardwareBuffer::HBU_STATIC_WRITE_ONLY);

      vbuf->writeData(0, vbuf->getSizeInBytes(), uvs, true);

      bind->setBinding(nextBuf++, vbuf);       
    }

  // Create the submesh that holds triangle data
  SubMesh* sub = msh->createSubMesh(name);
  sub->useSharedVertices = true;

  if(numFaces)
    {
      //std::cerr << "+ Adding faces\n";
      /// Allocate index buffer of the requested number of faces
      HardwareIndexBufferSharedPtr ibuf = HardwareBufferManager::getSingleton().
	createIndexBuffer(
			  HardwareIndexBuffer::IT_16BIT, 
			  numFaces,
			  HardwareBuffer::HBU_STATIC_WRITE_ONLY);

      /// Upload the index data to the card
      ibuf->writeData(0, ibuf->getSizeInBytes(), faces, true);

      /// Set parameters of the submesh
      sub->indexData->indexBuffer = ibuf;
      sub->indexData->indexCount = numFaces;
      sub->indexData->indexStart = 0;
    }

  // Create a material with the given texture, if any.

  // If this mesh has a material, attach it.
  if(material) sub->setMaterialName(name);

  /*
  // Assign this submesh to the given bone
  VertexBoneAssignment v;
  v.boneIndex = ((Bone*)bone)->getHandle();
  v.weight = 1.0;

  std::cerr << "+ Assigning bone index " << v.boneIndex << "\n";

  for(int i=0; i < numVerts; i++)
    {
      v.vertexIndex = i;
      sub->addBoneAssignment(v);
    }
  */
  /// Set bounding information (for culling)
  msh->_setBounds(AxisAlignedBox(minX,minY,minZ,maxX,maxY,maxZ));

  //std::cerr << "+ Radius: " << radius << "\n";
  msh->_setBoundingSphereRadius(radius);
}

extern "C" void cpp_createMaterial(char *name,	    // Name to give
						    // resource

				   float *ambient,  // Ambient RBG
						    // value
				   float *diffuse,
				   float *specular,
				   float *emissive, // Self
						    // illumination

				   float glossiness,// Same as
						    // shininess?

				   float alpha,     // Use this in all
						    // alpha values?

				   char* texture)   // Texture
{
      MaterialPtr material = MaterialManager::getSingleton().create(
        name,
	ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

      // This assigns the texture to this material. If the texture
      // name is a file name, and this file exists (in a resource
      // directory), it will automatically be loaded when needed. If
      // not, we should already have inserted a manual loader for the texture.
      if(texture)
	material->getTechnique(0)->getPass(0)->createTextureUnitState(texture);

      // Set bells and whistles
      material->setAmbient(ambient[0], ambient[1], ambient[2]);
      material->setDiffuse(diffuse[0], diffuse[1], diffuse[2], alpha);
      material->setSpecular(specular[0], specular[1], specular[2], alpha);
      material->setSelfIllumination(emissive[0], emissive[1], emissive[2]);
      material->setShininess(glossiness);

      // Enable transparancy? This is just guesswork. Perhaps we
      // should use NiAlphaProperty or something instead. This looks
      // like crap so it's not enabled right now.

      //material->setSceneBlending(SBT_TRANSPARENT_ALPHA);

      // Temporary, just to store the name of one valid material.
      LASTNAME = material->getName();
}

extern "C" SceneNode *cpp_getDetachedNode()
{
  SceneNode *node = root->createChildSceneNode();
  root->removeChild(node);
  return node;
}

extern "C" SceneNode* cpp_createNode(
		char *name,
		float *trafo,
		SceneNode *parent,
		int32_t noRot)
{
  //std::cout << "cpp_createNode(" << name << ")";
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

/* Code currently not in use

// We need this later for animated meshes. Boy will this be a mess.
extern "C" void* cpp_setupSkeleton(char* name)
{
  SkeletonPtr skel = SkeletonManager::getSingleton().create(
    name, "Closet", true);

  skel->load();

  // Create all bones at the origin and unrotated. This is necessary
  // since our submeshes each have their own model space. We must
  // move the bones after creating an entity, then copy this entity.
  return (void*)skel->createBone();  
}

// Use this later when loading textures directly from NIF files
extern "C" void cpp_createTexture(char* name, uint32_t width, uint32_t height)
{
  TexturePtr texture = TextureManager::getSingleton().createManual(
      name, 		// name
      "ManualTexture",	// group
      TEX_TYPE_2D,     	// type
      width, hight,     // width & height
      0,                // number of mipmaps
      PF_BYTE_BGRA,     // pixel format
      TU_DEFAULT);      // usage; should be TU_DYNAMIC_WRITE_ONLY_DISCARDABLE for
                        // textures updated very often (e.g. each frame)

  // Get the pixel buffer
  HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();

  // Lock the pixel buffer and get a pixel box
  pixelBuffer->lock(HardwareBuffer::HBL_NORMAL); // for best performance use HBL_DISCARD!
  const PixelBox& pixelBox = pixelBuffer->getCurrentLock();

  uint8* pDest = static_cast<uint8*>(pixelBox.data);

  // Fill in some pixel data. This will give a semi-transparent blue,
  // but this is of course dependent on the chosen pixel format.
  for (size_t j = 0; j < 256; j++)
    for(size_t i = 0; i < 256; i++)
      {
        *pDest++ = 255; // B
        *pDest++ =   0; // G
        *pDest++ =   0; // R
        *pDest++ = 127; // A
      }

  // Unlock the pixel buffer
  pixelBuffer->unlock();

  // Create a material using the texture
  MaterialPtr material = MaterialManager::getSingleton().create(
	"DynamicTextureMaterial", // name
	ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

  material->getTechnique(0)->getPass(0)->createTextureUnitState("DynamicTexture");
  material->getTechnique(0)->getPass(0)->setSceneBlending(SBT_TRANSPARENT_ALPHA);
}

extern "C" void *cpp_insertBone(char* name, void* rootBone, int32_t index)
{
  return (void*) ( ((Bone*)rootBone)->createChild(index) );
}
*/
