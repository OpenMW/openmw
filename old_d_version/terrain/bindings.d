module terrain.bindings;

alias void *SceneNode;
alias void *Bounds;
alias void *MeshObj;

// These are all defined in cpp_terrain.cpp:
extern(C):

SceneNode terr_createChildNode(float relX, float relY, SceneNode);
void terr_destroyNode(SceneNode);
Bounds terr_makeBounds(float minHeight, float maxHeight, float width, SceneNode);
void terr_killBounds(Bounds);
float terr_getSqCamDist(Bounds);
MeshObj terr_makeMesh(SceneNode,void*,int,float);
void terr_killMesh(MeshObj);

void terr_genData();
void terr_setupRendering();

void terr_makeLandMaterial(char*,float);
ubyte *terr_makeAlphaLayer(char*,int);
void terr_closeAlpha(char*,char*,float);
void terr_cleanupAlpha(char*,void*,int);

void terr_resize(void*,void*,int,int);
void terr_saveImage(void*,int,char*);
