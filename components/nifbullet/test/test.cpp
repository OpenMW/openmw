#include "bullet_nif_loader.hpp"
#include "..\nifogre\ogre_nif_loader.hpp"
#include "..\bsa\bsa_archive.hpp"
#include "..\nifogre\ogre_nif_loader.hpp"
#include <Ogre.h>
#include <OIS.h>
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include "BtOgrePG.h"
#include "BtOgreGP.h"
#include "BtOgreExtras.h"

const char* mesh = "meshes\\x\\ex_hlaalu_b_24.nif";

class MyMotionState : public btMotionState {
public:
    MyMotionState(const btTransform &initialpos, Ogre::SceneNode *node) {
        mVisibleobj = node;
        mPos1 = initialpos;
        node->setPosition(initialpos.getOrigin().x(),initialpos.getOrigin().y(),initialpos.getOrigin().z());
    }

    virtual ~MyMotionState() {
    }

    void setNode(Ogre::SceneNode *node) {
        mVisibleobj = node;
    }

    virtual void getWorldTransform(btTransform &worldTrans) const {
        worldTrans = mPos1;
    }

    virtual void setWorldTransform(const btTransform &worldTrans) {
        if(NULL == mVisibleobj) return; // silently return before we set a node
        btQuaternion rot = worldTrans.getRotation();
        mVisibleobj->setOrientation(rot.w(), rot.x(), rot.y(), rot.z());
        btVector3 pos = worldTrans.getOrigin();
        mVisibleobj->setPosition(pos.x(), pos.y(), pos.z());
    }

protected:
    Ogre::SceneNode *mVisibleobj;
    btTransform mPos1;
};


int main()
{
    try
    {
        //Ogre stuff

        Ogre::Root* pRoot = new Ogre::Root();
        pRoot->showConfigDialog();

        BulletShapeManager* manag = new BulletShapeManager();

        Ogre::RenderWindow* win = pRoot->initialise(true,"test");
        Ogre::SceneManager* scmg = pRoot->createSceneManager(Ogre::ST_GENERIC,"MonGestionnaireDeScene");
        Ogre::Camera* pCamera = scmg->createCamera("test");
        Ogre::Viewport* pViewport = win->addViewport(pCamera);
        pCamera->setPosition(-50,0,0);
        pCamera->setFarClipDistance(10000);
        pCamera->setNearClipDistance(1.);
        pCamera->lookAt(0,0,0);
        //Ogre::ResourceGroupManager::getSingleton().addResourceLocation("C++/OgreSK/media/models","FileSystem","General");
        Ogre::ResourceGroupManager::getSingleton().addResourceLocation("","FileSystem","General");
        /*Ogre::ResourceGroupManager::getSingleton().addResourceLocation("C++/OgreSK/media/materials/scripts","FileSystem","General");
          Ogre::ResourceGroupManager::getSingleton().addResourceLocation("C++/OgreSK/media/materials/textures","FileSystem","General");
          Ogre::ResourceGroupManager::getSingleton().addResourceLocation("C++/OgreSK/media/materials/programs","FileSystem","General");*/


        //OIS stuff
        OIS::ParamList pl;
        size_t windowHnd = 0;
        std::ostringstream windowHndStr;
        win->getCustomAttribute("WINDOW", &windowHnd); 
        windowHndStr << windowHnd;
        pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));
        OIS::InputManager *pInputManager = OIS::InputManager::createInputSystem( pl );
        OIS::Mouse *pMouse = static_cast<OIS::Mouse*>(pInputManager->createInputObject(OIS::OISMouse, false));
        OIS::Keyboard* pKeyboard = static_cast<OIS::Keyboard*>(pInputManager->createInputObject(OIS::OISKeyboard, false));
        unsigned int width, height, depth;
        int top, left;
        win->getMetrics(width, height, depth, left, top);
        const OIS::MouseState &ms = pMouse->getMouseState();
        ms.width = width;
        ms.height = height;


        //Ressources stuff
        Bsa::addBSA("Morrowind.bsa");
        //Ogre::ResourceGroupManager::getSingleton().createResourceGroup("general");

        Ogre::ResourcePtr ptr = BulletShapeManager::getSingleton().getByName(mesh,"General");
        NifBullet::ManualBulletShapeLoader* ShapeLoader = new NifBullet::ManualBulletShapeLoader();

        ShapeLoader->load(mesh,"General");
        //BulletShapeManager::getSingleton().unload(mesh);
        //ShapeLoader->load(mesh,"General");

        NIFLoader::load(mesh);

        Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
        //BulletShapeManager::getSingleton().
        BulletShapePtr shape = BulletShapeManager::getSingleton().getByName(mesh,"General");
        BulletShapeManager::getSingleton().load(mesh,"General");
        BulletShapeManager::getSingleton().unload(mesh);
        BulletShapeManager::getSingleton().load(mesh,"General");
        BulletShapeManager::getSingleton().load(mesh,"General");
        //shape->load();
        //shape->unload();
        //shape->load();

        //Bullet init
        btBroadphaseInterface* broadphase = new btDbvtBroadphase();

        // Set up the collision configuration and dispatcher
        btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
        btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);

        // The actual physics solver
        btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;

        // The world.
        btDiscreteDynamicsWorld* dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,broadphase,solver,collisionConfiguration);
        dynamicsWorld->setGravity(btVector3(0,-10,0));



        //le sol?
        Ogre::SceneNode *node = scmg->getRootSceneNode()->createChildSceneNode("node");
        Ogre::Entity *ent = scmg->createEntity("Mesh1",mesh);
        node->attachObject(ent);
        MyMotionState* mst = new MyMotionState(btTransform::getIdentity(),node);

        btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0,mst,shape->Shape,btVector3(0,0,0));
        btRigidBody* groundRigidBody = new btRigidBody(groundRigidBodyCI);
        dynamicsWorld->addRigidBody(groundRigidBody);

        //une balle:
        Ogre::SceneNode *node2 = scmg->getRootSceneNode()->createChildSceneNode("node2");
        Ogre::Entity *ent2 = scmg->createEntity("Mesh2","ogrehead.mesh");
        node2->attachObject(ent2);
        node2->setPosition(0,500,0);
        btTransform iT;
        iT.setIdentity();
        iT.setOrigin(btVector3(0,5000,0));
        MyMotionState* mst2 = new MyMotionState(btTransform::getIdentity(),node2);

        btSphereShape* sphereshape = new btSphereShape(10);
        btRigidBody::btRigidBodyConstructionInfo sphereCI(10,mst2,sphereshape,btVector3(0,0,0));
        btRigidBody* sphere = new btRigidBody(sphereCI);
        dynamicsWorld->addRigidBody(sphere);


        //btOgre!
        BtOgre::DebugDrawer* mDebugDrawer = new BtOgre::DebugDrawer(scmg->getRootSceneNode(), dynamicsWorld);
        dynamicsWorld->setDebugDrawer(mDebugDrawer);

        Ogre::Timer timer;
        timer.reset();
        bool cont = true;
        while(cont)
        {
            if(timer.getMilliseconds()>30)
            {
                pMouse->capture();
                pKeyboard->capture();

                Ogre::Vector3 a(0,0,0);

                if(pKeyboard->isKeyDown(OIS::KC_UP))
                {
                    a = a + Ogre::Vector3(0,0,-20);
                }
                if(pKeyboard->isKeyDown(OIS::KC_DOWN))
                {
                    a = a + Ogre::Vector3(0,0,20);
                }
                if(pKeyboard->isKeyDown(OIS::KC_ESCAPE))
                {
                    cont = false;
                }
                OIS::MouseState MS = pMouse->getMouseState();
                pCamera->yaw(-Ogre::Degree(MS.X.rel));
                pCamera->pitch(-Ogre::Degree(MS.Y.rel));
                pCamera->moveRelative(a);

                pRoot->renderOneFrame();
                mDebugDrawer->step();
                timer.reset();
                dynamicsWorld->stepSimulation(0.03);
            }
        }
        std::cout << "cool";
        delete manag;
        delete pRoot;
        char a;
        std::cin >> a;
    }
    catch(Ogre::Exception& e)
    {
        std::cout << e.getFullDescription();
        char a;
        std::cin >> a;
    }
}
