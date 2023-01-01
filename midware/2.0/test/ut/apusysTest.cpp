#include <cstdlib>
#include <iostream>
#include <cstring>
#include <vector>
#include <assert.h>

#include "apusysTest.h"

apusysTest::apusysTest()
{
    mSession = apusysSession_createInstance();
    if (mSession == nullptr)
    {
        LOG_ERR("create apu session fail\n");
        abort();
    }

    mCodeGen = new sampleCodeGen();
    if (mCodeGen == nullptr) {
        LOG_ERR("create sample codegen fail\n");
        abort();
    }
}

apusysTest::~apusysTest()
{
    delete mCodeGen;
    apusysSession_deleteInstance(mSession);
}

apusys_session_t *apusysTest::getSession()
{
    return mSession;
}

class sampleCodeGen *apusysTest::getCodeGen()
{
    return mCodeGen;
}
