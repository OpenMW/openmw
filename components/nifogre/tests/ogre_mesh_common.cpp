#include "ogre_common.cpp"

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

MeshPtr makeMesh(const string &name)
{
  return MeshManager::getSingleton().createManual(name, "General", &mml);
}
