#include <cstdlib>
#include <iostream>
#include <cstring>
#include <vector>
#include <assert.h>

#include "apusysTest.h"

IApusysTest::IApusysTest()
{
    mEngine = ISampleEngine::createInstance("sample case test");
    if (mEngine == nullptr)
    {
        LOG_ERR("create sample engine fail\n");
        assert(mEngine != nullptr);
    }
}

IApusysTest::~IApusysTest()
{
    if (ISampleEngine::deleteInstance(mEngine) == false)
    {
        LOG_ERR("delete sample engine fail\n");
    }
}

