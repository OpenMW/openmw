#include <Ogre.h>
#include <iostream>
#include <assert.h>

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
    int numFaces = 2;
    const float vertices[] =
      { -1,-1,0, 1,-1,0,
        1,1,0,   -1,1,0 };

    const short faces[] =
      { 0,1,2,  0,3,2 };

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

    SubMesh* sub = mesh->createSubMesh("tris");
    sub->useSharedVertices = true;

    /// Set parameters of the submesh
    sub->indexData->indexBuffer = ibuf;
    sub->indexData->indexCount = numFaces;
    sub->indexData->indexStart = 0;

    sub->setMaterialName(MaterialManager::getSingleton().getDefaultSettings()->getName());

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

int main()
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
    if(!root->showConfigDialog())
      return 1;

  // Create a window
  window = root->initialise(true, "Test");

  // We might need input managment too

  // More initialization
  SceneManager *mgr = root->createSceneManager(ST_GENERIC);
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

  // Create a couple of manual meshes
  MeshManager::getSingleton().createManual("mesh1.mm", "General", &mml);
  MeshManager::getSingleton().createManual("mesh2.mm", "General", &mml);

  // Display the meshes
  SceneNode *node = mgr->getRootSceneNode()->createChildSceneNode("node");
  Entity *ent1 = mgr->createEntity("Mesh1", "mesh1.mm");
  node->attachObject(ent1);

  node->setPosition(0,0,30);

  // Render loop
  root->startRendering();

  // Cleanup
  delete root;
  return 0;
}
