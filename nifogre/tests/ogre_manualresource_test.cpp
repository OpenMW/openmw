#include <Ogre.h>
#include <iostream>
#include <assert.h>

/*
  This is a test of the manual resource loader interface to Ogre,
  applied to manually created meshes. It defines a simple mesh
  consisting of two triangles, and creates three instances of it as
  different meshes using the same loader. It is a precursor to the NIF
  loading code. If the Ogre interface changes and you have to change
  this test, then you will also have to change parts of the NIF
  loader.
 */

using namespace std;
using namespace Ogre;

struct MyMeshLoader : ManualResourceLoader
{
  void loadResource(Resource *resource)
  {
    Mesh *mesh = dynamic_cast<Mesh*>(resource);
    assert(mesh);

    const String& name = mesh->getName();
    cout << "Manually loading mesh " << name << endl;

    // Create the mesh here
    int numVerts = 4;
    int numFaces = 2*3;
    const float vertices[] =
      { -1,-1,0, 1,-1,0,
        1,1,0,   -1,1,0 };

    const short faces[] =
      { 0,2,1,  0,3,2 };

    mesh->sharedVertexData = new VertexData();
    mesh->sharedVertexData->vertexCount = numVerts;

    VertexDeclaration* decl = mesh->sharedVertexData->vertexDeclaration;

    decl->addElement(0, 0, VET_FLOAT3, VES_POSITION);

    HardwareVertexBufferSharedPtr vbuf = 
      HardwareBufferManager::getSingleton().createVertexBuffer(
	VertexElement::getTypeSize(VET_FLOAT3),
        numVerts, HardwareBuffer::HBU_STATIC_WRITE_ONLY);

    // Upload the vertex data to the card
    vbuf->writeData(0, vbuf->getSizeInBytes(), vertices, true);

    // Set vertex buffer binding so buffer 0 is bound to our vertex buffer
    VertexBufferBinding* bind = mesh->sharedVertexData->vertexBufferBinding; 
    bind->setBinding(0, vbuf);

    /// Allocate index buffer of the requested number of faces
    HardwareIndexBufferSharedPtr ibuf = HardwareBufferManager::getSingleton().
      createIndexBuffer(HardwareIndexBuffer::IT_16BIT, 
                        numFaces,
                        HardwareBuffer::HBU_STATIC_WRITE_ONLY);

    /// Upload the index data to the card
    ibuf->writeData(0, ibuf->getSizeInBytes(), faces, true);

    SubMesh* sub = mesh->createSubMesh(name+"tris");
    sub->useSharedVertices = true;

    /// Set parameters of the submesh
    sub->indexData->indexBuffer = ibuf;
    sub->indexData->indexCount = numFaces;
    sub->indexData->indexStart = 0;

    mesh->_setBounds(AxisAlignedBox(-1.1,-1.1,-1.1,1.1,1.1,1.1));
    mesh->_setBoundingSphereRadius(2);
  }
};

MyMeshLoader mml;

RenderWindow *window;

// Lets you quit by closing the window
struct QuitListener : FrameListener
{
  bool frameStarted(const FrameEvent& evt)
  {
    if(window->isClosed())
      return false;
    return true;
  }
} qlistener;

int main(int argc, char**args)
{
  // When the test is done, consider disabling the rendering part
  // unless a command line parameter is given (and write a note about
  // this to console.) This allows you to run the test from scripts
  // and still do some meaningful testing, even if you can't inpsect
  // the result visually.

  // Disable Ogre logging
  new LogManager;
  Log *log = LogManager::getSingleton().createLog("");
  log->setDebugOutputEnabled(false);

  // Set up Root.
  Root *root = new Root("plugins.cfg","ogre.cfg","");

  if(!root->restoreConfig())
    {
      cout << "WARNING: we do NOT recommend fullscreen mode!\n";
      if(!root->showConfigDialog())
        return 1;
    }

  SceneManager *mgr = root->createSceneManager(ST_GENERIC);

  // Only render if there are arguments on the command line (we don't
  // care what they are.)
  bool render = (argc>=2);

  // Create a window
  window = root->initialise(true, "Test");
  if(render)
    {
      // More initialization
      Camera *cam = mgr->createCamera("cam");
      Viewport *vp = window->addViewport(cam);
      cam->setAspectRatio(Real(vp->getActualWidth()) / Real(vp->getActualHeight()));
      cam->setFOVy(Degree(55));
      cam->setPosition(0,0,0);
      cam->lookAt(0,0,10);
      cam->setNearClipDistance(1);

      root->addFrameListener(&qlistener);

      // Background color
      vp->setBackgroundColour(ColourValue(0.5,0.5,0.5));

      mgr->setAmbientLight(ColourValue(1,1,1));
    }

  // Create a couple of manual meshes
  MeshManager::getSingleton().createManual("mesh1.mm", "General", &mml);
  MeshManager::getSingleton().createManual("mesh2.mm", "General", &mml);
  MeshManager::getSingleton().createManual("mesh3.mm", "General", &mml);

  // Display the meshes
  {
    SceneNode *node = mgr->getRootSceneNode()->createChildSceneNode("node");
    Entity *ent = mgr->createEntity("Mesh1", "mesh1.mm");
    node->attachObject(ent);
    node->setPosition(3,1,8);
  }

  {
    SceneNode *node = mgr->getRootSceneNode()->createChildSceneNode("node2");
    Entity *ent = mgr->createEntity("Mesh2", "mesh2.mm");
    node->attachObject(ent);
    node->setPosition(-3,1,8);
  }
  {
    SceneNode *node = mgr->getRootSceneNode()->createChildSceneNode("node3");
    Entity *ent = mgr->createEntity("Mesh3", "mesh3.mm");
    node->attachObject(ent);
    node->setPosition(0,-2,8);
  }

  // Render loop
  if(render)
    {
      cout << "Rendering. Close the window to exit.\n";
      root->startRendering();
    }

  // Cleanup
  delete root;
  return 0;
}
