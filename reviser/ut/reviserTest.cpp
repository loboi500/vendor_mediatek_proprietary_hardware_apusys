#include <cstdlib>
#include <iostream>
#include <cstring>
#include <vector>
#include <assert.h>

#include "reviserTest.h"

IReviserTest::IReviserTest()
{
    mSession = apusysSession_createInstance();
    if (mSession == nullptr)
    {
    	RVR_ERR("create apu session fail\n");
        abort();
    }
    mSetnum = 0;
    mVlmSize = 0;
    mLoopNum = 1;
}

IReviserTest::~IReviserTest()
{
	apusysSession_deleteInstance(mSession);
}


apusys_session_t *IReviserTest::getSession()
{
    return mSession;
}
