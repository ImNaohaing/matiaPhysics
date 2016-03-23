
#include "../CommonInterfaces/CommonRigidBodyBase.h"


// use "struct" indicates it is one implement of some class
struct BasicUsage : public CommonRigidBodyBase
{
    BasicUsage(struct GUIHelperInterface *helper) :
        CommonRigidBodyBase(helper)
    {
    }
    virtual ~BasicUsage(){}
    virtual void initPysics(){};
    virtual void renderScene() override{};
    const char * getsd(){ return sd; }
    char *      sd;
};