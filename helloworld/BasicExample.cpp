

#include "BasicExample.h"

#include "btBulletDynamicsCommon.h"
#define ARRAY_SIZE_Y 5
#define ARRAY_SIZE_X 5
#define ARRAY_SIZE_Z 5

#include "LinearMath/btVector3.h"
#include "LinearMath/btAlignedObjectArray.h"

#include "../bullet3/examples/CommonInterfaces/CommonRigidBodyBase.h"


struct BasicExample : public CommonRigidBodyBase
{
    BasicExample(struct GUIHelperInterface* helper)
        :CommonRigidBodyBase(helper)
    {
    }
    virtual ~BasicExample(){}
    virtual void initPhysics();
    virtual void renderScene();
    void resetCamera()
    {
        float dist = 41;
        float pitch = 52;
        float yaw = 35;
        float targetPos[3] = { 0, 0.46, 0 };
        m_guiHelper->resetCamera(dist, pitch, yaw, targetPos[0], targetPos[1], targetPos[2]);
    }
};

void BasicExample::initPhysics()
{
    m_guiHelper->setUpAxis(1);

    createEmptyDynamicsWorld();

    m_guiHelper->createPhysicsDebugDrawer(m_dynamicsWorld);

    if (m_dynamicsWorld->getDebugDrawer())
        m_dynamicsWorld->getDebugDrawer()->setDebugMode(btIDebugDraw::DBG_DrawWireframe + btIDebugDraw::DBG_DrawContactPoints);

    ///create a few basic rigid bodies
    btBoxShape* groundShape = createBoxShape(btVector3(btScalar(50.), btScalar(50.), btScalar(50.)));


    //groundShape->initializePolyhedralFeatures();
    //	btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0,1,0),50);

    m_collisionShapes.push_back(groundShape);

    btTransform groundTransform;
    groundTransform.setIdentity();
    groundTransform.setOrigin(btVector3(0, -50, 0));

    {
        btScalar mass(0.);
        btRigidBody* body = createRigidBody(mass, groundTransform, groundShape, btVector4(0, 0, 1, 1));
    }


    {
        //create a few dynamic rigidbodies
        // Re-using the same collision is better for memory usage and performance

        btBoxShape* colShape = createBoxShape(btVector3(1, 1, 1));


        //btCollisionShape* colShape = new btSphereShape(btScalar(1.));
        m_collisionShapes.push_back(colShape);

        /// Create Dynamic Objects
        btTransform startTransform;
        startTransform.setIdentity();

        btScalar	mass(1.f);

        //rigidbody is dynamic if and only if mass is non zero, otherwise static
        bool isDynamic = (mass != 0.f);

        btVector3 localInertia(0, 0, 0);
        if (isDynamic)
            colShape->calculateLocalInertia(mass, localInertia);


        for (int k = 0; k<ARRAY_SIZE_Y; k++)
        {
            for (int i = 0; i<ARRAY_SIZE_X; i++)
            {
                for (int j = 0; j<ARRAY_SIZE_Z; j++)
                {
                    startTransform.setOrigin(btVector3(
                        btScalar(2.0*i),
                        btScalar(20 + 2.0*k),
                        btScalar(2.0*j)));


                    btRigidBody* body = createRigidBody(mass, startTransform, colShape);


                }
            }
        }
    }

    m_guiHelper->autogenerateGraphicsObjects(m_dynamicsWorld);
}


void BasicExample::renderScene()
{
    CommonRigidBodyBase::renderScene();

}







CommonExampleInterface*    BasicExampleCreateFunc(CommonExampleOptions& options)
{
    return new BasicExample(options.m_guiHelper);
}



